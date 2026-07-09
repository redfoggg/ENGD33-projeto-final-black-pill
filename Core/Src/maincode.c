#include "maincode.h"
#include "simio.h"
#include <stdio.h>
#include <stdlib.h>

#define USE_UART_SIM_DATA 0

extern UART_HandleTypeDef huart2;

/* ================DEFINES E VARIÁVEIS DE CONTROLE================ */

uint16_t sCurrentScreen = SCREEN1;

__attribute__((used)) volatile uint32_t lastButtonPressTick = 0;
__attribute__((used)) volatile uint32_t button_callback_count = 0;
__attribute__((used)) volatile uint32_t button_notify_count = 0;

/* =============ESTRUTURAS DE ARMAZENAMENTO DE DADOS============== */

dataset xCurrentLinearVel;
dataset xCurrentPosition;
dataset xCurrentAngularVel;
dataset xCurrentCurrent;

/* ===========================HANDLERS============================ */

TaskHandle_t xHandlerDisplayManager = NULL;

QueueHandle_t xQueueCurrent = NULL;
QueueHandle_t xQueueAngularVel = NULL;
QueueHandle_t xQueuePosition = NULL;

SemaphoreHandle_t xMutexCurrentCurrent = NULL;
SemaphoreHandle_t xMutexCurrentLinearVel = NULL;
SemaphoreHandle_t xMutexCurrentAngularVel = NULL;
SemaphoreHandle_t xMutexCurrentPosition = NULL;

/* =====================INICIALIZAÇÃO DO RTOS===================== */

void userRTOS(void) {
    BaseType_t ret;

    xMutexCurrentCurrent = xSemaphoreCreateMutex();
    xMutexCurrentLinearVel = xSemaphoreCreateMutex();
    xMutexCurrentAngularVel = xSemaphoreCreateMutex();
    xMutexCurrentPosition = xSemaphoreCreateMutex();

    xQueueCurrent = xQueueCreate(50, sizeof(dataset));
    xQueueAngularVel = xQueueCreate(30, sizeof(dataset));
    xQueuePosition = xQueueCreate(5, sizeof(dataset));

    sim_printf("INIT RTOS | heap before tasks=%lu\r\n", xPortGetFreeHeapSize());

    sim_printf("BUTTON DEBUG | cb=%lu | notify=%lu | last=%lu\r\n",
               button_callback_count,
               button_notify_count,
               lastButtonPressTick);

    ret = xTaskCreate(vDisplayManager, "displayManager", 2048, (void*) 0, 1, &xHandlerDisplayManager);
    sim_printf("CREATE displayManager=%ld | heap=%lu\r\n", ret, xPortGetFreeHeapSize());

#if USE_UART_SIM_DATA == 0

    ret = xTaskCreate(vTaskGenerateCurrentQueue, "gerarQueueCorrente", 128, (void*) 0, 5, NULL);
    sim_printf("CREATE gerarQueueCorrente=%ld | heap=%lu\r\n", ret, xPortGetFreeHeapSize());

    ret = xTaskCreate(vTaskGenerateAngularVelQueue, "gerarQueueVelAngular", 128, (void*) 0, 4, NULL);
    sim_printf("CREATE gerarQueueVelAngular=%ld | heap=%lu\r\n", ret, xPortGetFreeHeapSize());

    ret = xTaskCreate(vTaskGeneratePositionQueue, "gerarQueuePosicao", 128, (void*) 0, 3, NULL);
    sim_printf("CREATE gerarQueuePosicao=%ld | heap=%lu\r\n", ret, xPortGetFreeHeapSize());

#else

    sim_printf("SKIP gerarQueueCorrente | USE_UART_SIM_DATA=1\r\n");
    sim_printf("SKIP gerarQueueVelAngular | USE_UART_SIM_DATA=1\r\n");
    sim_printf("SKIP gerarQueuePosicao | USE_UART_SIM_DATA=1\r\n");
    sim_printf("SIM MODE | dados recebidos pela UART2\r\n");

#endif

    ret = xTaskCreate(vTaskQueueCurrentReader, "queueCorrenteReader", 256, (void*) 0, 2, NULL);
    sim_printf("CREATE queueCorrenteReader=%ld | heap=%lu\r\n", ret, xPortGetFreeHeapSize());

    ret = xTaskCreate(vTaskQueueAngularVelReader, "queueVelAngularReader", 256, (void*) 0, 2, NULL);
    sim_printf("CREATE queueVelAngularReader=%ld | heap=%lu\r\n", ret, xPortGetFreeHeapSize());

    ret = xTaskCreate(vTaskQueuePositionReader, "queuePosicaoReader", 256, (void*) 0, 2, NULL);
    sim_printf("CREATE queuePosicaoReader=%ld | heap=%lu\r\n", ret, xPortGetFreeHeapSize());

#if DEBUG_RENODE
    ret = xTaskCreate(vTaskSimCommandReader, "simCommandReader", 256, (void*) 0, 2, NULL);
    sim_printf("CREATE simCommandReader=%ld | heap=%lu\r\n", ret, xPortGetFreeHeapSize());
#endif
}

