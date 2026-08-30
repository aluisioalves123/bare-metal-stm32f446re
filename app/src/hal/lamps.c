#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "board.h"
#include "hal/lamps.h"

void lamps_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOA);

  gpio_mode_setup(
    LEDS_PORT,
    GPIO_MODE_OUTPUT,
    GPIO_PUPD_NONE,
    TURN_SIGNAL_RIGHT_LED_PIN | TURN_SIGNAL_LEFT_LED_PIN
  );

  // estado padrao dos farois: acesos (o ODR nasce zerado depois do reset)
  gpio_set(LEDS_PORT, TURN_SIGNAL_RIGHT_LED_PIN | TURN_SIGNAL_LEFT_LED_PIN);
}

void lamps_all_off(void) {
  gpio_clear(LEDS_PORT, TURN_SIGNAL_RIGHT_LED_PIN | TURN_SIGNAL_LEFT_LED_PIN);
}

void blink_leds(leds_should_blink_t leds_should_blink) {
  if (leds_should_blink.turn_signal_right) {
    gpio_toggle(LEDS_PORT, TURN_SIGNAL_RIGHT_LED_PIN);
  } else {
    gpio_set(LEDS_PORT, TURN_SIGNAL_RIGHT_LED_PIN);
  }

  if (leds_should_blink.turn_signal_left) {
    gpio_toggle(LEDS_PORT, TURN_SIGNAL_LEFT_LED_PIN);
  } else {
    gpio_set(LEDS_PORT, TURN_SIGNAL_LEFT_LED_PIN);
  }
}
