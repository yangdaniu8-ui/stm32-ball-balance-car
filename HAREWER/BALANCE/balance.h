#ifndef __BALANCE_H
#define __BALANCE_H

#include "stm32f10x.h"

/************************************************************
 * 钢球平衡控制模块
 *
 * 硬件：
 *   舵机 MG996R → PA8 (TIM1_CH1), 50Hz PWM
 *   视觉 K230D  → USART1 PA9(TX)/PA10(RX), 115200bps
 *
 * 摆杆参数：
 *   半长 12.5cm，钢球目标位置由 K230D 视觉模块发送
 ************************************************************/

/* ---- 舵机 PWM 参数 ---- */
#define SERVO_PWM_MIN       500     /* 0°  脉冲 0.5ms */
#define SERVO_PWM_MAX       2500    /* 180° 脉冲 2.5ms */
#define SERVO_PWM_CENTER    1800    /* 水平校准值 */
#define SERVO_MAX_ANGLE     12.0f   /* 最大摆动角度 */

/* ---- 摆杆参数 ---- */
#define ROD_HALF_LEN        12.5f   /* 摆杆半长 (cm) */

/* ---- 串级 PID 参数 ---- */

/* 速度内环 */
#define VEL_KP              1.4f    /* 速度环 P */
#define VEL_KI              0.01f   /* 速度环 I */
#define VEL_KD              1.8f    /* 速度环 D (压直线摆动) */
#define VEL_INTEGRAL_MAX     10.0f   /* 速度积分限幅 */
#define VEL_OUTPUT_MAX       20.0f   /* 速度环输出上限 (cm/s) */

/* 位置外环 */
#define POS_KP              2.0f    /* 位置环 P (快拉回中心) */
#define POS_KI              0.18f    /* 位置环 I */
#define POS_INTEGRAL_MAX     10.0f   /* 位置积分限幅 */

/* ---- 加速度前馈 ---- */
extern volatile float g_AccelFF;     /* 运行时加速度前馈角度 (°) */

/* ---- 共用参数 ---- */
#define BAL_DEADZONE         0.18f   /* 死区 cm */
#define BAL_FILTER_ALPHA     0.4f    /* EMA 滤波系数 (0~1) */
#define BAL_RAMP_MAX          1.0f    /* 舵机速度限幅 */
#define SERVO_INVERT           1      /* 1=反转舵机方向 */
#define SERVO_BIAS            1.2f    /* 机械不对称偏置 (°) */

/* ---- 钢球目标位置（由 K230D 更新）---- */
extern volatile float g_BallTarget_cm;
extern volatile float g_BallCurrent_cm;
extern volatile u8    g_BallDataReady;
extern volatile u16   g_UART_RxCount;
extern volatile u16   g_PB7_ToggleCount;
extern volatile char  g_LastRxByte;         /* 最后一个收到的字节 */
extern int g_ServoCenterPWM;

/* ---- 函数声明 ---- */
void Balance_Init(void);
void Balance_PID_Update(void);
void Balance_SetServo(float angle);
void Balance_SetRawPWM(u16 pwm);
void Balance_UART_Poll(void);
void Balance_GPIO_Monitor(void);        /* GPIO 监听 PB7 电平变化 */

#endif
