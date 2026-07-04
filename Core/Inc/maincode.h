#ifndef MAINCODE_H
#define MAINCODE_H

/* ===========================INCLUDES============================ */

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "main.h"
#include "ssd1306.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "usertypedefs.h"
#include "lookuptable.h"

/* ================DEFINES AND CONTROL VARIABLES================ */

#define SCREEN1 1
#define SCREEN2 2
#define SCREEN3 3

#define REFRESH_SCREEN 699

extern uint16_t sCurrentScreen;

/* =============DATA STORAGE STRUCTURES============== */

extern dataset xCurrentLinearVel;
extern dataset xCurrentPosition;
extern dataset xCurrentAngularVel;
extern dataset xCurrentCurrent;

/* ===========================HANDLERS============================ */

extern TaskHandle_t xHandlerDisplayManager;

extern QueueHandle_t xQueueCurrent;
extern QueueHandle_t xQueueAngularVel;
extern QueueHandle_t xQueuePosition;

extern SemaphoreHandle_t xMutexCurrentCurrent;
extern SemaphoreHandle_t xMutexCurrentLinearVel;
extern SemaphoreHandle_t xMutexCurrentAngularVel;
extern SemaphoreHandle_t xMutexCurrentPosition;

/* ==========================PROTOTYPES========================== */

void userRTOS(void);
void vDisplayManager(void*);
void vTaskGenerateCurrentQueue(void*);
void vTaskGenerateAngularVelQueue(void*);
void vTaskGeneratePositionQueue(void*);
void vTaskQueueCurrentReader(void*);
void vTaskQueueAngularVelReader(void*);
void vTaskQueuePositionReader(void*);
void initialize(void);
void funcBaseScreen1(void);
void funcBaseScreen2(void);
void funcBaseScreen3(void);
void baseScreen(uint16_t);
void funcDataScreen1(void);
void funcDataScreen2(void);
void funcDataScreen3(void);
void dataScreen(uint16_t);

#endif /* MAINCODE_H */
