#ifndef _ENCODER_H
#define _ENCODER_H

#include "stm32f10x.h"

/************************************************************
 * 编码器引脚定义
 * M1 编码器（右后轮）— TIM2 编码器模式
 *   M1A → PA0 (TIM2_CH1)
 *   M1B → PA1 (TIM2_CH2)
 *
 * M2 编码器（左后轮）— TIM4 编码器模式
 *   M2A → PB6 (TIM4_CH1)
 *   M2B → PB7 (TIM4_CH2)
 ************************************************************/

/* ---------- M1 编码器（TIM2）---------- */
#define ENC_M1_TIM          TIM2
#define ENC_M1_RCC_APB      RCC_APB1Periph_TIM2
#define ENC_M1_PORT         GPIOA
#define ENC_M1_PIN_A        GPIO_Pin_0
#define ENC_M1_PIN_B        GPIO_Pin_1
#define ENC_M1_RCC_GPIO     RCC_APB2Periph_GPIOA

/* ---------- M2 编码器（TIM4）---------- */
#define ENC_M2_TIM          TIM4
#define ENC_M2_RCC_APB      RCC_APB1Periph_TIM4
#define ENC_M2_PORT         GPIOB
#define ENC_M2_PIN_A        GPIO_Pin_6
#define ENC_M2_PIN_B        GPIO_Pin_7
#define ENC_M2_RCC_GPIO     RCC_APB2Periph_GPIOB

/* ---------- 编码器计数上限 ---------- */
#define ENC_MAX_COUNT       65535

/* 电机标识 */
#define ENC_MOTOR_M1        1
#define ENC_MOTOR_M2        2

/* ---------- 函数声明 ---------- */
void Encoder_Init_M1(void);             /* TIM2 编码器模式初始化 */
void Encoder_Init_M2(void);             /* TIM4 编码器模式初始化 */
int  Encoder_GetCount(u8 motor);        /* 读取编码器计数值（不清零） */
int  Encoder_ReadAndClear(u8 motor);    /* 读取编码器计数值并清零 */
void Encoder_ClearCount(u8 motor);      /* 清零编码器计数 */

/* 速度计算相关 */
float Encoder_GetRPM(u8 motor, u16 ppr, u16 interval_ms);  /* 计算转速 RPM */

#endif
