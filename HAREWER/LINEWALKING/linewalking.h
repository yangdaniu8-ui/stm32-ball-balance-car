#ifndef LINEWALKING_H
#define LINEWALKING_H
#include "stm32f10x.h"
/* Physical order: L1=PC14, L2=PB4, R1=PC15, R2=PB12. LOW means black.
 * Tune the controller in USER/APP/app_config.h, not in this GPIO header.
 */
#define LineWalk_L1_PIN GPIO_Pin_14
#define LineWalk_L2_PIN GPIO_Pin_4
#define LineWalk_R1_PIN GPIO_Pin_15
#define LineWalk_R2_PIN GPIO_Pin_12
#define LineWalk_L1_PORT GPIOC
#define LineWalk_L2_PORT GPIOB
#define LineWalk_R1_PORT GPIOC
#define LineWalk_R2_PORT GPIOB
void LineWalking_GPIO_Init(void);
void GetLineWalking(int *l1, int *l2, int *r1, int *r2);
void LineWalking_PID(int baseSpeed);
void LineWalking(void);
u8 Check_StartLine(void);
#endif
