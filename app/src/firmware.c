#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/vector.h>
#include <stdbool.h>

#define LEDS_PORT (GPIOA)
#define TURN_SIGNAL_RIGHT_LED_PIN (GPIO5)
#define TURN_SIGNAL_LEFT_LED_PIN (GPIO6)

#define TURN_SIGNAL_RIGHT_BUTTON_PORT (GPIOB)
#define TURN_SIGNAL_RIGHT_BUTTON_PIN (GPIO6)

#define TURN_SIGNAL_LEFT_BUTTON_PORT (GPIOC)
#define TURN_SIGNAL_LEFT_BUTTON_PIN (GPIO7)

#define HAZARD_BUTTON_PORT (GPIOA)
#define HAZARD_BUTTON_PIN (GPIO9)

#define CPU_FREQ (180000000)
#define SYSTICK_FREQ (1000)
#define DEBOUNCE_TICKS (20)

volatile uint64_t ticks = 0;
void sys_tick_handler(void) {
  ticks++;
}

static uint64_t get_ticks(void) {
  return ticks;
}

static void rcc_setup(void) {
  rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_180MHZ]);
}

static void gpio_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);

  gpio_mode_setup(
    LEDS_PORT, 
    GPIO_MODE_OUTPUT, 
    GPIO_PUPD_NONE, 
    TURN_SIGNAL_RIGHT_LED_PIN | TURN_SIGNAL_LEFT_LED_PIN
  );

  // estado padrao dos LEDs: acesos (o ODR nasce zerado depois do reset)
  gpio_set(LEDS_PORT, TURN_SIGNAL_RIGHT_LED_PIN | TURN_SIGNAL_LEFT_LED_PIN);

  gpio_mode_setup(
    TURN_SIGNAL_RIGHT_BUTTON_PORT, 
    GPIO_MODE_INPUT, 
    GPIO_PUPD_PULLUP, 
    TURN_SIGNAL_RIGHT_BUTTON_PIN
  );

  gpio_mode_setup(
    TURN_SIGNAL_LEFT_BUTTON_PORT, 
    GPIO_MODE_INPUT, 
    GPIO_PUPD_PULLUP, 
    TURN_SIGNAL_LEFT_BUTTON_PIN
  );

  gpio_mode_setup(
    HAZARD_BUTTON_PORT, 
    GPIO_MODE_INPUT, 
    GPIO_PUPD_PULLUP, 
    HAZARD_BUTTON_PIN
  );
}

static void systick_setup(void) {
  systick_set_frequency(SYSTICK_FREQ, CPU_FREQ);
  systick_counter_enable();
  systick_interrupt_enable();
}

typedef struct {
  bool turn_signal_right_button_pressed;
  bool turn_signal_left_button_pressed;
  bool hazard_button_pressed;
} button_states_t;

// quantos scans seguidos cada botao aparece apertado
typedef struct {
  uint32_t turn_signal_right;
  uint32_t turn_signal_left;
  uint32_t hazard;
} debounce_counters_t;

static button_states_t read_buttons(void) {
  return (button_states_t){
    .turn_signal_right_button_pressed = (gpio_get(TURN_SIGNAL_RIGHT_BUTTON_PORT, TURN_SIGNAL_RIGHT_BUTTON_PIN) == 0),
    .turn_signal_left_button_pressed  = (gpio_get(TURN_SIGNAL_LEFT_BUTTON_PORT, TURN_SIGNAL_LEFT_BUTTON_PIN) == 0),
    .hazard_button_pressed            = (gpio_get(HAZARD_BUTTON_PORT, HAZARD_BUTTON_PIN) == 0)
  };
}

static debounce_counters_t next_debounce(debounce_counters_t counters,
                                         button_states_t raw) {
  return (debounce_counters_t){
    .turn_signal_right = raw.turn_signal_right_button_pressed ? counters.turn_signal_right + 1 : 0,
    .turn_signal_left  = raw.turn_signal_left_button_pressed  ? counters.turn_signal_left  + 1 : 0,
    .hazard            = raw.hazard_button_pressed            ? counters.hazard            + 1 : 0
  };
}

static button_states_t debounced_buttons(debounce_counters_t counters) {
  return (button_states_t){
    .turn_signal_right_button_pressed = (counters.turn_signal_right > DEBOUNCE_TICKS),
    .turn_signal_left_button_pressed  = (counters.turn_signal_left  > DEBOUNCE_TICKS),
    .hazard_button_pressed            = (counters.hazard            > DEBOUNCE_TICKS)
  };
}

typedef struct {
  bool turn_signal_right;
  bool turn_signal_left;
} leds_should_blink_t;

