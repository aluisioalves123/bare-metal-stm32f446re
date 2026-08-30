#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/vector.h>

#include "hal/systick.h"

// volatile porque quem escreve e a interrupcao, nao o fluxo do main.
// sem isso o compilador poderia cachear o valor num registrador e o
// laco do main nunca veria a variavel mudar.
static volatile uint64_t ticks = 0;

void sys_tick_handler(void) {
  ticks++;
}

uint64_t get_ticks(void) {
  return ticks;
}

void systick_setup(void) {
  systick_set_frequency(SYSTICK_FREQ, CPU_FREQ);
  systick_counter_enable();
  systick_interrupt_enable();
}
