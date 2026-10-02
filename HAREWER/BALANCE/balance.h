#ifndef BALANCE_H
#define BALANCE_H
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_config.h"
/* PA8/TIM1_CH1; USART1 PB6 TX / PB7 RX. Wiring is unchanged. */
extern int g_ServoCenterPWM;
extern volatile uint32_t g_UART_RxCount;
void Balance_Init(void);
void Balance_StartRx(TaskHandle_t receiver);
int Balance_ReadRx(char *ch, uint32_t *timestamp_ms);
uint32_t Balance_RxErrors(void);
void Balance_SetServo(float angle);
void Balance_SetRawPWM(u16 pwm);
#endif