/* =====================CALLBACKS DE INTERRUPÇÃO===================== */


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        button_callback_count++;

        uint32_t now = HAL_GetTick();

        if (now - lastButtonPressTick > 200)
        {
            lastButtonPressTick = now;
            button_notify_count++;

            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveFromISR(xHandlerDisplayManager, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

/* =========================TASKS DO RTOS========================= */

// Gerenciamento da tela
void vDisplayManager(void *p){
	baseScreen(sCurrentScreen);
	while(1){
		uint32_t ulNotified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(REFRESH_SCREEN));
		if(ulNotified > 0){
			switch(sCurrentScreen) {
    		    case SCREEN1:
    		        sCurrentScreen = SCREEN2;
    		        break;
    		    case SCREEN2:
    		        sCurrentScreen = SCREEN3;
    		        break;
    		    case SCREEN3:
    		       	sCurrentScreen = SCREEN1;
    		        break;
    		    default:
    		        break;
    		}
			baseScreen(sCurrentScreen);
		}
		dataScreen(sCurrentScreen);
	}
}

// Geração de dados de current e envio para queue
void vTaskGenerateCurrentQueue(void *p) {
	TickType_t xLastWakeTime;
	dataset currents;
	uint16_t sIndex = 0;
	while(1) {
		xLastWakeTime = xTaskGetTickCount();
		currents.x = vectorX[sIndex];
		currents.y = vectorY[sIndex];
		currents.z = vectorZ[sIndex];
		currents.timestamp = xLastWakeTime;
		if(sIndex >= LENGTH_LUT - 1){
			sIndex = 0;
		}else{
			sIndex++;
		}
		if(xQueueSendToBack(xQueueCurrent, &currents, 0) == errQUEUE_FULL){
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 0);
		}
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
	}
}


void vTaskGenerateAngularVelQueue(void *p) { //geração de dados de velocidade angular e envio para queue
	TickType_t xLastWakeTime;
	dataset angularVel;
	uint16_t sIndex = 0;
	while(1) {
		xLastWakeTime = xTaskGetTickCount();
		angularVel.x = AngVelocityX[sIndex];
		angularVel.y = AngVelocityY[sIndex];
		angularVel.z = AngVelocityZ[sIndex];
		angularVel.timestamp = xLastWakeTime;
		if(sIndex >= LENGTH_LUT - 1){
			sIndex = 0;
		}else{
			sIndex++;
		}
		if(xQueueSendToBack(xQueueAngularVel, &angularVel, 0) == errQUEUE_FULL){
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 0);
		}
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
	}
}

// Geração de dados de GPS e envio para queue
void vTaskGeneratePositionQueue(void *p) {
	TickType_t xLastWakeTime;
	dataset position;
	uint16_t sIndex = 0;
	while(1) {
		xLastWakeTime = xTaskGetTickCount();
		position.x = VecPositionX[sIndex];
		position.y = VecPositionY[sIndex];
		position.z = VecPositionZ[sIndex];
		position.timestamp = xLastWakeTime;
		if(sIndex >= LENGTH_LUT - 1){
			sIndex = 0;
		}else{
			sIndex++;
		}
		if(xQueueSendToBack(xQueuePosition, &position, 0) == errQUEUE_FULL){
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 0);
		}
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
	}
}

// Leitura de dados de current da queue
void vTaskQueueCurrentReader(void *p) {
	const TickType_t xMaxMutexDelay = pdMS_TO_TICKS(1);
	TickType_t xLastWakeTime;
	dataset current;
	while (1) {
		xLastWakeTime = xTaskGetTickCount();
		while(xQueueReceive(xQueueCurrent, &current, 0) != errQUEUE_EMPTY){
			if(xSemaphoreTake(xMutexCurrentCurrent, xMaxMutexDelay) == pdPASS){
				xCurrentCurrent = current;
				xSemaphoreGive(xMutexCurrentCurrent);
			}
		}
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(25));
	}
}

