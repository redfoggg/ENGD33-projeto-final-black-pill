#include "maincode.h"

/* ================DEFINES E VARIÁVEIS DE CONTROLE================ */

uint16_t sTelaAtual = TELA1;

/* =============ESTRUTURAS DE ARMAZENAMENTO DE DADOS============== */

dataset xVelLinearAtual;
dataset xPosicaoAtual;
dataset xVelAngularAtual;
dataset xCorrenteAtual;

/* ===========================HANDLERS============================ */

TaskHandle_t xHandlerDisplayManager = NULL;

QueueHandle_t xQueueCorrente = NULL;
QueueHandle_t xQueueVelAngular = NULL;
QueueHandle_t xQueuePosicao = NULL;

SemaphoreHandle_t xMutexCorrenteAtual = NULL;
SemaphoreHandle_t xMutexVelLinearAtual = NULL;
SemaphoreHandle_t xMutexVelAngularAtual = NULL;
SemaphoreHandle_t xMutexPosicaoAtual = NULL;

/* =====================INICIALIZAÇÃO DO RTOS===================== */

void userRTOS(void) {
    xMutexVelLinearAtual = xSemaphoreCreateMutex();
    xMutexVelAngularAtual = xSemaphoreCreateMutex();
    xMutexPosicaoAtual = xSemaphoreCreateMutex();

    xQueueCorrente = xQueueCreate(50, sizeof(dataset));
    xQueueVelAngular = xQueueCreate(30, sizeof(dataset));
    xQueuePosicao = xQueueCreate(5, sizeof(dataset));

    xTaskCreate(vDisplayManager, "displayManager", 2048, (void*) 0, 1, &xHandlerDisplayManager);
    xTaskCreate(vTaskGerarQueueCorrente, "gerarQueueCorrente", 128, (void*) 0, 5, NULL);
    xTaskCreate(vTaskGerarQueueVelAngular, "gerarQueueVelAngular", 128, (void*) 0, 4, NULL);
    xTaskCreate(vTaskGerarQueuePosicao, "gerarQueuePosicao", 128, (void*) 0, 3, NULL);
    xTaskCreate(vTaskQueueCorrenteReader, "queueCorrenteReader", 256, (void*) 0, 2, NULL);
    xTaskCreate(vTaskQueueVelAngularReader, "queueVelAngularReader", 256, (void*) 0, 2, NULL);
    xTaskCreate(vTaskQueuePosicaoReader, "queuePosicaoReader", 256, (void*) 0, 2, NULL);

    vTaskStartScheduler();

    while(1);
}

/* =========================TASKS DO RTOS========================= */

// Gerenciamento da tela
void vDisplayManager(void *p){
	TickType_t xLastWakeTime;;
	uint16_t sIndice = 0;
	while(1){
		xLastWakeTime = xTaskGetTickCount();
		if(sIndice >= 9){
			switch(sTelaAtual) {
    		    case TELA1:
    		        sTelaAtual = TELA2;
    		        break;
    		    case TELA2:
    		        sTelaAtual = TELA3;
    		        break;
    		    case TELA3:
    		       	sTelaAtual = TELA1;
    		        break;
    		    default:
    		        break;
    		}
			baseTela(sTelaAtual);
			sIndice = 0;
		}else{
			sIndice++;
		}
		dadosTela(sTelaAtual);
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(REFRESH_TELA));
	}
}

// Geração de dados de corrente e envio para queue
void vTaskGerarQueueCorrente(void *p) {
	TickType_t xLastWakeTime;
	dataset correntes;
	uint16_t sIndice = 0;
	while(1) {
		xLastWakeTime = xTaskGetTickCount();
		correntes.x = vetorCorrenteX[sIndice];
		correntes.y = vetorCorrenteY[sIndice];
		correntes.z = vetorCorrenteZ[sIndice];
		correntes.timestamp = xLastWakeTime;
		if(sIndice >= LENGTH_LUT - 1){
			sIndice = 0;
		}else{
			sIndice++;
		}
		if(xQueueSendToBack(xQueueCorrente, &correntes, 0) == errQUEUE_FULL){
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 0);
		}
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
	}
}

