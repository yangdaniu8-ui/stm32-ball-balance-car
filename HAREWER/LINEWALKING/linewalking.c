#include "stm32f10x.h"
#include "sys.h"
#include "linewalking.h"
#include "moto.h"
#include "control_math.h"
extern int g_Speed;
static LineController compatibility_controller;
void LineWalking_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin = LineWalk_L1_PIN | LineWalk_R1_PIN;
    GPIO_Init(GPIOC, &gpio);
    gpio.GPIO_Pin = LineWalk_L2_PIN | LineWalk_R2_PIN;
    GPIO_Init(GPIOB, &gpio);
}
void GetLineWalking(int *l1, int *l2, int *r1, int *r2)
{
    *l1 = GPIO_ReadInputDataBit(LineWalk_L1_PORT, LineWalk_L1_PIN);
    *l2 = GPIO_ReadInputDataBit(LineWalk_L2_PORT, LineWalk_L2_PIN);
    *r1 = GPIO_ReadInputDataBit(LineWalk_R1_PORT, LineWalk_R1_PIN);
    *r2 = GPIO_ReadInputDataBit(LineWalk_R2_PORT, LineWalk_R2_PIN);
}
/* Compatibility only: production control uses its own resettable controller. */
void LineWalking_PID(int baseSpeed)
{
    int l1, l2, r1, r2, right, left;
    uint8_t mask;
    GetLineWalking(&l1, &l2, &r1, &r2);
    mask = (uint8_t)((l1 == LOW ? 1 : 0) | (l2 == LOW ? 2 : 0) |
                    (r1 == LOW ? 4 : 0) | (r2 == LOW ? 8 : 0));
    LineController_Update(&compatibility_controller, mask, baseSpeed, 0.01f, &right, &left);
    Moto_SetM1Speed(right);
    Moto_SetM2Speed(left);
}
void LineWalking(void) { LineWalking_PID(g_Speed); }
u8 Check_StartLine(void)
{
    int l1, l2, r1, r2;
    GetLineWalking(&l1, &l2, &r1, &r2);
    /* Not used for lap completion: the real start marker is too short. */
    return l1 == LOW && l2 == LOW && r1 == LOW && r2 == LOW;
}
