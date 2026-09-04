#include "stm32f10x.h"
#include "linewalking.h"
#include "sys.h"
#include "moto.h"
#include "delay.h"

extern int g_Speed;

/**
 * Function       LineWalking_GPIO_Init
 * @brief         四路巡线传感器 GPIO 初始化（上拉输入）
 */
void LineWalking_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

#ifdef USE_LINE_L1
    RCC_APB2PeriphClockCmd(LineWalk_L1_RCC, ENABLE);
    GPIO_InitStructure.GPIO_Pin   = LineWalk_L1_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LineWalk_L1_PORT, &GPIO_InitStructure);
#endif

#ifdef USE_LINE_L2
    RCC_APB2PeriphClockCmd(LineWalk_L2_RCC, ENABLE);
    GPIO_InitStructure.GPIO_Pin   = LineWalk_L2_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LineWalk_L2_PORT, &GPIO_InitStructure);
#endif

#ifdef USE_LINE_R1
    RCC_APB2PeriphClockCmd(LineWalk_R1_RCC, ENABLE);
    GPIO_InitStructure.GPIO_Pin   = LineWalk_R1_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LineWalk_R1_PORT, &GPIO_InitStructure);
#endif

#ifdef USE_LINE_R2
    RCC_APB2PeriphClockCmd(LineWalk_R2_RCC, ENABLE);
    GPIO_InitStructure.GPIO_Pin   = LineWalk_R2_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LineWalk_R2_PORT, &GPIO_InitStructure);
#endif
}

/**
 * Function       GetLineWalking
 * @brief         读取四路传感器值（0=黑线, 1=白底）
 */
void GetLineWalking(int *p_iL1, int *p_iL2, int *p_iR1, int *p_iR2)
{
    *p_iL1 = GPIO_ReadInputDataBit(LineWalk_L1_PORT, LineWalk_L1_PIN);
    *p_iL2 = GPIO_ReadInputDataBit(LineWalk_L2_PORT, LineWalk_L2_PIN);
    *p_iR1 = GPIO_ReadInputDataBit(LineWalk_R1_PORT, LineWalk_R1_PIN);
    *p_iR2 = GPIO_ReadInputDataBit(LineWalk_R2_PORT, LineWalk_R2_PIN);
}

/**
 * Function       Check_StartLine
 * @brief         检测启停线（四路全黑 = 粗横线）
 * @retval        1=检测到, 0=未检测到
 */
u8 Check_StartLine(void)
{
    static u8 confirm_cnt = 0;
    int L1, L2, R1, R2;

    GetLineWalking(&L1, &L2, &R1, &R2);

    if (L1 == LOW && L2 == LOW && R1 == LOW && R2 == LOW)
    {
        confirm_cnt++;
        if (confirm_cnt >= STARTLINE_CONFIRM)
        {
            confirm_cnt = 0;
            return 1;
        }
    }
    else
    {
        if (confirm_cnt > 0) confirm_cnt--;
    }
    return 0;
}

/**************************************************************************
 * LineWalking_PID — PID 巡线控制
 *
 * 传感器权重: L1=-3, L2=-1, R1=+1, R2=+3
 * position < 0 → 线偏左 → 右转（左轮加速/右轮减速）
 * position > 0 → 线偏右 → 左转（右轮加速/左轮减速）
 *
 * PID 说明:
 *   P — 比例项: 立即响应位置偏差，决定转弯强度
 *   I — 积分项: 消除弯道稳态误差（持续偏移时逐渐加大修正）
 *   D — 微分项: 预测趋势，入弯时提前加大修正，抑制出弯后震荡
 *
 * 调参指南（实地测试时修改 linewalking.h）:
 *   - 弯道转不过来 → 增大 LINE_KP / LINE_KI
 *   - 直道 S 形摇摆 → 增大 LINE_KD，减小 LINE_KP
 *   - 弯道甩出去     → 增大 LINE_TURN_MIN，或降低速度
 **************************************************************************/
void LineWalking_PID(int baseSpeed)
{
    int L1, L2, R1, R2;
    int position;
    int P, D, correction;
    int speedM1, speedM2;
    int max_corr;
    static int last_position = 0;
    static int integral = 0;

    GetLineWalking(&L1, &L2, &R1, &R2);

    /* === 计算位置偏差（-3 ~ +3）=== */
    position  = (L1 == LOW) ? -3 : 0;
    position += (L2 == LOW) ? -1 : 0;
    position += (R1 == LOW) ? +1 : 0;
    position += (R2 == LOW) ? +3 : 0;

    /* === P: 比例项 === */
    P = LINE_KP * position;

    /* === I: 积分项（抗饱和）=== */
    integral += position;
    if (integral > LINE_I_MAX)  integral = LINE_I_MAX;
    if (integral < -LINE_I_MAX) integral = -LINE_I_MAX;
    if (position == 0)
        integral = integral * 3 / 4;

    /* === D: 微分项（变化率）=== */
    D = LINE_KD * (position - last_position);
    last_position = position;

    /* === 合成修正量 + 重量偏置 === */
    correction = P + (LINE_KI * integral / 10) + D + LINE_BIAS;

    /* 限幅 */
    max_corr = baseSpeed + LINE_TURN_MIN;
    if (correction > max_corr)  correction = max_corr;
    if (correction < -max_corr) correction = -max_corr;

    /* === 计算左右轮速度 === */
    speedM1 = baseSpeed + correction;   /* 右后轮 */
    speedM2 = baseSpeed - correction;   /* 左后轮 */

    /* 限幅 */
    if (speedM1 > SPEED_MAX) speedM1 = SPEED_MAX;
    if (speedM1 < -800)      speedM1 = -800;
    if (speedM2 > SPEED_MAX) speedM2 = SPEED_MAX;
    if (speedM2 < -800)      speedM2 = -800;

    /* === 输出 EMA 平滑滤波（减少电机突变）=== */
    {
        static int filt_M1 = 0, filt_M2 = 0;
        static u8 first = 1;
        if (first) { filt_M1 = speedM1; filt_M2 = speedM2; first = 0; }
        filt_M1 = (filt_M1 * 3 + speedM1) / 4;   /* alpha=0.25 */
        filt_M2 = (filt_M2 * 3 + speedM2) / 4;
        Moto_SetM1Speed(filt_M1);
        Moto_SetM2Speed(filt_M2);
    }
}

/**
 * Function       LineWalking
 * @brief         兼容旧接口
 */
void LineWalking(void)
{
    LineWalking_PID(g_Speed);
}
