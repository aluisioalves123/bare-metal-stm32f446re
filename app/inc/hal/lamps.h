#ifndef INC_LAMPS_H
#define INC_LAMPS_H

#include "logic/turn_signal.h"

void lamps_setup(void);

// apaga os dois farois de uma vez, usado ao trocar de modo
void lamps_all_off(void);

// alterna o lado que sinaliza e mantem o outro aceso
void blink_leds(leds_should_blink_t leds_should_blink);

#endif // INC_LAMPS_H
