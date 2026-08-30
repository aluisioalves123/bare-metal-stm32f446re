#ifndef INC_TURN_SIGNAL_H
#define INC_TURN_SIGNAL_H

#include <stdbool.h>

#include "hal/buttons.h"

typedef enum {
  SIGNAL_OFF,
  SIGNAL_RIGHT,
  SIGNAL_LEFT,
  SIGNAL_HAZARD
} signal_state_t;

typedef struct {
  bool turn_signal_right;
  bool turn_signal_left;
} leds_should_blink_t;

// pura: recebe o scan anterior por parametro em vez de guardar num global.
// a comparacao com o anterior e o que detecta a BORDA do aperto; sem ela,
// segurar o botao trocaria de estado 1000x por segundo.
signal_state_t next_signal_state(signal_state_t current,
                                 button_states_t now,
                                 button_states_t previous);

// pura: estado -> quais farois devem piscar
leds_should_blink_t which_leds_blink(signal_state_t signal_state);

#endif // INC_TURN_SIGNAL_H
