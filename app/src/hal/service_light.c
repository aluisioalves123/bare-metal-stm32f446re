#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

#include "hal/service_light.h"

void service_light_setup(void) {
  // toda saida se habilita por completo: porta, pino e clock do periferico.
  // o clock do GPIOB fica repetido em relacao ao buttons_setup de proposito:
  // se um dia os botoes sairem, o service light nao pode parar de funcionar
  // por tabela, por uma coisa que nem e da conta dele.
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_TIM2);

  // PB10 em funcao alternativa AF1, para receber a saida do TIM2
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10);
  gpio_set_af(GPIOB, GPIO_AF1, GPIO10);

  // TIM2: PWM modo 1 no canal 3
  timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_prescaler(TIM2, 224);
  timer_set_period(TIM2, 399);
  timer_set_oc_mode(TIM2, TIM_OC3, TIM_OCM_PWM1);
  // preload: a escrita no CCR so vale no proximo update, para nao cortar
  // o ciclo no meio e sair um pulso torto
  timer_enable_oc_preload(TIM2, TIM_OC3);
  timer_enable_oc_output(TIM2, TIM_OC3);
  timer_enable_counter(TIM2);
}

void activate_service_light(service_light_state_t service_light_state) {
  uint32_t ccr = TIM_CCR3(TIM2);

  if (service_light_state == SERVICE_LIGHT_ON) {
    if (ccr < TIM_ARR(TIM2)) {
      timer_set_oc_value(TIM2, TIM_OC3, ccr + 1);
    }
  } else {
    if (ccr > 0) {
      timer_set_oc_value(TIM2, TIM_OC3, ccr - 1);
    }
  }
}
