#ifndef __MOTO_H
#define	__MOTO_H

#include "stm32f10x.h"

/************************************************************
 * TB6612 电机驱动引脚定义（避开 PA0~PA7 数码管 + PB0 蜂鸣器）
 *
 * PWM：TIM4 CH3(PB8)=M1速度, CH4(PB9)=M2速度
 * 方向：PB1=IA1, PB5=IA2, PB14=IB1, PB15=IB2
 * STBY：PB13
 *
 * 控制逻辑：
 *   IN1=1, IN2=0 → 正转
 *   IN1=0, IN2=1 → 反转
 *   IN1=0, IN2=0 → 滑行停止
 *   IN1=1, IN2=1 → 刹车
 *
 * 软启动：MOTO_RAMP_RATE=5000 单位/秒，0→满速 ≈1.4s
 ************************************************************/

/* ---- 软启动 ---- */
#define MOTO_RAMP_RATE      5000

/* ---- M1 电机（右后轮）---- */
#define MOTO_M1_IA1_PORT    GPIOB
#define MOTO_M1_IA1_PIN     GPIO_Pin_1       /* PB1 */
#define MOTO_M1_IA2_PORT    GPIOB
#define MOTO_M1_IA2_PIN     GPIO_Pin_5       /* PB5 */

/* M1 PWM — TIM4_CH3 (PB8) */
#define MOTO_M1_PWM_PORT    GPIOB
#define MOTO_M1_PWM_PIN     GPIO_Pin_8
#define MOTO_M1_PWM_TIM     TIM4
#define MOTO_M1_PWM_CH      3

/* ---- M2 电机（左后轮）---- */
#define MOTO_M2_IB1_PORT    GPIOB
#define MOTO_M2_IB1_PIN     GPIO_Pin_14      /* PB14 */
#define MOTO_M2_IB2_PORT    GPIOB
#define MOTO_M2_IB2_PIN     GPIO_Pin_15      /* PB15 */

/* M2 PWM — TIM4_CH4 (PB9) */
#define MOTO_M2_PWM_PORT    GPIOB
#define MOTO_M2_PWM_PIN     GPIO_Pin_9
#define MOTO_M2_PWM_TIM     TIM4
#define MOTO_M2_PWM_CH      4

/* ---- STBY ---- */
#define MOTO_STBY_PORT      GPIOB
#define MOTO_STBY_PIN       GPIO_Pin_13      /* PB13 */

/* ---- 方向控制宏 ---- */
#define M1_IA1_HIGH()   GPIO_SetBits(MOTO_M1_IA1_PORT, MOTO_M1_IA1_PIN)
#define M1_IA1_LOW()    GPIO_ResetBits(MOTO_M1_IA1_PORT, MOTO_M1_IA1_PIN)
#define M1_IA2_HIGH()   GPIO_SetBits(MOTO_M1_IA2_PORT, MOTO_M1_IA2_PIN)
#define M1_IA2_LOW()    GPIO_ResetBits(MOTO_M1_IA2_PORT, MOTO_M1_IA2_PIN)

#define M2_IB1_HIGH()   GPIO_SetBits(MOTO_M2_IB1_PORT, MOTO_M2_IB1_PIN)
#define M2_IB1_LOW()    GPIO_ResetBits(MOTO_M2_IB1_PORT, MOTO_M2_IB1_PIN)
#define M2_IB2_HIGH()   GPIO_SetBits(MOTO_M2_IB2_PORT, MOTO_M2_IB2_PIN)
#define M2_IB2_LOW()    GPIO_ResetBits(MOTO_M2_IB2_PORT, MOTO_M2_IB2_PIN)

#define MOTO_STBY_EN()  GPIO_SetBits(MOTO_STBY_PORT, MOTO_STBY_PIN)
#define MOTO_STBY_DIS() GPIO_ResetBits(MOTO_STBY_PORT, MOTO_STBY_PIN)

/* ---- 速度等级 ---- */
#define SPEED_MAX      7199
#define SPEED_HIGH     6500
#define SPEED_MED      5000
#define SPEED_LOW      3500
#define SPEED_MIN      800

/* ---- 函数声明 ---- */
void Moto_GPIO_Init(void);
void Moto_PWM_Init(u16 arr, u16 psc);
void Moto_SetM1Speed(int speed);
void Moto_SetM2Speed(int speed);
void Moto_RampUpdate(void);
void Forward(int Speed);
void Backward(int Speed);
void Turnleft(int Speed);
void Turnright(int Speed);
void Stop(void);
void SpinLeft(int Speed);
void SpinRight(int Speed);
void Moto_Brake(void);

#endif
