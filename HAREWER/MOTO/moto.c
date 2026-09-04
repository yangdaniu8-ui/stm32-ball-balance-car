#include "moto.h"

/* DWT 周期计数器（72MHz，精确计时） */
extern u32 DWT_GetTick(void);
extern volatile u32 g_SysTick;

/**************************************************************************
 * 软启动数据结构
 * ramp_current — 当前实际 PWM（有符号, 正=正转, 负=反转）
 * ramp_target  — 目标 PWM
 * last_dwt     — 上次更新时的 DWT 计数值
 *
 * Moto_SetMxSpeed() → 只更新 target
 * Moto_RampUpdate()  → 每循环调用，按 MOTO_RAMP_RATE 速率平滑过渡
 * Stop() / Brake()   → 直接操作硬件，跳过斜坡
 *
 * 计时用 DWT->CYCCNT (72MHz)，不受 delay_ms 关闭 SysTick 影响
 **************************************************************************/
typedef struct {
    int  current;
    int  target;
    u32  last_dwt;
} MotoRamp_t;

static MotoRamp_t ramp_m1;
static MotoRamp_t ramp_m2;

/* ---- 内部辅助：限幅 ---- */
static int Moto_Clamp(int val, int min, int max)
{
    if (val > max) return max;
    if (val < min) return min;
    return val;
}

/* ---- 内部辅助：直接写硬件（TIM4 CH3/CH4）---- */
static void Moto_ApplyM1HW(int speed)
{
    if (speed > 0)
    {
        M1_IA1_LOW();
        M1_IA2_HIGH();
        TIM_SetCompare3(TIM4, (u16)speed);
    }
    else if (speed < 0)
    {
        M1_IA1_HIGH();
        M1_IA2_LOW();
        TIM_SetCompare3(TIM4, (u16)(-speed));
    }
    else
    {
        M1_IA1_LOW();
        M1_IA2_LOW();
        TIM_SetCompare3(TIM4, 0);
    }
}

static void Moto_ApplyM2HW(int speed)
{
    if (speed > 0)
    {
        M2_IB1_HIGH();
        M2_IB2_LOW();
        TIM_SetCompare4(TIM4, (u16)speed);
    }
    else if (speed < 0)
    {
        M2_IB1_LOW();
        M2_IB2_HIGH();
        TIM_SetCompare4(TIM4, (u16)(-speed));
    }
    else
    {
        M2_IB1_LOW();
        M2_IB2_LOW();
        TIM_SetCompare4(TIM4, 0);
    }
}

/**************************************************************************
 * Moto_GPIO_Init — 初始化 TB6612 方向引脚 + STBY
 **************************************************************************/
void Moto_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    /* PB1(IA1), PB5(IA2) */
    GPIO_InitStructure.GPIO_Pin = MOTO_M1_IA1_PIN | MOTO_M1_IA2_PIN;
    GPIO_Init(MOTO_M1_IA1_PORT, &GPIO_InitStructure);

    /* PB14(IB1), PB15(IB2) */
    GPIO_InitStructure.GPIO_Pin = MOTO_M2_IB1_PIN | MOTO_M2_IB2_PIN;
    GPIO_Init(MOTO_M2_IB1_PORT, &GPIO_InitStructure);

    /* PB13(STBY) */
    GPIO_InitStructure.GPIO_Pin = MOTO_STBY_PIN;
    GPIO_Init(MOTO_STBY_PORT, &GPIO_InitStructure);

    /* 初始全部拉低，STBY 使能 */
    M1_IA1_LOW(); M1_IA2_LOW();
    M2_IB1_LOW(); M2_IB2_LOW();
    MOTO_STBY_EN();

    /* 初始化斜坡 */
    ramp_m1.current  = 0;
    ramp_m1.target   = 0;
    ramp_m1.last_dwt = DWT_GetTick();

    ramp_m2.current  = 0;
    ramp_m2.target   = 0;
    ramp_m2.last_dwt = DWT_GetTick();
}

/**************************************************************************
 * Moto_PWM_Init — TIM4 CH3(PB8) + CH4(PB9) PWM 初始化
 * PWM 频率 = 72MHz / (psc+1) / (arr+1)
 * 例: arr=7199, psc=0 → 10kHz
 **************************************************************************/
