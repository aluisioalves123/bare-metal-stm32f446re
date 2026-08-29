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
| 3 | Timer / PWM | ⬜ |
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