// Geração de dados de velocidade angular e envio para queue
void vTaskGerarQueueVelAngular(void *p) {
	TickType_t xLastWakeTime;
	dataset velAngular;
	uint16_t sIndice = 0;
	while(1) {
		xLastWakeTime = xTaskGetTickCount();
		velAngular.x = vetorVelAngX[sIndice];
		velAngular.y = vetorVelAngY[sIndice];
		velAngular.z = vetorVelAngZ[sIndice];
		velAngular.timestamp = xLastWakeTime;
		if(sIndice >= LENGTH_LUT - 1){
			sIndice = 0;
		}else{
			sIndice++;
		}
		if(xQueueSendToBack(xQueueVelAngular, &velAngular, 0) == errQUEUE_FULL){
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 0);
		}
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
	}
}

// Geração de dados de GPS e envio para queue
void vTaskGerarQueuePosicao(void *p) {
	TickType_t xLastWakeTime;
	dataset posicao;
	uint16_t sIndice = 0;
	while(1) {
		xLastWakeTime = xTaskGetTickCount();
		posicao.x = vetorPosicaoX[sIndice];
		posicao.y = vetorPosicaoY[sIndice];
		posicao.z = vetorPosicaoZ[sIndice];
		posicao.timestamp = xLastWakeTime;
		if(sIndice >= LENGTH_LUT - 1){
			sIndice = 0;
		}else{
			sIndice++;
		}
		if(xQueueSendToBack(xQueuePosicao, &posicao, 0) == errQUEUE_FULL){
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 0);
		}
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
	}
}

// Leitura de dados de corrente da queue
void vTaskQueueCorrenteReader(void *p) {
	const TickType_t xMaxMutexDelay = pdMS_TO_TICKS(1);
	TickType_t xLastWakeTime;
	dataset corrente;
	while (1) {
		xLastWakeTime = xTaskGetTickCount();
		while(xQueueReceive(xQueueCorrente, &corrente, 0) != errQUEUE_EMPTY){
			if(xSemaphoreTake(xMutexCorrenteAtual, xMaxMutexDelay) == pdPASS){
				xCorrenteAtual = corrente;
				xSemaphoreGive(xMutexCorrenteAtual);
			}
		}
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(125));
	}
}

// Leitura de dados de velocidade angular da queue
void vTaskQueueVelAngularReader(void *p) {
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
	dataset velAngular, velLinear;
	while (1) {
		xLastWakeTime = xTaskGetTickCount();

		while(xQueueReceive(xQueueVelAngular, &velAngular, 0) != errQUEUE_EMPTY) {
			linearVX = r_cm*(2.0/3.0)*(-(sin_alpha1*velAngular.x)-(sin_alpha2*velAngular.y)-(sin_alpha3*velAngular.z));
			linearVY = r_cm*(2.0/3.0)*((cos_alpha1*velAngular.x)+(cos_alpha2*velAngular.y)+(cos_alpha3*velAngular.z));
			linearW = (r_cm*(velAngular.x+velAngular.y+velAngular.z))/(3*L_cm);
			velLinear.x = linearVX;
			velLinear.y = linearVY;
			velLinear.z = linearW;

			if(xSemaphoreTake(xMutexVelLinearAtual, xMaxMutexDelay) == pdPASS){
				xVelLinearAtual = velLinear;
				xSemaphoreGive(xMutexVelLinearAtual);
			}

			if(xSemaphoreTake(xMutexVelAngularAtual, xMaxMutexDelay) == pdPASS){
				xVelAngularAtual = velAngular;
				xSemaphoreGive(xMutexVelAngularAtual);
			}
		}

		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
	}
}

// Leitura de dados de GPS da queue
void vTaskQueuePosicaoReader(void *p) {
	const TickType_t xMaxMutexDelay = pdMS_TO_TICKS(1);
	TickType_t xLastWakeTime;
	dataset posicao;
	while (1) {
		xLastWakeTime = xTaskGetTickCount();
		while(xQueueReceive(xQueuePosicao, &posicao, 0) != errQUEUE_EMPTY){
			if(xSemaphoreTake(xMutexPosicaoAtual, xMaxMutexDelay) == pdPASS){
				xPosicaoAtual = posicao;
				xSemaphoreGive(xMutexPosicaoAtual);
			}
		}
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(125));
	}
}

/* ======================FUNÇÕES AUXILIARES======================= */

/* Layout para o OLED SSD1306 (128x64, monocromatico).
 * Titulo na linha y=0; valores nas linhas y=14/26/38/50 usando FONT1 (8x12). */
#define TITULO_Y	0
#define LINHA1_Y	14
#define LINHA2_Y	26
#define LINHA3_Y	38
#define LINHA4_Y	50

