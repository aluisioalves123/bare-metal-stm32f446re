#include "logic/service_light.h"

service_light_state_t next_service_light_state(service_light_state_t current,
                                               button_states_t now,
                                               button_states_t previous) {
  // so vale o instante em que o botao passou de solto para apertado
  bool service_light_edge = now.service_light_button_pressed &&
                            !previous.service_light_button_pressed;

  // apertar liga; apertar de novo desliga
  if (service_light_edge) {
    return (current == SERVICE_LIGHT_ON) ? SERVICE_LIGHT_OFF : SERVICE_LIGHT_ON;
  }

  // nenhum aperto novo: o estado se mantem
  return current;
}
