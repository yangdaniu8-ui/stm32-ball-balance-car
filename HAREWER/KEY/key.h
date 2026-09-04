#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

/************************************************************
 * 按键驱动（内部上拉，按下低电平）
 *
 * 引脚分配：
 *   KEY_START → PA11  — 启动/停止
 *   KEY_MODE  → PA12  — 模式切换
 *   KEY_UP    → PA8   — 速度+
 *   KEY_DOWN  → PA15  — 速度-（需禁用 JTAG）
 ************************************************************/

#define KEY_START_PORT      GPIOA
#define KEY_START_PIN       GPIO_Pin_11
#define KEY_START_RCC       RCC_APB2Periph_GPIOA

#define KEY_MODE_PORT       GPIOA
#define KEY_MODE_PIN        GPIO_Pin_12
#define KEY_MODE_RCC        RCC_APB2Periph_GPIOA

#define KEY_UP_PORT         GPIOB
#define KEY_UP_PIN          GPIO_Pin_3        /* PB3 (PA8 给舵机 TIM1_CH1) */
#define KEY_UP_RCC          RCC_APB2Periph_GPIOB

#define KEY_DOWN_PORT       GPIOA
#define KEY_DOWN_PIN        GPIO_Pin_15
#define KEY_DOWN_RCC        RCC_APB2Periph_GPIOA

/* ---- 按键编号 ---- */
#define KEY_ID_START        0
#define KEY_ID_MODE         1
#define KEY_ID_UP           2
#define KEY_ID_DOWN         3
#define KEY_COUNT           4

/* ---- 按键状态 ---- */
#define KEY_STATE_NONE      0
#define KEY_STATE_SHORT     1
#define KEY_STATE_LONG      2
#define KEY_STATE_HOLD      3

/* ---- 函数声明 ---- */
void Key_GPIO_Init(void);
void Key_Scan(void);
u8   Key_GetState(u8 key_id);
u8   Key_IsPressed(u8 key_id);
u8   Key_AnyPressed(void);

#endif