// Inicialização da tela executada antes da inicialização do RTOS
void inicializar(void){
	SSD1306_Init();
	baseTela(sTelaAtual);
}

// Base da Tela 1
void funcBaseTela1(void){
	SSD1306_DrawText("Vel Lin / Pos", FONT1, 0, TITULO_Y, SSD1306_WHITE, SSD1306_BLACK);
}

// Base da Tela 2
void funcBaseTela2(void){
	SSD1306_DrawText("Vel Angular", FONT1, 0, TITULO_Y, SSD1306_WHITE, SSD1306_BLACK);
}

// Base da Tela 3
void funcBaseTela3(void){
	SSD1306_DrawText("Corrente", FONT1, 0, TITULO_Y, SSD1306_WHITE, SSD1306_BLACK);
}

// Seleção de base de tela
void baseTela(uint16_t sNumTela){
	SSD1306_Fill(SSD1306_BLACK);
	switch(sNumTela){
		case TELA1:
			funcBaseTela1();
			break;
		case TELA2:
			funcBaseTela2();
			break;
		case TELA3:
			funcBaseTela3();
			break;
		default:
			break;
	}
	SSD1306_UpdateScreen();
}

// Exibição de valores da tela 1
void funcDadosTela1(void){
	const TickType_t xMaxMutexDelay = pdMS_TO_TICKS(1);
	dataset velLinear;
	dataset posicao;
	char textBuffer[20];

	if(xSemaphoreTake(xMutexVelLinearAtual, xMaxMutexDelay) == pdPASS){
		velLinear = xVelLinearAtual;
		xSemaphoreGive(xMutexVelLinearAtual);
	}
	if(xSemaphoreTake(xMutexPosicaoAtual, xMaxMutexDelay) == pdPASS){
		posicao = xPosicaoAtual;
		xSemaphoreGive(xMutexPosicaoAtual);
	}

	sprintf(textBuffer, "Vx:%.1f   ", velLinear.x);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINHA1_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "Vy:%.1f   ", velLinear.y);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINHA2_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "Px:%.2f  ", posicao.x);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINHA3_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "Py:%.2f  ", posicao.y);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINHA4_Y, SSD1306_WHITE, SSD1306_BLACK);
}

// Exibição dos valores da tela 2
void funcDadosTela2(void){
	const TickType_t xMaxMutexDelay = pdMS_TO_TICKS(1);
	dataset velAngular;
	dataset velLinear;
	char textBuffer[20];

	if(xSemaphoreTake(xMutexVelLinearAtual, xMaxMutexDelay) == pdPASS){
		velLinear = xVelLinearAtual;
		xSemaphoreGive(xMutexVelLinearAtual);
	}
	if(xSemaphoreTake(xMutexVelAngularAtual, xMaxMutexDelay) == pdPASS){
		velAngular = xVelAngularAtual;
		xSemaphoreGive(xMutexVelAngularAtual);
	}

	sprintf(textBuffer, "M1:%.1f   ", velAngular.x);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINHA1_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "M2:%.1f   ", velAngular.y);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINHA2_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "M3:%.1f   ", velAngular.z);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINHA3_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "W :%.2f  ", velLinear.z);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINHA4_Y, SSD1306_WHITE, SSD1306_BLACK);
}

// Exibição dos valores da tela 3
void funcDadosTela3(void){
	const TickType_t xMaxMutexDelay = pdMS_TO_TICKS(1);
	dataset corrente;
	char textBuffer[20];

	if(xSemaphoreTake(xMutexCorrenteAtual, xMaxMutexDelay) == pdPASS){
		corrente = xCorrenteAtual;
		xSemaphoreGive(xMutexCorrenteAtual);
	}

	sprintf(textBuffer, "Ix:%.1f   ", corrente.x);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINHA1_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "Iy:%.1f   ", corrente.y);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINHA2_Y, SSD1306_WHITE, SSD1306_BLACK);
	sprintf(textBuffer, "Iz:%.1f   ", corrente.z);
	SSD1306_DrawText(textBuffer, FONT1, 0, LINHA3_Y, SSD1306_WHITE, SSD1306_BLACK);
}

// Exibição de dados na tela
void dadosTela(uint16_t sNumTela){
	switch(sNumTela){
		case TELA1:
			funcDadosTela1();
			break;
		case TELA2:
			funcDadosTela2();
			break;
		case TELA3:
			funcDadosTela3();
			break;
		default:
			break;
	}
	SSD1306_UpdateScreen();
}