void Moto_PWM_Init(u16 arr, u16 psc)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* 使能 GPIOB 和 TIM4 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    /* PB8(TIM4_CH3), PB9(TIM4_CH4) — 复用推挽 */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 时基 */
    TIM_TimeBaseStructure.TIM_Period        = arr;
    TIM_TimeBaseStructure.TIM_Prescaler     = psc;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    /* PWM 输出 CH3 + CH4 */
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse       = 0;

    TIM_OC3Init(TIM4, &TIM_OCInitStructure);
    TIM_OC4Init(TIM4, &TIM_OCInitStructure);

    TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM4, ENABLE);

    TIM_CtrlPWMOutputs(TIM4, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
}

/**************************************************************************
 * Moto_RampUpdate — 斜坡更新（主循环每轮调用）
 * 用 DWT 周期计数器计算时间差，以 5000 单位/秒的速率平滑过渡
 **************************************************************************/
void Moto_RampUpdate(void)
{
    u32 now = DWT_GetTick();
    u32 elapsed_us;
    int max_step, diff;

    /* ---- M1 ---- */
    elapsed_us = (now - ramp_m1.last_dwt) / 72;
    if (elapsed_us > 0)
    {
        ramp_m1.last_dwt = now;
        max_step = (int)((u32)MOTO_RAMP_RATE * elapsed_us / 1000000);
        if (max_step < 1) max_step = 1;

        diff = ramp_m1.target - ramp_m1.current;
        if (diff > max_step)
            ramp_m1.current += max_step;
        else if (diff < -max_step)
            ramp_m1.current -= max_step;
        else
            ramp_m1.current = ramp_m1.target;

        ramp_m1.current = Moto_Clamp(ramp_m1.current, -SPEED_MAX, SPEED_MAX);
        Moto_ApplyM1HW(ramp_m1.current);
    }

    /* ---- M2 ---- */
    elapsed_us = (now - ramp_m2.last_dwt) / 72;
    if (elapsed_us > 0)
    {
        ramp_m2.last_dwt = now;
        max_step = (int)((u32)MOTO_RAMP_RATE * elapsed_us / 1000000);
        if (max_step < 1) max_step = 1;

        diff = ramp_m2.target - ramp_m2.current;
        if (diff > max_step)
            ramp_m2.current += max_step;
        else if (diff < -max_step)
            ramp_m2.current -= max_step;
        else
            ramp_m2.current = ramp_m2.target;

        ramp_m2.current = Moto_Clamp(ramp_m2.current, -SPEED_MAX, SPEED_MAX);
        Moto_ApplyM2HW(ramp_m2.current);
    }
}

/**************************************************************************
 * 目标速度设置（只设 target，Moto_RampUpdate 负责平滑执行）
 **************************************************************************/
void Moto_SetM1Speed(int speed) { ramp_m1.target = Moto_Clamp(speed, -SPEED_MAX, SPEED_MAX); }
void Moto_SetM2Speed(int speed) { ramp_m2.target = Moto_Clamp(speed, -SPEED_MAX, SPEED_MAX); }

/**************************************************************************
 * 运动控制函数
 **************************************************************************/
void Forward(int Speed)    { Moto_SetM1Speed(Speed);  Moto_SetM2Speed(Speed);  }
void Backward(int Speed)   { Moto_SetM1Speed(-Speed); Moto_SetM2Speed(-Speed); }
void Turnleft(int Speed)   { Moto_SetM1Speed(Speed);  Moto_SetM2Speed(0);      }
void Turnright(int Speed)  { Moto_SetM1Speed(0);      Moto_SetM2Speed(Speed);  }
void SpinLeft(int Speed)   { Moto_SetM1Speed(Speed);  Moto_SetM2Speed(-Speed); }
void SpinRight(int Speed)  { Moto_SetM1Speed(-Speed); Moto_SetM2Speed(Speed);  }

/**************************************************************************
 * Stop — 立即停止（不走斜坡）+ 重置斜坡状态
 **************************************************************************/
void Stop(void)
{
    Moto_ApplyM1HW(0);
    Moto_ApplyM2HW(0);
    ramp_m1.current = 0; ramp_m1.target = 0;
    ramp_m2.current = 0; ramp_m2.target = 0;
}

/**************************************************************************
 * Moto_Brake — 立即刹车（IN1=IN2=1 短接制动）
 **************************************************************************/
void Moto_Brake(void)
{
    M1_IA1_HIGH(); M1_IA2_HIGH();
    M2_IB1_HIGH(); M2_IB2_HIGH();
    TIM_SetCompare3(TIM4, 0);
    TIM_SetCompare4(TIM4, 0);
    ramp_m1.current = 0; ramp_m1.target = 0;
    ramp_m2.current = 0; ramp_m2.target = 0;
}
