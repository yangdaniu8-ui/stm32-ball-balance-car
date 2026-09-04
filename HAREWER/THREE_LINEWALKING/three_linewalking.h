#ifndef __THREE_LINEWALKING_H__
#define __THREE_LINEWALKING_H__	

/*

	从车身后面往前看： 左侧到右边巡线传感器顺序为  L  M  R

*/


#define USE_LINE_L
#define USE_LINE_M
#define USE_LINE_R


#define LineWalk_L_RCC		RCC_APB2Periph_GPIOA
#define LineWalk_M_RCC		RCC_APB2Periph_GPIOA
#define LineWalk_R_RCC    	RCC_APB2Periph_GPIOA


#define LineWalk_L_PIN		GPIO_Pin_5
#define LineWalk_M_PIN		GPIO_Pin_4
#define LineWalk_R_PIN		GPIO_Pin_3


#define LineWalk_L_PORT	GPIOA
#define LineWalk_M_PORT	GPIOA
#define LineWalk_R_PORT	GPIOA



void three_LineWalking_GPIO_Init(void);
void three_GetLineWalking(int *p_iL, int *p_iM, int *p_iR);
void three_LineWalking(void);


#endif
