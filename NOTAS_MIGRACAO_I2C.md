# Notas — Migração da comunicação do display: SPI → I2C (IIC)

> Documento de retomada da conversa com o Claude (sessão de 2026-06-28).
> Objetivo: poder voltar a este trabalho noutro dia sem perder o contexto.

## Por que esta mudança

A **especificação de solução** do projeto previa comunicação **IIC (I2C)**, mas a
implementação original estava em **SPI**. Investigando, concluímos:

- O display original (**ILI9341**, TFT 320×240 colorido) **só suporta SPI ou
  paralelo** — não existe ILI9341 (nem similar colorido) em I2C. Por isso a
  implementação em SPI estava, na prática, **tecnicamente correta**, e a spec é
  que estava inviável para aquele hardware.
- O **datasheet do STM32F411xC/xE** confirma que o microcontrolador tem I2C
  (I2C1/2/3, Fast Mode 400 kHz) **e** SPI — ou seja, o MCU **nunca** foi o
  limitante. O datasheet que decide a viabilidade é o do **display**, não o do
  MCU.
- Argumento de banda: I2C (400 kHz) seria lento demais para um TFT colorido;
  SPI (8 Mbit/s no projeto) é o correto para display rápido. I2C é para sensores.

**Decisão:** para honrar o "IIC" da spec, trocamos o display por um **OLED
SSD1306 (128×64, monocromático, I2C, endereço 0x3C)** — o display I2C padrão e
bem suportado no STM32 HAL. (Não vamos rodar na placa; o alvo é **compilar limpo
no STM32CubeIDE**.)

## Decisões travadas na conversa

1. Display: **SSD1306** (128×64 I2C, addr 0x3C).
2. Escopo das telas: **adaptação completa** para 128×64 mono.
3. Implementação do I2C: **HAL_I2C** (padrão do projeto) + atualizar o `.ioc`.

## Descoberta que reduziu o trabalho

A camada gráfica e as fontes do projeto são **agnósticas de protocolo**: o motor
de texto/formas desenha só via primitivas `DrawPixel`/`DrawRectangle`/`DrawHLine`/
`DrawVLine`. Por isso **reaproveitamos o motor de texto e todas as fontes**
(`Core/Src/fonts.c`), reimplementando apenas as primitivas sobre um framebuffer
SSD1306. Não foi preciso escrever tabelas de fonte novas.

## O que foi alterado (estado atual do código)

### Periférico SPI1 → I2C1 (PB6=SCL, PB7=SDA, 400 kHz)
- `projeto_final_engd33.ioc` — removido SPI1 (PA5/6/7) e GPIOs TFT (PA2/3/4);
  habilitado I2C1 em PB6/PB7.
- `Core/Src/main.c` — `hspi1`→`hi2c1`; `MX_SPI1_Init`→`MX_I2C1_Init`; init dos
  pinos TFT removida do `MX_GPIO_Init`.
- `Core/Src/stm32f4xx_hal_msp.c` — `HAL_SPI_MspInit/DeInit` → `HAL_I2C_MspInit/
  DeInit` (PB6/PB7, AF4, open-drain, pull-up).
- `Core/Inc/main.h` — removidos os defines `TFT_*` (mantido `LED`/PC13).
- `Core/Inc/stm32f4xx_hal_conf.h` — `HAL_I2C_MODULE_ENABLED` habilitado.

### Driver novo do display (SSD1306 sobre HAL_I2C)
- `Core/Inc/ssd1306.h` + `Core/Src/ssd1306.c` — framebuffer 128×64, sequência de
  init, `SSD1306_UpdateScreen`, primitivas de desenho, e `DrawChar`/`DrawText`
  reaproveitando `fonts.c`. Transporte via `HAL_I2C_Master_Transmit` (comandos) e
  `HAL_I2C_Mem_Write` com control byte 0x40 (dados).

### Telas adaptadas (128×64) — lógica de RTOS intacta
- `Core/Src/maincode.c` — **tasks/filas/mutexes/geração/leitura permanecem iguais**;
  só as 3 telas foram reescritas (FONT1 8×12; título + valores), com flush via
  `SSD1306_UpdateScreen()`.
- `Core/Inc/maincode.h` — includes ajustados; `funcScaleXGraph` removido.

### Arquivos removidos (eram específicos de SPI/ILI9341)
- `Core/Src/ILI9341_STM32_Driver.c`, `Core/Inc/ILI9341_STM32_Driver.h`
- `Core/Src/ILI9341_GFX.c`, `Core/Inc/ILI9341_GFX.h`
- Mantidos: `Core/Src/fonts.c`, `Core/Inc/fonts.h`.

## ⚠️ Como compilar e validar (PENDENTE — fazer no CubeIDE)

O build **não foi validado** (não havia toolchain ARM no ambiente da sessão). A
corretude é "por construção". Passos:

1. Abrir `projeto_final_engd33.ioc` no STM32CubeIDE → **Project → Generate Code**.
   **Obrigatório:** é o que copia para o projeto os arquivos
   `stm32f4xx_hal_i2c.c/.h`, que ainda **não existem** (o CubeMX só copia o HAL
   dos periféricos habilitados). Sem este passo, o link falha.
2. **Project → Clean…** e depois **Build All** → meta: **0 errors**.
   (O Clean é importante para descartar os `.o` antigos do ILI9341 em `Debug/`.)
3. (Opcional, se um dia rodar na placa) analisador lógico/osciloscópio em
   **PB6 (SCL) / PB7 (SDA)** para capturar o endereço `0x3C` e o stream I2C —
   evidência objetiva da migração.

Esperado no passo 1: o CubeMX recria `hi2c1`/`MX_I2C1_Init`/MSP como já deixamos
(convergem) e desabilita sozinho o `HAL_SPI_MODULE_ENABLED` remanescente.

## Próximos passos possíveis (não feitos ainda)

- Validar o build no CubeIDE (passos acima).
- Escrever o trecho do relatório documentando a migração (justificativa SPI→I2C
  + troca de display, usando os dois datasheets).
- Ajustar layout/fonte das telas se quiser melhor aproveitamento do 128×64.

## Referência

Plano detalhado da implementação (gerado nesta sessão):
`~/.claude/plans/sprightly-meandering-church.md`