// Leitura de dados de velocidade angular da queue
void vTaskQueueAngularVelReader(void *p) {
	const TickType_t xMaxMutexDelay = pdMS_TO_TICKS(1);
	const uint16_t L_cm = 20;
	const uint16_t r_cm = 15;
	const float alpha1 = 0;
	const float alpha2 = 2*M_PI/3;
	const float alpha3 = 4*M_PI/3;
	const float sin_alpha1 = sin(alpha1);
	const float sin_alpha2 = sin(alpha2);
	const float sin_alpha3 = sin(alpha3);
	const float cos_alpha1 = cos(alpha1);
	const float cos_alpha2 = cos(alpha2);
	const float cos_alpha3 = cos(alpha3);
	TickType_t xLastWakeTime;
	float linearVX, linearVY, linearW;
	dataset angularVel, linearVel;
	while (1) {
		xLastWakeTime = xTaskGetTickCount();

		while(xQueueReceive(xQueueAngularVel, &angularVel, 0) != errQUEUE_EMPTY) {
			linearVX = r_cm*(2.0/3.0)*(-(sin_alpha1*angularVel.x)-(sin_alpha2*angularVel.y)-(sin_alpha3*angularVel.z));
			linearVY = r_cm*(2.0/3.0)*((cos_alpha1*angularVel.x)+(cos_alpha2*angularVel.y)+(cos_alpha3*angularVel.z));
			linearW = (r_cm*(angularVel.x+angularVel.y+angularVel.z))/(3*L_cm);
			linearVel.x = linearVX;
			linearVel.y = linearVY;
			linearVel.z = linearW;

			if(xSemaphoreTake(xMutexCurrentLinearVel, xMaxMutexDelay) == pdPASS){
				xCurrentLinearVel = linearVel;
				xSemaphoreGive(xMutexCurrentLinearVel);
			}

			if(xSemaphoreTake(xMutexCurrentAngularVel, xMaxMutexDelay) == pdPASS){
				xCurrentAngularVel = angularVel;
				xSemaphoreGive(xMutexCurrentAngularVel);
			}
		}

		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
	}
}

// Leitura de dados de GPS da queue
void vTaskQueuePositionReader(void *p) {
	const TickType_t xMaxMutexDelay = pdMS_TO_TICKS(1);
	TickType_t xLastWakeTime;
	dataset position;
	while (1) {
		xLastWakeTime = xTaskGetTickCount();
		while(xQueueReceive(xQueuePosition, &position, 0) != errQUEUE_EMPTY){
			if(xSemaphoreTake(xMutexCurrentPosition, xMaxMutexDelay) == pdPASS){
				xCurrentPosition = position;
				xSemaphoreGive(xMutexCurrentPosition);
			}
		}
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(125));
	}
}

/* ======================FUNÇÕES AUXILIARES======================= */

/* Layout para o OLED SSD1306 (128x64, monocromatico).
 * Titulo na linha y=0; valores nas linhas y=14/26/38/50 usando FONT1 (8x12). */
#define TITLE_Y	0
#define LINE1_Y	14
#define LINE2_Y	26
#define LINE3_Y	38
#define LINE4_Y	50

// Inicialização da tela executada antes da inicialização do RTOS
void initialize(void){
	SSD1306_Init();
	baseScreen(sCurrentScreen);
}

// Base da Tela 1
void funcBaseScreen1(void){
	SSD1306_DrawText("Vel Lin / Pos", FONT1, 0, TITLE_Y, SSD1306_WHITE, SSD1306_BLACK);
}

// Base da Tela 2
void funcBaseScreen2(void){
	SSD1306_DrawText("Vel Angular", FONT1, 0, TITLE_Y, SSD1306_WHITE, SSD1306_BLACK);
}

// Base da Tela 3
void funcBaseScreen3(void){
	SSD1306_DrawText("Corrente", FONT1, 0, TITLE_Y, SSD1306_WHITE, SSD1306_BLACK);
}

// Seleção de base de tela
void baseScreen(uint16_t sScreenNum){
	SSD1306_Fill(SSD1306_BLACK);
	switch(sScreenNum){
		case SCREEN1:
			funcBaseScreen1();
			break;
		case SCREEN2:
			funcBaseScreen2();
			break;
		case SCREEN3:
			funcBaseScreen3();
			break;
		default:
			break;
	}
	SSD1306_UpdateScreen();
}

