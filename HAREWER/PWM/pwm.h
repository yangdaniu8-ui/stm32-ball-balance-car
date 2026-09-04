#ifndef __PWM_H
#define	__PWM_H

#include "stm32f10x.h"

/************************************************************
 * PWM 驱动（已集成到 moto.c 的 Moto_PWM_Init）
 * 本文件保留用于兼容旧代码引用
 *
 * TIM3 CH1 (PA6) → M1 电机速度 (A)
 * TIM3 CH2 (PA7) → M2 电机速度 (B)
 ************************************************************/

/* PWM 占空比直接操作宏（兼容旧代码，建议使用 Moto_SetMxSpeed） */
#define   M1_PWM   TIM3->CCR1
#define   M2_PWM   TIM3->CCR2

void PWM_Int(u16 arr, u16 psc);

#endif
