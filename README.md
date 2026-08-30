# Bare Metal STM32F446RE

Firmware ARM Cortex-M4 escrito do zero, sem HAL e sem código gerado — só
registradores, linker script e o que eu escrever.

Este repositório acompanha a **Bare Metal Programming Series** do canal
**Low Byte Productions** ([código do curso](https://github.com/lowbyteproductions/bare-metal-series)),
com um detalhe: o curso usa uma
**Nucleo-F401RE** e um **J-Link**, e eu estou fazendo numa
**Nucleo-F446RE** com o **ST-Link on-board**. Cada diferença que aparece
está documentada aqui.

## Hardware e ferramentas

| | |
|---|---|
| Placa | NUCLEO-F446RE (STM32F446RET6, Cortex-M4F, 512K flash / 128K RAM) |
| Gravador | ST-Link V2-1 on-board, via SWD |
| Compilador | `arm-none-eabi-gcc` 14.3.1 |
| Biblioteca | [libopencm3](https://github.com/libopencm3/libopencm3) (submódulo) |
| Gravação/debug | OpenOCD 0.12 + Cortex-Debug no VS Code |

Todo o toolchain vem embutido no STM32CubeIDE — nada foi instalado à parte.
Os caminhos exatos estão no [CLAUDE.md](CLAUDE.md).

## O que muda em relação ao curso

A F446RE é quase drop-in em cima da F401RE:

- **Iguais:** LD2 em `PA5`, botão B1 em `PC13`, VCP em `USART2` (PA2/PA3),
  mesma flag `-DSTM32F4`, mesma `libopencm3_stm32f4`. O Makefile do curso
  não muda.
- **Diferente:** RAM de **96K → 128K** no `linkerscript.ld`. Dá pra
  confirmar na placa: o MSP inicial tem que ler `0x20020000`.
- **Diferente:** gravação por ST-Link em vez de J-Link.

## Compilar e gravar

Pelo VS Code (`Ctrl+Shift+P` → *Tasks: Run Task*):

| Task | O que faz |
|---|---|
| `build` | compila `app/firmware.elf` (também no `Ctrl+Shift+B`) |
| `flash` | compila e grava na placa via OpenOCD |
| `clean` | limpa os artefatos |
| `libopencm3: build` | recompila a biblioteca (só na primeira vez) |

`F5` compila, grava e entra em debug parado no `main`.

Clonando do zero:

```sh
git clone --recursive https://github.com/aluisioalves123/bare-metal-stm32f446re.git
```

## Progresso

| Ep | Tema | Status |
|---|---|:---:|
| 1 | RCC via PLL + blinky | ✅ |
| 2 | SysTick | ✅ |
| 3 | Timer / PWM | ✅ |
| 4 | Separação bootloader / aplicação | ⬜ |
| 5 | Driver de UART | ⬜ |
| 6 | Ring buffer | ⬜ |
| 7 | Protocolo de pacotes | ⬜ |
| 8 | API de flash | ⬜ |
| 9 | API de timer | ⬜ |
| 10 | Firmware update no bootloader | ⬜ |
| 11 | Aplicação de firmware update | ⬜ |
| 12 | Validação da aplicação | ⬜ |
| 14 | Firmware assinado com CBC-MAC | ⬜ |

---

## Diário de bordo

### Episódio 1 — RCC via PLL e blinky

Primeiro firmware: configurar o clock e piscar o LD2.

```c
rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
rcc_periph_clock_enable(RCC_GPIOA);
gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
```

**O que ficou claro:**

- **Periférico sem clock não existe.** O `rcc_periph_clock_enable(RCC_GPIOA)`
  não é burocracia: sem ele, escrever no GPIOA simplesmente não faz nada.
  Cada periférico tem seu bit de clock que precisa ser ligado antes.

- **O linker script e o startup code são um contrato.** O
  `linkerscript.ld` define os símbolos `_data_loadaddr`, `_data`, `_edata`,
  `_ebss`; o `reset_handler` do libopencm3 usa exatamente esses símbolos pra
  copiar o `.data` da flash pra RAM e zerar o `.bss` antes de chamar o
  `main`. Num PC quem faz isso é o sistema operacional — aqui alguém tem que
  escrever.

- **Sem delay, o LED não pisca — ele fica fraco.** Um `gpio_toggle` num
  `while(1)` a 84 MHz alterna o pino a alguns MHz. O olho integra e enxerga
  um LED aceso pela metade. A `delay_cycles` existe pra trazer isso pra
  frequência visível.

- **A libopencm3 é fina de propósito.** `gpio_toggle` são duas linhas:
  lê o `ODR`, escreve no `BSRR`. Dá pra abrir a fonte e entender em segundos
  — o que não acontece com a HAL da ST.

**Resultado:** 1812 bytes (1800 text + 12 data), gravado por SWD, verify OK.

### Episódio 2 — SysTick: trocando espera ocupada por interrupção

O blink do episódio 1 prendia a CPU num laço de `nop`. Agora um timer de
hardware avisa a cada 1 ms e o `main` só consulta um contador — o LED pisca
a cada 1,5 s.

```c
volatile uint64_t ticks = 0;

void sys_tick_handler(void) {
  ticks++;
}
```

**O que ficou claro:**

- **Saí de _nenhuma_ interrupção para interrupções.** No episódio 1 a CPU
  ficava 100% ocupada contando `nop` — não dava pra fazer mais nada enquanto
  esperava. O SysTick dispara uma exceção 1000×/s, o processador larga o que
  estava fazendo, incrementa o contador e volta. É isso que vai permitir,
  mais pra frente, piscar o LED **e** atender a UART ao mesmo tempo.

- **`volatile` não protege o handler, protege o laço.** O `sys_tick_handler`
  nunca seria descartado: ele é referenciado pela tabela de vetores. O risco
  real está no `while` do `main` — nada dentro dele escreve em `ticks`, então
  o compilador leria a variável uma vez, guardaria num registrador e
  compararia sempre o mesmo valor. O LED nunca piscaria. `volatile` obriga a
  reler da memória a cada acesso.

- **Os 8 bytes de `.bss` são o `ticks`.** E quem zera isso antes do `main` é o
  `reset_handler` — o mesmo contrato com o linker script que apareceu no
  episódio 1, agora com uma consequência visível.

**Adaptação da placa:** troquei `RCC_CLOCK_3V3_84MHZ` por
`RCC_CLOCK_3V3_180MHZ`, já que a F446 vai bem além dos 84 MHz da F401 usada
no curso.

> ⚠️ **Pendência conhecida:** o `rcc_clock_setup_pll` da libopencm3 configura
> VOS scale, PLL e 5 wait states de flash, mas **não habilita o overdrive
> mode** — os bits `ODEN`/`ODSWEN` do `PWR_CR` não são escritos em lugar
> nenhum da biblioteca. O reference manual do F446 exige overdrive acima de
> **168 MHz**, então o chip está rodando fora de especificação. Funciona na
> bancada, mas não é garantido em temperatura ou tensão de canto. A resolver:
> ou cair para `RCC_CLOCK_3V3_168MHZ`, ou habilitar o overdrive na mão.

**Resultado:** 1976 text + 12 data + 8 bss.

### Episódio 3 — Timer / PWM: sinaleira de carro

O exercício do episódio, construído em cima da base de SysTick do episódio 2:
um conjunto de setas com pisca-alerta, fechando com um farol de trabalho em
PWM.

#### Setas e pisca-alerta

**Hardware:** 3 botões (direita, esquerda, pisca-alerta) em portas diferentes,
todos com pull-up interno e aterrados pelo botão — ativos em baixo. 2 LEDs
como faróis.

**Comportamento.** Os LEDs ficam acesos em repouso, como luz de posição. Um
aperto liga o modo e ele fica travado; apertar o mesmo botão de novo desliga.
Qualquer outro botão troca de modo direto.

| Estado | Esquerdo | Direito |
|---|---|---|
| `SIGNAL_OFF` | aceso | aceso |
| `SIGNAL_RIGHT` | aceso | pisca |
| `SIGNAL_LEFT` | pisca | aceso |
| `SIGNAL_HAZARD` | pisca | pisca |

**O que ficou claro:**

- **Debounce e borda resolvem problemas diferentes, e os dois são
  necessários.** O debounce (20 scans seguidos) filtra o repique mecânico do
  contato. A detecção de borda garante que um aperto vale um comando, por mais
  tempo que o dedo fique no botão — sem ela, segurar meio segundo dispararia
  ~500 trocas de estado.

- **Borda de subida no software é borda de descida no fio.** Com pull-up, o
  pino cai de 3,3 V para GND quando o botão fecha. O `gpio_get(...) == 0` do
  `read_buttons` inverte isso, e dali pra dentro do programa apertar é uma
  transição `false → true`. A polaridade do hardware morre numa linha só: se um
  dia o botão virar pull-down, muda o `== 0` para `!= 0` e mais nada no
  programa fica sabendo.

- **Núcleo puro, casca imperativa.** As únicas funções que tocam hardware são
  `read_buttons` e `blink_leds`, e elas só traduzem — pino para `bool` e
  `bool` para pino. Toda a decisão (`next_debounce`, `debounced_buttons`,
  `next_signal_state`, `which_leds_blink`) é pura: recebe tudo por parâmetro e
  devolve o resultado. O estado que sobrevive entre iterações mora visível no
  `main`, não escondido em globais.

  A prova mais direta disso é a `.bss`: **8 bytes**, que são só o contador do
  SysTick. Nenhuma outra variável precisa de memória estática.

- **O `main` virou o roteiro.** Dá pra ler o programa de cima pra baixo, cada
  linha alimentando a próxima, sem caçar global pelo arquivo.

**Resultado:** 2416 text + 12 data + 8 bss.

#### Estrutura do código

Separado por camada — o que toca hardware de um
lado, o que é decisão pura do outro:

```
app/
├── inc/
│   ├── board.h              mapa do hardware: o único arquivo com pinos
│   ├── hal/                 systick.h  buttons.h  lamps.h
│   └── logic/               turn_signal.h
└── src/
    ├── main.c               a casca: setup e o laço
    ├── hal/                 systick.c  buttons.c  lamps.c
    └── logic/               turn_signal.c
```

O `main.c` não inclui `board.h`: ele não sabe que existe PA5 nem pull-up.
Quando a fiação virar PCB, só o `board.h` muda.

#### Farol de trabalho com rampa PWM

Ligar um farol de trabalho de estalo incomoda a vista e dá pico de corrente. O
requisito é uma rampa de 400 ms — o que significa PWM com o valor de comparação
subindo aos poucos, em vez de um pino ligando de uma vez.

Botão B1 (PC13) liga e desliga. TIM2 canal 3 sai no PB10 em AF1, com
`PSC = 224` e `ARR = 399`: a 180 MHz isso dá 2 kHz de PWM. A cada systick o
`CCR3` anda um passo, para cima ou para baixo, então percorrer os 400 níveis
leva exatamente os 400 ms pedidos — nos dois sentidos.

```c
uint32_t ccr = TIM_CCR3(TIM2);

if (service_light_state == SERVICE_LIGHT_ON) {
  if (ccr < TIM_ARR(TIM2)) { timer_set_oc_value(TIM2, TIM_OC3, ccr + 1); }
} else {
  if (ccr > 0)              { timer_set_oc_value(TIM2, TIM_OC3, ccr - 1); }
}
```

**A restrição que dá o tom:** a rampa não pode travar a seta. As duas coisas
acontecem ao mesmo tempo, cada uma na sua máquina de estados, alimentadas pela
mesma base de tempo do SysTick. Nada de `delay` — se houvesse um, os 400 ms de
rampa engoliriam duas piscadas.

**O que ficou claro:**

- **Rampa é estado, não espera.** A tentação é escrever um laço que sobe o duty
  e dorme entre os passos. Isso funcionaria, e mataria o pisca-alerta. Guardar a
  posição da rampa e avançar um passo por tick é o que deixa as duas
  funcionalidades coexistirem num único fluxo.

- **O `CCR` é o próprio contador da rampa.** Não precisei de variável para
  guardar em que ponto a rampa está: o valor já vive no registrador do
  periférico, e o `ARR` também. Um estado a menos para sincronizar.

- **Cada módulo habilita o que usa, por completo.** O `service_light_setup()`
  liga o clock do GPIOB mesmo o `buttons_setup()` já tendo ligado. Repetir custa
  um `|=`; depender em silêncio de outro módulo custa uma tarde de depuração
  quando alguém mexer nos botões.

- **`timer_enable_oc_preload` evita pulso torto.** Sem ele, escrever no `CCR` no
  meio de um ciclo trunca o pulso em curso. Com preload, o valor novo só entra
  em vigor no próximo update do timer.

**Resultado:** 3784 text + 12 data + 8 bss. Farol acendendo e apagando
suavemente enquanto a seta pisca sem hesitar.

**Próximo passo:** episódio 4 do curso — separação entre bootloader e
aplicação.
