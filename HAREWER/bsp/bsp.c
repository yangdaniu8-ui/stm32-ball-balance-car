/**
 * @file    bsp.c
 * @brief   板级支持包 — 硬件初始化
 *
 * 引脚冲突注意：
 *   扩展板占用 PA0~PA7（数码管）+ PB0（蜂鸣器），这些引脚不可用。
 *   编码器（PA0/PA1）暂时禁用。
 *   PA15 用作按键 KEY_DOWN，需先禁用 JTAG（保留 SWD）。
 */

#include "bsp.h"

/* ---- DWT 寄存器（core_cm3.h 版本较老，手动补全）---- */
typedef struct {
    __IO uint32_t CTRL;
    __IO uint32_t CYCCNT;
    __IO uint32_t CPICNT;
    __IO uint32_t EXCCNT;
    __IO uint32_t SLEEPCNT;
    __IO uint32_t LSUCNT;
    __IO uint32_t FOLDCNT;
    __IO uint32_t PCSR;
} DWT_Type;

#define DWT_BASE                ((uint32_t)0xE0001000)
#define DWT                     ((DWT_Type *)DWT_BASE)
#define DWT_CTRL_CYCCNTENA_Msk  (1UL << 0)

/* ---- 全局变量 ---- */
int  g_Speed   = 2500;
int  g_Mode    = 0;
u8   g_Running = 1;
volatile u32 g_SysTick = 0;
static u8  key_scan_div = 0;

/* ---- DWT ---- */
static void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

u32 DWT_GetTick(void)
{
    return DWT->CYCCNT;
}

/* ---- SysTick 1ms 中断 ---- */
static void SysTick_TimerInit(void)
{
    SysTick->LOAD = 9000 - 1;
    SysTick->VAL  = 0;
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk
                  |  SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void)
{
    g_SysTick++;
    key_scan_div++;
    if (key_scan_div >= 10) {
        key_scan_div = 0;
        Key_Scan();
    }
}

/**************************************************************************
 * bsp_init — 总初始化
 *
 * 注意：JTAG 禁用放在最前面，确保 PA15/PB3/PB4 可用作 GPIO。
 *       SWD（PA13/PA14）保持可用，不影响调试和下载。
 **************************************************************************/
void bsp_init(void)
{
    SystemInit();
    delay_init();

    /* ---- 禁用 JTAG，保留 SWD ---- */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    /* ---- DWT ---- */
    DWT_Init();

    /* ---- 电机 ---- */
    Moto_GPIO_Init();
    Moto_PWM_Init(7199, 0);     /* TIM4 CH3+CH4, 10kHz */

    /* ---- 巡线 ---- */
    LineWalking_GPIO_Init();

    /* ---- OLED ---- */
    OLED_Init();

    /* ---- 按键 ---- */
    Key_GPIO_Init();

    /* ---- 舵机 + K230D 串口 (USART1, 115200) ---- */
    Balance_Init();

    /* ---- SysTick 1ms ---- */
    SysTick_TimerInit();

    /* ---- 启动画面 ---- */
    OLED_Clear();
    OLED_ShowString(0, 0,  "H-Ball Car", 16);
    OLED_ShowString(0, 20, "Press START", 12);
    OLED_Refresh();
}

void System_Init(void)
{
    SystemInit();
}
