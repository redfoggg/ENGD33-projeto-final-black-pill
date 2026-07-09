# ENGD33 — Projeto Final Black Pill

Projeto final da disciplina **Programação em Tempo Real para Sistemas Embarcados (ENGD33)**, desenvolvido para a placa **Black Pill STM32F411CEU6** com **FreeRTOS** e display **OLED SSD1306 128x64 via I2C**.

O projeto implementa o **módulo de interface via display** de uma base robótica móvel omnidirecional. O firmware simula dados de corrente, velocidade angular e posição, processa essas informações em tarefas concorrentes do FreeRTOS e apresenta os valores em três telas no OLED.

## Objetivo do projeto

O objetivo é desenvolver e validar um módulo embarcado de interface capaz de:

- simular dados de sensores por meio de tabelas de valores;
- organizar os dados em estruturas comuns;
- usar tasks periódicas do FreeRTOS para geração e leitura de dados;
- comunicar tasks por filas;
- proteger variáveis globais com mutexes;
- calcular velocidades lineares a partir das velocidades angulares das rodas;
- exibir informações em um display OLED SSD1306;
- alternar telas por botão em PA0/EXTI0;
- validar o funcionamento em simulação com Renode.

## Funcionalidades principais

O display possui três telas principais:

1. **SCREEN1 — Velocidade linear e posição**
   - `Vx`
   - `Vy`
   - `Px`
   - `Py`

2. **SCREEN2 — Velocidades angulares**
   - `M1`
   - `M2`
   - `M3`
   - `W`

3. **SCREEN3 — Correntes dos motores**
   - `Ix`
   - `Iy`
   - `Iz`

A troca de telas é feita pelo botão conectado ao pino **PA0**, tratado por interrupção externa **EXTI0**. Em simulação, também é possível trocar a tela pela UART2 enviando o comando `n`.

## Hardware alvo

- Placa: **Black Pill STM32F411CEU6**
- Display: **OLED SSD1306 128x64 monocromático**
- Comunicação com display: **I2C1**
- Endereço I2C do OLED: `0x3C`
- Botão de troca de telas: **PA0 / EXTI0**
- LED de debug: **PC13**
- UART de debug: **USART2 / UART2**

## Organização das pastas

```text
ENGD33-projeto-final-black-pill/
├── Core/
│   ├── Inc/                 # Headers do projeto
│   └── Src/                 # Código-fonte principal
├── Drivers/                 # Drivers HAL/CMSIS da STM32
├── Middlewares/             # FreeRTOS
├── Scripts/                 # Script de geração da lookuptable
├── scripts/                 # Scripts auxiliares de simulação/renderização
├── renode/                  # Arquivos de simulação do Renode
├── simulation_logs/         # Logs e imagens geradas nas simulações
├── relatorio/               # Relatório do projeto em LaTeX
├── Debug/                   # Arquivos gerados pela compilação no STM32CubeIDE
├── projeto_final_engd33.ioc # Configuração STM32CubeMX/STM32CubeIDE
└── README.md
```

## Relatório

O relatório do projeto está na pasta:

```text
relatorio/
```

O arquivo principal do relatório é:

```text
relatorio/main.tex
```
Na mesma pasta está o resultado em pdf da compilação. "Relatorio_TempoReal.pdf"

## Arquivos principais do firmware

### `Core/Src/maincode.c`

Contém a maior parte da lógica da aplicação:

- criação das filas e mutexes;
- criação das tasks do FreeRTOS;
- geração dos dados simulados;
- leitura das filas;
- cálculo das velocidades lineares;
- gerenciamento das telas;
- callback do botão PA0/EXTI0;
- comandos auxiliares via UART2;
- monitoramento de temporização das tasks.

### `Core/Inc/maincode.h`

Contém definições, protótipos e constantes utilizadas pelo módulo principal, incluindo o tempo nominal de atualização da tela:

```c
#define REFRESH_SCREEN 699
```

### `Core/Src/lookuptable.c`

Contém as tabelas de valores utilizadas para simular os dados de entrada do sistema:

- `currentVectorX`, `currentVectorY`, `currentVectorZ`
- `angularVelVectorX`, `angularVelVectorY`, `angularVelVectorZ`
- `positionVectorX`, `positionVectorY`, `positionVectorZ`

### `Core/Src/ssd1306.c`

Contém o driver do display OLED SSD1306, incluindo o framebuffer em RAM e as funções de atualização via I2C.

## FreeRTOS

O firmware utiliza FreeRTOS para organizar a aplicação em tarefas concorrentes.

As principais tasks são:

| Task | Função | Período aproximado |
|---|---|---:|
| `vTaskGenerateCurrentQueue` | Gera dados simulados de corrente | 1 ms |
| `vTaskGenerateAngularVelQueue` | Gera velocidades angulares simuladas | 10 ms |
| `vTaskGeneratePositionQueue` | Gera dados simulados de posição | 100 ms |
| `vTaskQueueCurrentReader` | Lê a fila de corrente | 25 ms |
| `vTaskQueueAngularVelReader` | Lê velocidade angular e calcula `Vx`, `Vy`, `W` | 100 ms |
| `vTaskQueuePositionReader` | Lê a fila de posição | 125 ms |
| `vDisplayManager` | Gerencia telas e atualiza o OLED | `REFRESH_SCREEN` + tempo de atualização |
| `vTaskSimCommandReader` | Recebe comandos via UART2 no Renode | auxiliar |
| `vTaskTimingMonitor` | Imprime contadores de temporização | 10 s |

