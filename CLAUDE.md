# Bare Metal Programming Series — STM32F446RE

Acompanhamento da playlist [Bare Metal Programming Series](https://github.com/lowbyteproductions/bare-metal-series)
da Low Byte Productions, adaptada da **Nucleo-F401RE** (usada no curso) para a
**Nucleo-F446RE**, com o **ST-Link on-board** no lugar do J-Link que ele usa.

## Regra de ouro deste ambiente

**Não instale nada sem perguntar.** Não existe toolchain ARM no PATH desta máquina —
tudo vem embutido no **STM32CubeIDE 2.2.0**. Antes de sugerir `winget`/`choco`,
procure em `C:\ST`, `C:\Program Files*` e `%LOCALAPPDATA%\Programs`.

## Toolchain (caminhos reais)

Raiz: `C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\`

| Ferramenta | Subpasta do plugin |
|---|---|
| GCC 14.3.1 + gdb | `...externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740\tools\bin` |
| make 4.4.1 | `...externaltools.make.win32_2.2.200.202604021615\tools\bin` |
| OpenOCD 0.12 (ST) | `...externaltools.openocd.win32_2.4.500.202604080855\tools\bin` |
| Scripts do OpenOCD | `...debug.openocd_2.3.400.202606220929\resources\openocd\st_scripts` |
| SVD do F446 | `...productdb.debug_2.2.500.202605201745\resources\cmsis\STMicroelectronics_CMSIS_SVD\STM32F446.svd` |

Esses caminhos já estão gravados em `.vscode/tasks.json`, `.vscode/launch.json` e
`.vscode/settings.json`. **Se atualizar o CubeIDE, as versões nos nomes das pastas
mudam e os quatro arquivos quebram juntos.**

## Como buildar e gravar

Pelo VS Code: `Ctrl+Shift+B` (build), tasks `flash` e `clean`, `F5` para debug
(config *Debug (ST-Link on-board)*).

Pela linha de comando, os tasks usam **Git Bash**, não PowerShell:

```sh
export PATH="$CUBE_GCC:$CUBE_MAKE:$PATH"
make -C app                 # gera app/firmware.elf
```

## Armadilhas já resolvidas — não redescubra

1. **`python3` do PATH é o stub falso da Microsoft Store** e retorna erro 9009. O
   Python real é o standalone do uv em
   `%USERPROFILE%\AppData\Roaming\uv\python\cpython-3.11-windows-x86_64-none\`,
   que só tem `python.exe`, sem `python3.exe`. Como o gerador de `nvic.h` do
   libopencm3 tem shebang `#!/usr/bin/env python3`, existe um shim em `tools/bin/python3`
   (gitignored, específico desta máquina). Dentro dele o caminho **precisa** ser
   estilo `C:/...` — o estilo `/c/...` falha quando invocado pelo make.
   Só é necessário ao recompilar o libopencm3.

2. **O OpenOCD da ST rejeita `interface/stlink.cfg`** (HLA, dá erro em `swj_newdap`).
   Use `interface/stlink-dap.cfg` + `-c "transport select dapdirect_swd"`.

3. **`board/st_nucleo_f4.cfg` não existe** nos scripts da ST. Use
   `interface/stlink-dap.cfg` + `target/stm32f4x.cfg`.

## F401RE → F446RE: o que muda

Idênticos nas duas placas: LD2=**PA5**, B1=**PC13**, VCP=**USART2** (PA2/PA3),
`-DSTM32F4` e `libopencm3_stm32f4`. O Makefile do curso **não muda**.

Muda só isto: **RAM de 96K para 128K** em `app/linkerscript.ld`. Confirmação na
placa: o MSP inicial tem que ler `0x20020000` (com 96K seria `0x20018000`).
Clock: o curso usa HSI a 84 MHz, que roda igual na F446 (máximo dela: 180 MHz).
Device id lido pelo OpenOCD: `0x10006421`, flash 512 KiB.

> **Atenção no Ep. 4**, quando ele divide a flash entre bootloader e aplicação: os
> endereços do vídeo são calculados sobre os 96K da F401RE.

## Repositório do curso: episódio = commit

A `main` de `lowbyteproductions/bare-metal-series` é o **estado final** (pós Ep. 14).
Não há tags nem branches por episódio. Para consultar: `git show <commit>:<arquivo>`.

| Ep | Commit | | Ep | Commit |
|---|---|---|---|---|
| 1 RCC + blinky | `afe8fca` | | 8 flash API | `cbec671` |
| 2 SysTick | `8515276` | | 9 timer API | `331be35` |
| 3 Timer/PWM | `29d3eef` | | 10 update no bootloader | `052a500` |
| 4 bootloader/app | `70ade42` | | 11 app do fw-updater | `c4ddb9e`, `25af337` |
| 5 UART | `e3bf673` | | 12 validade da app | `49815e4` |
| 6 ring buffer | `02e33ec` | | 14 CBC-MAC | `b5e9ea1` |
| 7 pacotes | `b4b111e` | | 7.3 base do fw-updater | `60949a1`, `1a93426` |

## Estado atual

Episódio 1 concluído: build, flash e debug validados na placa física.
