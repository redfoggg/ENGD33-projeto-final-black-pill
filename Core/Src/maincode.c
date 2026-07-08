#include "maincode.h"

/* ================DEFINES E VARIÁVEIS DE CONTROLE================ */

uint16_t sCurrentScreen = SCREEN1;

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
    xMutexCurrentCurrent = xSemaphoreCreateMutex();
    xMutexCurrentLinearVel = xSemaphoreCreateMutex();
    xMutexCurrentAngularVel = xSemaphoreCreateMutex();
    xMutexCurrentPosition = xSemaphoreCreateMutex();

    xQueueCurrent = xQueueCreate(50, sizeof(dataset));
    xQueueAngularVel = xQueueCreate(30, sizeof(dataset));
    xQueuePosition = xQueueCreate(5, sizeof(dataset));

    xTaskCreate(vDisplayManager, "displayManager", 2048, (void*) 0, 1, &xHandlerDisplayManager);
    xTaskCreate(vTaskGenerateCurrentQueue, "gerarQueueCorrente", 128, (void*) 0, 5, NULL);
    xTaskCreate(vTaskGenerateAngularVelQueue, "gerarQueueVelAngular", 128, (void*) 0, 4, NULL);
    xTaskCreate(vTaskGeneratePositionQueue, "gerarQueuePosicao", 128, (void*) 0, 3, NULL);
    xTaskCreate(vTaskQueueCurrentReader, "queueCorrenteReader", 256, (void*) 0, 2, NULL);
    xTaskCreate(vTaskQueueAngularVelReader, "queueVelAngularReader", 256, (void*) 0, 2, NULL);
    xTaskCreate(vTaskQueuePositionReader, "queuePosicaoReader", 256, (void*) 0, 2, NULL);
}

/* =====================CALLBACKS DE INTERRUPÇÃO===================== */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	static TickType_t xLastPress = 0;
	if(GPIO_Pin == BTN_Pin){
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		TickType_t xNow = xTaskGetTickCountFromISR();
		if((xNow - xLastPress) >= pdMS_TO_TICKS(200)){
			xLastPress = xNow;
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
		currents.x = currentVectorX[sIndex];
		currents.y = currentVectorY[sIndex];
		currents.z = currentVectorZ[sIndex];
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

// Geração de dados de velocidade angular e envio para queue
void vTaskGenerateAngularVelQueue(void *p) {
	TickType_t xLastWakeTime;
	dataset angularVel;
	uint16_t sIndex = 0;
	while(1) {
		xLastWakeTime = xTaskGetTickCount();
		angularVel.x = angularVelVectorX[sIndex];
		angularVel.y = angularVelVectorY[sIndex];
		angularVel.z = angularVelVectorZ[sIndex];
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
		position.x = positionVectorX[sIndex];
		position.y = positionVectorY[sIndex];
		position.z = positionVectorZ[sIndex];
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


