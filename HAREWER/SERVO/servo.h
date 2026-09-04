#ifndef __SERVO_H
#define	__SERVO_H

#include "stm32f10x.h"

extern int Angle_J1;

/*定义需要初始化的舵机宏定义开关*/
#define USE_SERVO_J1
#define Servo_J1_PIN	GPIO_Pin_11
#define Servo_J1_PORT	GPIOA
#define Servo_J1_RCC	RCC_APB2Periph_GPIOA

void Servo_J1(int v_iAngle);/*定义一个脉冲函数，用来模拟方式产生PWM值*/
void Servo_GPIO_Init(void);

void front_detection(void);
void left_detection(void);
void right_detection(void);

void TIM1_Int_Init(u16 arr,u16 psc);


#endif
