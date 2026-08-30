#ifndef INC_BOARD_H
#define INC_BOARD_H

#include <libopencm3/stm32/gpio.h>

// Mapa do hardware. Se a fiacao ou a PCB mudar, este e o unico arquivo
// que precisa mudar junto.

// --- Farois ---
#define LEDS_PORT                     (GPIOA)
#define TURN_SIGNAL_RIGHT_LED_PIN     (GPIO5)
#define TURN_SIGNAL_LEFT_LED_PIN      (GPIO6)

// --- Botoes (ativos em baixo, com pull-up interno) ---
#define TURN_SIGNAL_RIGHT_BUTTON_PORT (GPIOB)
#define TURN_SIGNAL_RIGHT_BUTTON_PIN  (GPIO6)

#define TURN_SIGNAL_LEFT_BUTTON_PORT  (GPIOC)
#define TURN_SIGNAL_LEFT_BUTTON_PIN   (GPIO7)

#define HAZARD_BUTTON_PORT            (GPIOA)
#define HAZARD_BUTTON_PIN             (GPIO9)

#endif // INC_BOARD_H