## Comunicação entre tasks

A comunicação entre produtoras e leitoras é feita por filas do FreeRTOS:

```c
xQueueCurrent
xQueueAngularVel
xQueuePosition
```

As variáveis globais usadas pelo display são protegidas por mutexes:

```c
xMutexCurrentCurrent
xMutexCurrentLinearVel
xMutexCurrentAngularVel
xMutexCurrentPosition
```

## Simulação com Renode

A pasta `renode/` contém os arquivos necessários para executar o firmware em simulação.

Arquivos principais:

```text
renode/blackpill_f411_oled.repl
renode/parte2_load.resc
renode/parte3_i2c_oled.resc
renode/dump_oled_buffer.py
renode/oled_logger.py
```

Para executar a simulação principal:

```bash
renode --console -e "i @renode/parte3_i2c_oled.resc"
```

Dentro do monitor do Renode:

```renode
start
```

Para observar a UART2 em outro terminal:

```bash
nc localhost 12345
```

## Comandos úteis na UART2

Durante a simulação, a UART2 é usada apenas para debug. Ela não substitui o display OLED.

Comando para trocar tela:

```text
n
```

Fluxo esperado:

```text
SCREEN1 -> SCREEN2 -> SCREEN3 -> SCREEN1
```

Também existe suporte auxiliar para envio de dados por UART no formato:

```text
P,x,y,z
A,x,y,z
C,x,y,z
```

No projeto atual, a macro abaixo está configurada para usar a `lookuptable.c` como fonte dos dados:

```c
#define USE_UART_SIM_DATA 0
```

## Testes realizados

Foram realizadas simulações para validar diferentes partes do projeto:

1. **Inicialização do FreeRTOS**
   - Confirmação da criação das tasks, filas e mutexes.

2. **Troca de telas via UART2**
   - Envio do comando `n` para alternar entre SCREEN1, SCREEN2 e SCREEN3.

3. **Botão virtual PA0/EXTI0**
   - Teste do fluxo botão → interrupção → callback → notificação FreeRTOS → troca de tela.

4. **Comunicação I2C/OLED**
   - Validação de escritas I2C para o OLED simulado no endereço `0x3C`.

5. **Captura do framebuffer SSD1306**
   - Leitura dos 1024 bytes do framebuffer e renderização em imagens `.png`.

6. **Temporização das tasks**
   - Medição das execuções das tasks em janelas de 10 segundos.

## Resultados de temporização

No teste de temporização, foram observados resultados próximos aos períodos definidos:

```text
TASK TIMING | janela=10000 ms
GEN  | corrente=10006 | velAngular=1001 | posicao=100
READ | corrente=400 | velAngular=100 | posicao=81
DISP | display=5

TASK TIMING | janela=10000 ms
GEN  | corrente=10005 | velAngular=1001 | posicao=100
READ | corrente=401 | velAngular=100 | posicao=80
DISP | display=4
```

As tasks geradoras e leitoras mantiveram contagens compatíveis com seus períodos. A task de display apresentou menor taxa efetiva porque sua atualização inclui escrita no framebuffer, envio do conteúdo pelo driver SSD1306/I2C e mensagens de debug pela UART.

## Logs e imagens das simulações

Os resultados das simulações ficam na pasta:

```text
simulation_logs/
```

Arquivos relevantes:

```text
simulation_logs/teste_botao_press_release_renode_atualizado.txt
simulation_logs/teste_i2c_oled_renode.txt
simulation_logs/renode_uart_test_20260709_024520.txt
simulation_logs/oled_framebuffer.bin
simulation_logs/oled_framebuffer.png
simulation_logs/screen1.png
simulation_logs/screen2.png
simulation_logs/screen3.png
```

As imagens `screen1.png`, `screen2.png` e `screen3.png` foram geradas a partir do framebuffer real do driver SSD1306.

## Renderização do framebuffer

O script abaixo converte o framebuffer binário do SSD1306 em uma imagem `.png`:

```text
scripts/render_ssd1306_buffer.py
```

Exemplo de uso:

```bash
python3 scripts/render_ssd1306_buffer.py simulation_logs/screen1.bin simulation_logs/screen1.png
```

## Como abrir e compilar o firmware

1. Instale o **STM32CubeIDE**.
2. Abra o STM32CubeIDE.
3. Importe o projeto como projeto existente.
4. Selecione a pasta raiz `ENGD33-projeto-final-black-pill/`.
5. Use a configuração **Debug**.
6. Execute **Clean Project** e depois **Build Project**.

O arquivo ELF gerado fica em:

```text
Debug/projeto_final_engd33.elf
```

Esse arquivo é carregado pelos scripts do Renode.

## Observações importantes

- A UART2 é usada apenas como instrumento de debug/simulação.
- A interface principal do projeto é o display OLED SSD1306 via I2C.
- A troca de telas prevista para o hardware físico é feita pelo botão em PA0/EXTI0.
- Os dados de sensores são simulados por `lookuptable.c`.
- O projeto valida o módulo de interface, não o controle completo da base robótica.

## Autores

- David Ferrari
- João Cerqueira
- Vinícius Jesus
- Felipe Cunha

Universidade Federal da Bahia — Campus Salvador  
Disciplina: Programação em Tempo Real para Sistemas Embarcados — ENGD33  
Julho de 2026
