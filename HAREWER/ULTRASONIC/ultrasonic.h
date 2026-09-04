#ifndef __ULTRASONIC_H__
#define __ULTRASONIC_H__

/*³¬Éù²¨´«¸ÐÆ÷*/
#define USE_ULTRASONIC

#define TRIG_RCC		RCC_APB2Periph_GPIOA
#define ECHO_RCC		RCC_APB2Periph_GPIOA

#define TRIG_PIN		GPIO_Pin_15
#define ECHO_PIN		GPIO_Pin_12

#define TRIG_PORT		GPIOA
#define ECHO_PORT		GPIOA

void Ultrasonic_GPIO_Init(void);
void Ultrasonic_servo_mode(void);


extern float GetUltrasonicDistance(void);
extern void Ultrasonic_Timer2_Init(void);


#endif