// Exibição de valores da tela 1
void funcDataScreen1(void){
	const TickType_t xMaxMutexDelay = pdMS_TO_TICKS(1);
	dataset linearVel = {0};
	dataset position = {0};
	char textBuffer[20];

	if(xSemaphoreTake(xMutexCurrentLinearVel, xMaxMutexDelay) == pdPASS){
		linearVel = xCurrentLinearVel;
		xSemaphoreGive(xMutexCurrentLinearVel);
	}
	if(xSemaphoreTake(xMutexCurrentPosition, xMaxMutexDelay) == pdPASS){
		position = xCurrentPosition;
		xSemaphoreGive(xMutexCurrentPosition);
	}

	sprintf(textBuffer, "Vx:%.1f   ", linearVel.x);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINE1_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "Vy:%.1f   ", linearVel.y);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINE2_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "Px:%.2f  ", position.x);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINE3_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "Py:%.2f  ", position.y);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINE4_Y, SSD1306_WHITE, SSD1306_BLACK);
	sim_printf("SCREEN1 | Vx=%.1f | Vy=%.1f | Px=%.2f | Py=%.2f\r\n", // UART PARA DEBUG!
	           linearVel.x, linearVel.y, position.x, position.y);
}

// Exibição dos valores da tela 2
void funcDataScreen2(void){
	const TickType_t xMaxMutexDelay = pdMS_TO_TICKS(1);
	dataset angularVel = {0};
	dataset linearVel = {0};
	char textBuffer[20];

	if(xSemaphoreTake(xMutexCurrentLinearVel, xMaxMutexDelay) == pdPASS){
		linearVel = xCurrentLinearVel;
		xSemaphoreGive(xMutexCurrentLinearVel);
	}
	if(xSemaphoreTake(xMutexCurrentAngularVel, xMaxMutexDelay) == pdPASS){
		angularVel = xCurrentAngularVel;
		xSemaphoreGive(xMutexCurrentAngularVel);
	}

	sprintf(textBuffer, "M1:%.1f   ", angularVel.x);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINE1_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "M2:%.1f   ", angularVel.y);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINE2_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "M3:%.1f   ", angularVel.z);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINE3_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "W :%.2f  ", linearVel.z);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINE4_Y, SSD1306_WHITE, SSD1306_BLACK);
	sim_printf("SCREEN2 | M1=%.1f | M2=%.1f | M3=%.1f | W=%.2f\r\n",
	           angularVel.x, angularVel.y, angularVel.z, linearVel.z); // QUART PARA DEBUG!!!
}

// Exibição dos valores da tela 3
void funcDataScreen3(void){
	const TickType_t xMaxMutexDelay = pdMS_TO_TICKS(1);
	dataset current = {0};
	char textBuffer[20];

	if(xSemaphoreTake(xMutexCurrentCurrent, xMaxMutexDelay) == pdPASS){
		current = xCurrentCurrent;
		xSemaphoreGive(xMutexCurrentCurrent);
	}

	sprintf(textBuffer, "Ix:%.1f   ", current.x);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINE1_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "Iy:%.1f   ", current.y);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINE2_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "Iz:%.1f   ", current.z);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINE3_Y, SSD1306_WHITE, SSD1306_BLACK);
	sim_printf("SCREEN3 | Ix=%.1f | Iy=%.1f | Iz=%.1f\r\n",
	           current.x, current.y, current.z); //Debug com UART
}

// Exibição de dados na tela
void dataScreen(uint16_t sScreenNum){
	switch(sScreenNum){
		case SCREEN1:
			funcDataScreen1();
			break;
		case SCREEN2:
			funcDataScreen2();
			break;
		case SCREEN3:
			funcDataScreen3();
			break;
		default:
			break;
	}
	SSD1306_UpdateScreen();
}

static int parseSimDataLine(char *line, char *type, float *x, float *y, float *z)
{
    char *ptr;
    char *end;

    if (line[0] == '\0' || line[1] != ',')
    {
        return 0;
    }

    *type = line[0];
    ptr = &line[2];

    *x = strtof(ptr, &end);
    if (end == ptr || *end != ',')
    {
        return 0;
    }

    ptr = end + 1;

    *y = strtof(ptr, &end);
    if (end == ptr || *end != ',')
    {
        return 0;
    }

    ptr = end + 1;

    *z = strtof(ptr, &end);
    if (end == ptr)
    {
        return 0;
    }

    return 1;
}

