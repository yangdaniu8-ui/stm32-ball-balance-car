#include "encoder.h"

/**************************************************************************
 * 函数功能：初始化 TIM2 为编码器接口模式（M1 电机编码器）
 *          M1A → PA0 (TIM2_CH1), M1B → PA1 (TIM2_CH2)
 *          编码器模式 3：同时计数 TI1 和 TI2 的边沿（4倍频）
 **************************************************************************/
void Encoder_Init_M1(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能时钟 */
    RCC_APB1PeriphClockCmd(ENC_M1_RCC_APB, ENABLE);
    RCC_APB2PeriphClockCmd(ENC_M1_RCC_GPIO, ENABLE);

    /* PA0, PA1 — 浮空输入（编码器模式要求） */
    GPIO_InitStructure.GPIO_Pin   = ENC_M1_PIN_A | ENC_M1_PIN_B;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(ENC_M1_PORT, &GPIO_InitStructure);

    /* 时基配置 */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler     = 0;                /* 不分频 */
    TIM_TimeBaseStructure.TIM_Period        = ENC_MAX_COUNT;    /* 自动重装值 65535 */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(ENC_M1_TIM, &TIM_TimeBaseStructure);

    /* 编码器模式：TI1+TI2 双边沿计数（4倍频） */
    TIM_EncoderInterfaceConfig(ENC_M1_TIM,
        TIM_EncoderMode_TI12,
        TIM_ICPolarity_Rising,
        TIM_ICPolarity_Rising);

    /* 输入捕获滤波 — 消除抖动 */
    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_ICFilter = 6;   /* 滤波系数 6，平衡响应速度和稳定性 */
    TIM_ICInit(ENC_M1_TIM, &TIM_ICInitStructure);

    /* 清零计数，使能定时器 */
    TIM_SetCounter(ENC_M1_TIM, 0);
    TIM_Cmd(ENC_M1_TIM, ENABLE);
}

/**************************************************************************
 * 函数功能：初始化 TIM4 为编码器接口模式（M2 电机编码器）
 *          M2A → PB6 (TIM4_CH1), M2B → PB7 (TIM4_CH2)
 *          编码器模式 3：同时计数 TI1 和 TI2 的边沿（4倍频）
 **************************************************************************/
void Encoder_Init_M2(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能时钟 */
    RCC_APB1PeriphClockCmd(ENC_M2_RCC_APB, ENABLE);
    RCC_APB2PeriphClockCmd(ENC_M2_RCC_GPIO, ENABLE);

    /* PB6, PB7 — 浮空输入 */
    GPIO_InitStructure.GPIO_Pin   = ENC_M2_PIN_A | ENC_M2_PIN_B;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(ENC_M2_PORT, &GPIO_InitStructure);

    /* 时基配置 */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler     = 0;
    TIM_TimeBaseStructure.TIM_Period        = ENC_MAX_COUNT;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(ENC_M2_TIM, &TIM_TimeBaseStructure);

    /* 编码器模式 */
    TIM_EncoderInterfaceConfig(ENC_M2_TIM,
        TIM_EncoderMode_TI12,
        TIM_ICPolarity_Rising,
        TIM_ICPolarity_Rising);

    /* 输入滤波 */
    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_ICFilter = 6;
    TIM_ICInit(ENC_M2_TIM, &TIM_ICInitStructure);

    /* 清零计数，使能定时器 */
    TIM_SetCounter(ENC_M2_TIM, 0);
    TIM_Cmd(ENC_M2_TIM, ENABLE);
}

/**************************************************************************
 * 函数功能：读取编码器当前计数值（不清零）
 * 入口参数：motor — ENC_MOTOR_M1 或 ENC_MOTOR_M2
 * 返回值  ：编码器计数值（有符号，正转>0，反转<0）
 **************************************************************************/
int Encoder_GetCount(u8 motor)
{
    int count = 0;

    if (motor == ENC_MOTOR_M1)
    {
        count = (short)TIM2->CNT;
    }
    else if (motor == ENC_MOTOR_M2)
    {
        count = (short)TIM4->CNT;
    }

    return count;
}

/**************************************************************************
 * 函数功能：读取编码器计数值并清零
 * 入口参数：motor — ENC_MOTOR_M1 或 ENC_MOTOR_M2
 * 返回值  ：清零前的计数值
 **************************************************************************/
int Encoder_ReadAndClear(u8 motor)
{
    int count = Encoder_GetCount(motor);

    if (motor == ENC_MOTOR_M1)
    {
        TIM2->CNT = 0;
    }
    else if (motor == ENC_MOTOR_M2)
    {
        TIM4->CNT = 0;
    }

    return count;
}

/**************************************************************************
 * 函数功能：清零编码器计数
 * 入口参数：motor — ENC_MOTOR_M1 或 ENC_MOTOR_M2
 **************************************************************************/
void Encoder_ClearCount(u8 motor)
{
    if (motor == ENC_MOTOR_M1)
    {
        TIM2->CNT = 0;
    }
    else if (motor == ENC_MOTOR_M2)
    {
        TIM4->CNT = 0;
    }
}

/**************************************************************************
 * 函数功能：计算电机转速（单位：RPM / 转每分钟）
 * 入口参数：motor       — ENC_MOTOR_M1 或 ENC_MOTOR_M2
 *           ppr         — 编码器线数（每圈脉冲数，4倍频前的值）
 *           interval_ms — 采样间隔（毫秒）
 * 返回值  ：转速 RPM，正值正转，负值反转
 *
 * 计算公式：RPM = (count / (ppr * 4)) / (interval_ms / 60000)
 *               = count * 60000 / (ppr * 4 * interval_ms)
 * 说明：编码器模式使用 4 倍频，所以实际每圈脉冲 = ppr * 4
 **************************************************************************/
float Encoder_GetRPM(u8 motor, u16 ppr, u16 interval_ms)
{
    int count = Encoder_ReadAndClear(motor);
    float rpm;

    /* 避免除零 */
    if (ppr == 0 || interval_ms == 0)
        return 0.0f;

    /* count * 60000 / (ppr * 4 * interval_ms) */
    rpm = (float)count * 60000.0f / ((float)ppr * 4.0f * (float)interval_ms);

    return rpm;
}
