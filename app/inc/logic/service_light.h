#ifndef INC_LOGIC_SERVICE_LIGHT_H
#define INC_LOGIC_SERVICE_LIGHT_H

#include "hal/buttons.h"

typedef enum {
  SERVICE_LIGHT_OFF,
  SERVICE_LIGHT_ON
} service_light_state_t;

// pura: recebe o scan anterior por parametro em vez de guardar num global.
// a comparacao com o anterior e o que detecta a BORDA do aperto; sem ela,
// segurar o botao trocaria de estado 1000x por segundo.
service_light_state_t next_service_light_state(service_light_state_t current,
                                               button_states_t now,
                                               button_states_t previous);

#endif // INC_LOGIC_SERVICE_LIGHT_H