static BaseType_t sendLatestDatasetToQueue(QueueHandle_t queue, dataset *data)
{
    if (queue == NULL)
    {
        return pdFAIL;
    }

    if (xQueueSendToBack(queue, data, 0) == pdPASS)
    {
        return pdPASS;
    }

    /*
     * Se a fila estiver cheia, descartamos dados antigos.
     * Para a simulação de interface, interessa principalmente
     * o valor mais recente recebido.
     */
    xQueueReset(queue);

    return xQueueSendToBack(queue, data, 0);
}

static void sendSimDataToQueue(char type, float x, float y, float z)
{
    dataset data;

    data.x = x;
    data.y = y;
    data.z = z;
    data.timestamp = xTaskGetTickCount();

    if (type == 'P' || type == 'p')
    {
        if (sendLatestDatasetToQueue(xQueuePosition, &data) == pdPASS)
        {
            sim_printf("SIM QUEUE | P enviado para xQueuePosition\r\n");
        }
        else
        {
            sim_printf("SIM QUEUE | erro ao enviar P\r\n");
        }
    }
    else if (type == 'A' || type == 'a')
    {
        if (sendLatestDatasetToQueue(xQueueAngularVel, &data) == pdPASS)
        {
            sim_printf("SIM QUEUE | A enviado para xQueueAngularVel\r\n");
        }
        else
        {
            sim_printf("SIM QUEUE | erro ao enviar A\r\n");
        }
    }
    else if (type == 'C' || type == 'c')
    {
        if (sendLatestDatasetToQueue(xQueueCurrent, &data) == pdPASS)
        {
            sim_printf("SIM QUEUE | C enviado para xQueueCurrent\r\n");
        }
        else
        {
            sim_printf("SIM QUEUE | erro ao enviar C\r\n");
        }
    }
}

 // Task auxiliar para simulação no Renode.
 //Permite trocar a tela digitando 'n' pela UART2.
 //Não substitui o botão físico PA0/EXTI0, que continua sendo
 // o mecanismo principal de troca de telas no firmware.

void vTaskSimCommandReader(void *p) // debug com UART
{
    uint8_t ch;
    char line[80];
    uint8_t index = 0;

    while (1)
    {
        if (HAL_UART_Receive(&huart2, &ch, 1, 10) == HAL_OK)
        {
            if (ch == '\r')
            {
                /* Ignora carriage return */
            }
            else if (ch == '\n')
            {
                line[index] = '\0';

                if (index > 0)
                {
                    if ((line[0] == 'n' || line[0] == 'N') && line[1] == '\0')
                    {
                        sim_printf("SIM CMD | next screen\r\n");

                        if (xHandlerDisplayManager != NULL)
                        {
                            xTaskNotifyGive(xHandlerDisplayManager);
                        }
                    }
                    else
                    {
                        char type;
                        float x, y, z;

                        if (parseSimDataLine(line, &type, &x, &y, &z))
                        {
                            if (type == 'P' || type == 'p')
                            {
                                sim_printf("SIM DATA | P x=%.2f y=%.2f z=%.2f\r\n", x, y, z);
                                sendSimDataToQueue(type, x, y, z);
                            }
                            else if (type == 'A' || type == 'a')
                            {
                                sim_printf("SIM DATA | A x=%.2f y=%.2f z=%.2f\r\n", x, y, z);
                                sendSimDataToQueue(type, x, y, z);
                            }
                            else if (type == 'C' || type == 'c')
                            {
                                sim_printf("SIM DATA | C x=%.2f y=%.2f z=%.2f\r\n", x, y, z);
                                sendSimDataToQueue(type, x, y, z);
                            }
                            else
                            {
                                sim_printf("SIM DATA | tipo invalido: %c\r\n", type);
                                sendSimDataToQueue(type, x, y, z);
                            }
                        }
                        else
                        {
                            sim_printf("SIM CMD | linha invalida: %s\r\n", line);
                            sendSimDataToQueue(type, x, y, z);
                        }
                    }
                }

                index = 0;
            }
            else
            {
                if (index < sizeof(line) - 1)
                {
                    line[index++] = (char)ch;
                }
                else
                {
                    index = 0;
                    sim_printf("SIM CMD | linha muito longa\r\n");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }



}

