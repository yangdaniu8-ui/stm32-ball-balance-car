#include "bsp.h"
int g_Speed = 2500;
int g_Mode = 0;
u8 g_Running = 0;
u32 DWT_GetTick(void) { return *((volatile uint32_t *)0xE0001004u); }
void bsp_init(void)
{
    /* Startup has already called SystemInit(). */
    SystemCoreClockUpdate();
    delay_init();
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    Moto_GPIO_Init();
    Moto_PWM_Init(7199, 0);
    Stop();
    LineWalking_GPIO_Init();
    Key_GPIO_Init();
    Balance_Init();
    OLED_Init();
}
void System_Init(void) { SystemInit(); }
