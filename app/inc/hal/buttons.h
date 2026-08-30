#ifndef INC_BUTTONS_H
#define INC_BUTTONS_H

#include <stdbool.h>
#include <stdint.h>

#define DEBOUNCE_TICKS (20)

typedef struct {
  bool turn_signal_right_button_pressed;
  bool turn_signal_left_button_pressed;
  bool hazard_button_pressed;
  bool service_light_button_pressed;
} button_states_t;

// quantos scans seguidos cada botao aparece apertado
typedef struct {
  uint32_t turn_signal_right;
  uint32_t turn_signal_left;
  uint32_t hazard;
  uint32_t service_light;
} debounce_counters_t;

void buttons_setup(void);

// IMPURA: le os pinos. E aqui que morre a polaridade do hardware.
button_states_t read_buttons(void);

// pura: contadores de antes + leitura crua -> contadores de agora
debounce_counters_t next_debounce(debounce_counters_t counters,
                                  button_states_t raw);

// pura: contadores -> quais botoes ja contam como apertados de verdade
button_states_t debounced_buttons(debounce_counters_t counters);

#endif // INC_BUTTONS_H
