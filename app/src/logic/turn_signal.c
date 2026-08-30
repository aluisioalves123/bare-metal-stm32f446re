#include "logic/turn_signal.h"

signal_state_t next_signal_state(signal_state_t current,
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

leds_should_blink_t which_leds_blink(signal_state_t signal_state) {
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
