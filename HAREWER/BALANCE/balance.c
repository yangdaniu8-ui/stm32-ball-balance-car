#include "balance.h"
#include "delay.h"
#include <stdlib.h>

extern volatile u32 g_SysTick;

volatile float g_BallTarget_cm  = 0.0f;
volatile float g_BallCurrent_cm = 0.0f;
volatile u8    g_BallDataReady  = 0;
volatile u16   g_UART_RxCount   = 0;
volatile u16   g_PB7_ToggleCount = 0;
volatile char  g_LastRxByte     = 0;
int g_ServoCenterPWM = SERVO_PWM_CENTER;
volatile float g_AccelFF = 0.0f;

static char rx_buf[16];
static u8   rx_idx = 0;

/* ---- 处理收到的字节 ---- */
static void ProcessRxChar(char ch)
{
    g_UART_RxCount++;
    g_LastRxByte = ch;
    if (ch == '\n' || ch == '\r')
    {
        if (rx_idx > 0)
        {
            rx_buf[rx_idx] = '\0';
            rx_idx = 0;
            g_BallCurrent_cm = atof(rx_buf);
            g_BallDataReady  = 1;
        }
    }
    else if (rx_idx < sizeof(rx_buf) - 1)
    {
        rx_buf[rx_idx++] = ch;
    }
}

/**************************************************************************
 * USART1 — PB6/PB7 重映射, 115200, 参照测试代码写法
 **************************************************************************/
static void K230D_UART_Init(void)
{
    GPIO_InitTypeDef g;
    USART_InitTypeDef u;
    NVIC_InitTypeDef n;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);

    /* USART1 重映射到 PB6(TX) PB7(RX) */
    GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

    /* PB6 TX — 复用推挽 */
    g.GPIO_Pin   = GPIO_Pin_6;
    g.GPIO_Mode  = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &g);

    /* PB7 RX — 上拉输入 */
    g.GPIO_Pin   = GPIO_Pin_7;
    g.GPIO_Mode  = GPIO_Mode_IPU;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &g);

    u.USART_BaudRate            = 115200;
    u.USART_WordLength          = USART_WordLength_8b;
    u.USART_StopBits            = USART_StopBits_1;
    u.USART_Parity              = USART_Parity_No;
    u.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    u.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &u);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    n.NVIC_IRQChannel                   = USART1_IRQn;
    n.NVIC_IRQChannelPreemptionPriority = 0;
    n.NVIC_IRQChannelSubPriority        = 0;
    n.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&n);

    USART_Cmd(USART1, ENABLE);
}

/**************************************************************************
 * USART1 中断
 **************************************************************************/
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        char ch = (char)USART_ReceiveData(USART1);
        ProcessRxChar(ch);
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

/**************************************************************************
 * TIM1 CH1 (PA8) — 50Hz 舵机 PWM，直接写寄存器
 *
 * 弃用库函数，避免任何潜在的寄存器误写。
 * 72MHz / 72 = 1MHz → ARR=20000 → 50Hz
 **************************************************************************/
static void Servo_PWM_Init(void)
{
    /* 1. 时钟 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_TIM1EN;

    /* 2. PA8 复用推挽, 50MHz */
    GPIOA->CRH &= ~0xF;           /* 清 PA8 配置 */
    GPIOA->CRH |= 0xB;            /* 50MHz AF_PP */

    /* 3. 复位 TIM1 */
    RCC->APB2RSTR |= RCC_APB2RSTR_TIM1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_TIM1RST;

    /* 4. 时基: 1MHz, 50Hz */
    TIM1->PSC = 72 - 1;
    TIM1->ARR = 20000 - 1;
    TIM1->CR1 = TIM_CR1_ARPE;     /* 自动重载预装载 + 向上计数 */

    /* 5. CH1 PWM 模式1, 使能预装载 */
    TIM1->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1  /* PWM mode 1 */
                | TIM_CCMR1_OC1PE;                       /* 预装载使能 */
    TIM1->CCER  = TIM_CCER_CC1E;                         /* CH1 输出使能 */
    TIM1->CCR1  = g_ServoCenterPWM;

    /* 6. 高级定时器: 使能主输出 + 启动 */
    TIM1->BDTR  = TIM_BDTR_MOE;
    TIM1->EGR   = TIM_EGR_UG;     /* 生成更新事件，加载影子寄存器 */
    TIM1->CR1  |= TIM_CR1_CEN;    /* 启动计数器 */
}

/**************************************************************************
 * Balance_Init
 **************************************************************************/
void Balance_Init(void)
{
    __enable_irq();
    Servo_PWM_Init();
    K230D_UART_Init();

    Balance_SetServo(0.0f);
    delay_ms(500);
}

/**************************************************************************
 * Balance_SetServo / Balance_SetRawPWM
 **************************************************************************/
void Balance_SetServo(float angle)
{
    float pwm;
    if (angle >  SERVO_MAX_ANGLE) angle =  SERVO_MAX_ANGLE;
    if (angle < -SERVO_MAX_ANGLE) angle = -SERVO_MAX_ANGLE;
    pwm = (float)g_ServoCenterPWM + angle * (SERVO_PWM_MAX - SERVO_PWM_MIN) / 180.0f;
    if (pwm > SERVO_PWM_MAX) pwm = SERVO_PWM_MAX;
    if (pwm < SERVO_PWM_MIN) pwm = SERVO_PWM_MIN;
    TIM1->CCR1 = (u16)pwm;
}

void Balance_SetRawPWM(u16 pwm)
{
    if (pwm > SERVO_PWM_MAX) pwm = SERVO_PWM_MAX;
    if (pwm < SERVO_PWM_MIN) pwm = SERVO_PWM_MIN;
    TIM1->CCR1 = pwm;
}

