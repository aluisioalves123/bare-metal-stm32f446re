#include <libopencm3/stm32/rcc.h>

#include "hal/buttons.h"
#include "hal/lamps.h"
#include "hal/service_light.h"
#include "hal/systick.h"
#include "logic/service_light.h"
#include "logic/turn_signal.h"

#define BLINK_INTERVAL_MS (333)

static void rcc_setup(void) {
  rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_180MHZ]);
}

int main(void) {
  rcc_setup();
  lamps_setup();
  buttons_setup();
  service_light_setup();
  systick_setup();

  uint64_t last_scan = get_ticks();
  uint64_t last_blink = 0;

  // todo o estado que sobrevive de uma iteracao para a outra mora aqui,
  // a vista, em vez de escondido em globais dentro dos modulos
  debounce_counters_t debounce = {
    .turn_signal_right = 0,
    .turn_signal_left  = 0,
    .hazard            = 0,
    .service_light     = 0
  };
  button_states_t previous_buttons = {
    .turn_signal_right_button_pressed = false,
    .turn_signal_left_button_pressed  = false,
    .hazard_button_pressed            = false,
    .service_light_button_pressed     = false
  };
  signal_state_t signal_state = SIGNAL_OFF;
  service_light_state_t service_light_state = SERVICE_LIGHT_OFF;
  leds_should_blink_t leds_should_blink = which_leds_blink(signal_state);

  while (1) {
    uint64_t now = get_ticks();

    if (now != last_scan) { // entra aqui a cada 1 ms
      last_scan = now;

      debounce = next_debounce(debounce, read_buttons());
      button_states_t buttons = debounced_buttons(debounce);

      signal_state_t next_state = next_signal_state(signal_state, buttons, previous_buttons);
      service_light_state_t next_service_light = next_service_light_state(service_light_state, buttons, previous_buttons);
      previous_buttons = buttons;

      if (next_state != signal_state) {
        signal_state = next_state;
        leds_should_blink = which_leds_blink(signal_state);

        // troca de modo: zera os farois e recomeca o ciclo de piscada do
        // zero, para o novo sinal acender na hora em vez de esperar o
        // proximo intervalo
        lamps_all_off();
        blink_leds(leds_should_blink);
        last_blink = now;
      }

      service_light_state = next_service_light;

      activate_service_light(service_light_state);
    }

    if (now - last_blink >= BLINK_INTERVAL_MS) {
      blink_leds(leds_should_blink);
      last_blink = now;
    }
  }

  // Never return
  return 0;
}
