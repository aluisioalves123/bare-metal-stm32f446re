#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "board.h"
#include "hal/buttons.h"

void buttons_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);

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

  gpio_mode_setup(
    SERVICE_LIGHT_BUTTON_PORT,
    GPIO_MODE_INPUT,
    GPIO_PUPD_PULLUP,
    SERVICE_LIGHT_BUTTON_PIN
  );
}

button_states_t read_buttons(void) {
  return (button_states_t){
    .turn_signal_right_button_pressed = (gpio_get(TURN_SIGNAL_RIGHT_BUTTON_PORT, TURN_SIGNAL_RIGHT_BUTTON_PIN) == 0),
    .turn_signal_left_button_pressed  = (gpio_get(TURN_SIGNAL_LEFT_BUTTON_PORT, TURN_SIGNAL_LEFT_BUTTON_PIN) == 0),
    .hazard_button_pressed            = (gpio_get(HAZARD_BUTTON_PORT, HAZARD_BUTTON_PIN) == 0),
    .service_light_button_pressed     = (gpio_get(SERVICE_LIGHT_BUTTON_PORT, SERVICE_LIGHT_BUTTON_PIN) == 0)
  };
}

debounce_counters_t next_debounce(debounce_counters_t counters,
                                  button_states_t raw) {
  return (debounce_counters_t){
    .turn_signal_right = raw.turn_signal_right_button_pressed ? counters.turn_signal_right + 1 : 0,
    .turn_signal_left  = raw.turn_signal_left_button_pressed  ? counters.turn_signal_left  + 1 : 0,
    .hazard            = raw.hazard_button_pressed            ? counters.hazard            + 1 : 0,
    .service_light     = raw.service_light_button_pressed     ? counters.service_light     + 1 : 0
  };
}

button_states_t debounced_buttons(debounce_counters_t counters) {
  return (button_states_t){
    .turn_signal_right_button_pressed = (counters.turn_signal_right > DEBOUNCE_TICKS),
    .turn_signal_left_button_pressed  = (counters.turn_signal_left  > DEBOUNCE_TICKS),
    .hazard_button_pressed            = (counters.hazard            > DEBOUNCE_TICKS),
    .service_light_button_pressed     = (counters.service_light     > DEBOUNCE_TICKS)
  };
}