/**************************************************************************
 * Balance_UART_Poll — 轮询 USART1 RX
 **************************************************************************/
void Balance_UART_Poll(void)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
    {
        ProcessRxChar((char)USART_ReceiveData(USART1));
    }
}

/**************************************************************************
 * Balance_GPIO_Monitor — GPIO 监听 PB7
 **************************************************************************/
void Balance_GPIO_Monitor(void)
{
    static u8 last = 1;
    u8 cur = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7);
    if (cur != last) { g_PB7_ToggleCount++; last = cur; }
}

/**************************************************************************
 * Balance_PID_Update — 串级 PID（位置环→速度环→舵机）
 *
 * 外环: 位置误差 → 期望速度 (cm/s)
 * 内环: 速度误差 → 舵机角度 (°)
 *
 * 速度按 K230D 数据真实到达间隔计算
 **************************************************************************/
void Balance_PID_Update(void)
{
    static u32   last_tick       = 0;
    static float prev_filtered   = 0.0f;
    static float vel_integral    = 0.0f;
    static float pos_integral    = 0.0f;
    static u8    first_run       = 1;

    float dt, filtered, ball_velocity;
    float pos_error, vel_target, vel_error;
    float output;
    u32 now_tick;

    if (first_run)
    {
        last_tick      = g_SysTick;
        prev_filtered  = g_BallCurrent_cm;
        first_run      = 0;
        return;
    }

    now_tick = g_SysTick;

    /* ---- 1. EMA 低通滤波 ---- */
    filtered = BAL_FILTER_ALPHA * g_BallCurrent_cm
             + (1.0f - BAL_FILTER_ALPHA) * prev_filtered;

    /* ---- 2. 钢球速度估计 (cm/s) — 按数据到达间隔计算 ---- */
    {
        static float last_pos   = 0.0f;
        static u32   last_tick  = 0;
        static float vel_filt   = 0.0f;
        static u8    vel_init   = 1;

        if (vel_init) { last_pos = filtered; last_tick = now_tick; vel_init = 0; }

        {
            float diff = filtered - last_pos;
            float abs_diff = (diff > 0) ? diff : -diff;
            if (abs_diff > 0.01f)
            {
                u32 data_dt = now_tick - last_tick;
                if (data_dt > 5)
                {
                    float raw_vel = diff * 1000.0f / (float)data_dt;
                    vel_filt = 0.5f * raw_vel + 0.5f * vel_filt;
                }
                last_pos  = filtered;
                last_tick = now_tick;
            }
        }
        ball_velocity = vel_filt;
    }
    prev_filtered = filtered;

    /* ---- dt ---- */
    dt = (float)(now_tick - last_tick) / 1000.0f;
    if (dt < 0.001f) dt = 0.001f;
    if (dt > 0.1f)   dt = 0.1f;
    last_tick = now_tick;

    /* ================================================================
     * 外环 — 位置环: 目标位置 → 期望速度
     * ================================================================ */
    pos_error = g_BallTarget_cm - filtered;

    if (pos_error > -BAL_DEADZONE && pos_error < BAL_DEADZONE)
        pos_error = 0.0f;

    if (pos_error > 0.5f || pos_error < -0.5f)
    {
        pos_integral += pos_error * dt;
        if (pos_integral >  POS_INTEGRAL_MAX) pos_integral =  POS_INTEGRAL_MAX;
        if (pos_integral < -POS_INTEGRAL_MAX) pos_integral = -POS_INTEGRAL_MAX;
    }
    else
    {
        pos_integral *= 0.9f;
    }

    vel_target = POS_KP * pos_error + POS_KI * pos_integral;

    if (vel_target >  VEL_OUTPUT_MAX) vel_target =  VEL_OUTPUT_MAX;
    if (vel_target < -VEL_OUTPUT_MAX) vel_target = -VEL_OUTPUT_MAX;

    /* ================================================================
     * 内环 — 速度环: 期望速度 → 舵机角度
     * ================================================================ */
    vel_error = vel_target - ball_velocity;

    vel_integral += vel_error * dt;
    if (vel_integral >  VEL_INTEGRAL_MAX) vel_integral =  VEL_INTEGRAL_MAX;
    if (vel_integral < -VEL_INTEGRAL_MAX) vel_integral = -VEL_INTEGRAL_MAX;

    output = VEL_KP * vel_error + VEL_KI * vel_integral - VEL_KD * ball_velocity;

    /* 静摩擦力起步助推 */
    {
        float abs_err = (pos_error > 0) ? pos_error : -pos_error;
        float abs_vel = (ball_velocity > 0) ? ball_velocity : -ball_velocity;
        if (abs_err > 1.0f && abs_vel < 0.3f)
            output += (pos_error > 0) ? 2.0f : -2.0f;
    }

    if (output >  SERVO_MAX_ANGLE) output =  SERVO_MAX_ANGLE;
    if (output < -SERVO_MAX_ANGLE) output = -SERVO_MAX_ANGLE;

#if SERVO_INVERT
    output = -output;
#endif

    output += SERVO_BIAS;
    output += g_AccelFF;    /* 加速度前馈 */

    /* 输出变化率限制 */
    {
        static float last_output = 0.0f;
        float delta = output - last_output;
        if (delta >  BAL_RAMP_MAX) delta =  BAL_RAMP_MAX;
        if (delta < -BAL_RAMP_MAX) delta = -BAL_RAMP_MAX;
        output = last_output + delta;
        last_output = output;
    }

    Balance_SetServo(output);
}