typedef enum {
  SIGNAL_OFF,
  SIGNAL_RIGHT,
  SIGNAL_LEFT,
  SIGNAL_HAZARD
} signal_state_t;

// a comparacao com o anterior e o que detecta a BORDA do aperto
// sem ela, segurar o botao trocaria de estado 1000x por segundo.
static signal_state_t next_signal_state(signal_state_t current,
                                        button_states_t now,
                                        button_states_t previous) {
  // so vale o instante em que o botao passou de solto para apertado
  bool right_edge  = now.turn_signal_right_button_pressed &&
                     !previous.turn_signal_right_button_pressed;
  bool left_edge   = now.turn_signal_left_button_pressed &&
                     !previous.turn_signal_left_button_pressed;
  bool hazard_edge = now.hazard_button_pressed &&
                     !previous.hazard_button_pressed;

  // apertar o mesmo botao do estado atual desliga; qualquer outro troca de modo
  if (hazard_edge) {
    return (current == SIGNAL_HAZARD) ? SIGNAL_OFF : SIGNAL_HAZARD;
  }
  if (right_edge) {
    return (current == SIGNAL_RIGHT) ? SIGNAL_OFF : SIGNAL_RIGHT;
  }
  if (left_edge) {
    return (current == SIGNAL_LEFT) ? SIGNAL_OFF : SIGNAL_LEFT;
  }

  // nenhum aperto novo: o estado se mantem
  return current;
}

static leds_should_blink_t which_leds_blink(signal_state_t signal_state) {
  switch (signal_state) {
    case SIGNAL_HAZARD:
      return (leds_should_blink_t){
        .turn_signal_right = true,
        .turn_signal_left  = true
      };
    case SIGNAL_RIGHT:
      return (leds_should_blink_t){
        .turn_signal_right = true,
        .turn_signal_left  = false
      };
    case SIGNAL_LEFT:
      return (leds_should_blink_t){
        .turn_signal_right = false,
        .turn_signal_left  = true
      };
    case SIGNAL_OFF:
    default:
      return (leds_should_blink_t){
        .turn_signal_right = false,
        .turn_signal_left  = false
      };
  }
}

static void blink_leds(leds_should_blink_t leds_should_blink) {
  
  if (leds_should_blink.turn_signal_right) {
    gpio_toggle(LEDS_PORT, TURN_SIGNAL_RIGHT_LED_PIN);
  } else {
    gpio_set(LEDS_PORT, TURN_SIGNAL_RIGHT_LED_PIN);
  };

  if (leds_should_blink.turn_signal_left) {
    gpio_toggle(LEDS_PORT, TURN_SIGNAL_LEFT_LED_PIN);
  } else {
    gpio_set(LEDS_PORT, TURN_SIGNAL_LEFT_LED_PIN);
  };
}

int main(void) {
  rcc_setup();
  gpio_setup();
  systick_setup();

  uint64_t last_scan = get_ticks();
  uint64_t last_blink = 0;

  // todo o estado que sobrevive de uma iteracao para a outra mora aqui,
  // a vista, em vez de escondido em globais dentro das funcoes
  debounce_counters_t debounce = {
    .turn_signal_right = 0,
    .turn_signal_left  = 0,
    .hazard            = 0
  };
  button_states_t previous_buttons = {
    .turn_signal_right_button_pressed = false,
    .turn_signal_left_button_pressed  = false,
    .hazard_button_pressed            = false
  };
  signal_state_t signal_state = SIGNAL_OFF;
  leds_should_blink_t leds_should_blink = which_leds_blink(signal_state);

  while (1) {
    uint64_t now = get_ticks();

    if (now != last_scan) { // entra aqui a cada 1 ms
      last_scan = now;

      debounce = next_debounce(debounce, read_buttons());
      button_states_t buttons = debounced_buttons(debounce);

      signal_state_t next_state = next_signal_state(signal_state, buttons, previous_buttons);
      previous_buttons = buttons;

      if (next_state != signal_state) {
        signal_state = next_state;
        leds_should_blink = which_leds_blink(signal_state);

        // troca de modo: zera os LEDs e recomeca o ciclo de piscada do zero,
        // para o novo sinal acender na hora em vez de esperar os 500 ms
        gpio_clear(LEDS_PORT, TURN_SIGNAL_RIGHT_LED_PIN | TURN_SIGNAL_LEFT_LED_PIN);
        blink_leds(leds_should_blink);
        last_blink = now;
      }
    }

    if (now - last_blink > 333) {
      blink_leds(leds_should_blink);
      last_blink = now;
    }
  }

  // Never return
  return 0;
}