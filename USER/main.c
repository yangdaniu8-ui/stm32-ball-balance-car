/**
 * @file    main.c
 * @brief   车载平衡滚球运动控制系统（H题）
 *
 * 赛题模式:
 *   MODE 0 — 要求2: 单车巡线一圈，自动停车计时 (≤20s, 偏差≤2cm)
 *   MODE 1 — 要求3: 静止钢球往返测试 (+5cm→-5cm, ≤5s)
 *   MODE 2 — 要求4: AB段巡线+钢球平衡 (≤8s, 球偏差≤1cm)
 *   MODE 3 — 要求5: 整圈巡线+钢球平衡 (≤30s, 球偏差≤1cm)
 *   MODE 4 — 要求6: 整圈+球在任意指定位置 (≤30s, 偏差≤1cm)
 */

#include "stm32f10x.h"
#include "delay.h"
#include "bsp.h"
#include "linewalking.h"
#include "oled.h"
#include "key.h"

/* ================================================================
 * 状态机
 * ================================================================ */
typedef enum {
    STATE_IDLE = 0,
    STATE_COUNTDOWN,
    STATE_RUNNING,
    STATE_FINISHED
} CarState_t;

static CarState_t g_State         = STATE_IDLE;
static u32 g_LapStartTick         = 0;
static u32 g_LapTimeMs            = 0;
static u32 g_BestLapMs            = 0;
static u8  g_CurrentLap           = 0;
static u8  g_LineCleared          = 0;
static u8  g_SlowDown             = 0;    /* 减速标志 */
static u32 g_LastOledTick         = 0;
static u8  g_CalibMode            = 0;
extern volatile float g_AccelFF;

/* ================================================================
 * 赛题模式 (MODE 按键切换)
 *   0=要求2巡线  1=要求3静态  2=要求4AB段  3=要求5整圈  4=要求6任意
 * ================================================================ */
static u8  g_TaskMode             = 0;

/* ---- 要求参数 ---- */
#define TASK2_MAX_TIME      20000   /* 要求2: ≤20s */
#define TASK2_TARGET_LAPS   1       /* 要求2: 1圈 */
#define TASK4_MAX_TIME      8000    /* 要求4: AB≤8s */
#define TASK5_MAX_TIME      30000   /* 要求5: ≤30s */

/* ================================================================
 * 前向声明
 * ================================================================ */
static void Key_Handler(void);
static void OLED_Display(void);
static void Speed_Adjust(int delta);
static u32  GetLapTimeMs(void);

/* ================================================================
 * main
 * ================================================================ */
int main(void)
{
    bsp_init();

    OLED_Clear();
    OLED_ShowString(0, 0,  "H-Ball Car", 16);
    OLED_ShowString(0, 20, "Task2: Line", 12);
    OLED_ShowString(0, 36, "Press START", 12);
    OLED_Refresh();

    while (1)
    {
        /* ---- 电机斜坡 ---- */
        Moto_RampUpdate();

        /* ---- K230D 串口轮询 ---- */
        Balance_UART_Poll();

        /* ---- 钢球平衡 (模式 1~4, 仅 RUNNING 状态) ---- */
        if (!g_CalibMode && g_TaskMode >= 1 && g_State == STATE_RUNNING)
        {
            Balance_PID_Update();
        }
        else if (!g_CalibMode && g_State == STATE_IDLE)
        {
            /* IDLE 时舵机保持水平 */
            Balance_SetServo(0.0f);
        }

        /* ---- 按键 ---- */
        Key_Handler();

        /* ---- 状态机 ---- */
        switch (g_State)
        {
        case STATE_IDLE:
            Stop();
            break;

        case STATE_COUNTDOWN:
        {
            static u32 cd_start = 0;
            static u8  cd_last  = 99;
            u8 cd_remain;

            if (cd_start == 0) {
                cd_start = g_SysTick;
                cd_last  = 99;
                Stop();
            }

            cd_remain = 3 - (u8)((g_SysTick - cd_start) / 1000);
            if (cd_remain >= 3) cd_remain = 3;

            if (cd_remain != cd_last) {
                cd_last = cd_remain;
                OLED_Clear();
                if (cd_remain > 0)
                    OLED_ShowNum(50, 20, cd_remain, 1, 16);
                else
                    OLED_ShowString(24, 20, "GO!", 16);
                OLED_Refresh();
            }

            if (cd_remain == 0) {
                g_State          = STATE_RUNNING;
                g_LapStartTick   = g_SysTick;
                g_CurrentLap     = 0;
                g_LineCleared    = 0;
                g_SlowDown       = 0;
                cd_start         = 0;
            }
            break;
        }

        case STATE_RUNNING:
        {
            u32 now_ms = GetLapTimeMs();

            if (g_TaskMode == 1)
            {
                /* 要求3: 静止钢球往返 O→+5cm→-5cm, ≤5s */
                if (now_ms < 2500)
                    g_BallTarget_cm = 5.0f;
                else
                    g_BallTarget_cm = -5.0f;

                if (now_ms >= 5000)
                {
                    g_LapTimeMs = now_ms;
                    g_BallTarget_cm = 0.0f;
                    g_State = STATE_FINISHED;
                }
            }
            else
            {
                u32 stop_ms;
                int run_speed = g_Speed;

                /* 各模式停车时间 */
                switch (g_TaskMode)
                {
                case 0:  stop_ms = 16600;  break;   /* 要求2: 一圈 */
                case 2:  stop_ms =  8000;  break;   /* 要求4: AB段 ≤8s */
                case 3:  stop_ms = 28000;  break;   /* 要求5: 整圈+球 */
                case 4:  stop_ms = 28000;  break;   /* 要求6: 整圈+任意 */
                default: stop_ms = 16600;  break;
                }

                /* 起步缓升: 前 3s 从 0 线性到全速，无起步冲击 */
                if (now_ms < 3000)
                {
                    run_speed = g_Speed * (int)now_ms / 3000;
                    g_AccelFF = 1.2f;   /* 加速度前馈: 管前倾接住球 */
                }
                else
                {
                    g_AccelFF = 0.0f;
                }

                if (now_ms >= stop_ms)
                {
                    g_LapTimeMs = now_ms;
                    Stop();
                    g_State = STATE_FINISHED;
                    if (g_BestLapMs == 0 || g_LapTimeMs < g_BestLapMs)
                        g_BestLapMs = g_LapTimeMs;
                }
                else
                {
                    LineWalking_PID(run_speed);
                }
            }
            break;
        }

        case STATE_FINISHED:
            Stop();
            break;
        }

        /* ---- OLED 刷新 (200ms) ---- */
        if (g_SysTick - g_LastOledTick >= 200)
        {
            g_LastOledTick = g_SysTick;
            OLED_Display();
        }
    }
}

/* ================================================================
 * GetLapTimeMs
 * ================================================================ */
static u32 GetLapTimeMs(void)
{
    if (g_LapStartTick == 0) return 0;
    return g_SysTick - g_LapStartTick;
}

/* ================================================================
 * Key_Handler
 *
 * START:  IDLE→倒计时 / FINISHED→复位IDLE / 长按紧急停
 * UP:    调速 (IDLE)
 * DOWN:  调速 (IDLE)
 * MODE:  切换赛题模式 (IDLE)
 * ================================================================ */
static void Key_Handler(void)
{
    u8 key;

    /* ---- START ---- */
    key = Key_GetState(KEY_ID_START);
    if (key == KEY_STATE_SHORT)
    {
        if (g_CalibMode) { g_CalibMode = 0; return; }

        switch (g_State)
        {
        case STATE_IDLE:
            g_State = STATE_COUNTDOWN;
            break;
        case STATE_FINISHED:
            g_State         = STATE_IDLE;
            g_LapStartTick  = 0;
            g_CurrentLap    = 0;
            g_LineCleared   = 0;
            g_SlowDown      = 0;
            if (g_TaskMode != 4) g_BallTarget_cm = 0.0f;
            break;
        case STATE_COUNTDOWN:
            g_State = STATE_IDLE;
            break;
        default: break;
        }
    }
    else if (key == KEY_STATE_LONG && g_State == STATE_RUNNING)
    {
        g_State         = STATE_IDLE;
        g_LapStartTick  = 0;
        g_LapTimeMs     = 0;
        g_CurrentLap    = 0;
        g_LineCleared   = 0;
        g_SlowDown      = 0;
        if (g_TaskMode != 4) g_BallTarget_cm = 0.0f;
        Stop();
        Balance_SetServo(0.0f);
    }

    /* ---- 校准模式 ---- */
    if (g_CalibMode)
    {
        key = Key_GetState(KEY_ID_UP);
        if (key == KEY_STATE_SHORT)
            { g_ServoCenterPWM += 10; Balance_SetRawPWM((u16)g_ServoCenterPWM); }
        else if (key == KEY_STATE_LONG || key == KEY_STATE_HOLD)
            { g_ServoCenterPWM += 5; Balance_SetRawPWM((u16)g_ServoCenterPWM); }

        key = Key_GetState(KEY_ID_DOWN);
        if (key == KEY_STATE_SHORT)
            { g_ServoCenterPWM -= 10; Balance_SetRawPWM((u16)g_ServoCenterPWM); }
        else if (key == KEY_STATE_LONG || key == KEY_STATE_HOLD)
            { g_ServoCenterPWM -= 5; Balance_SetRawPWM((u16)g_ServoCenterPWM); }
        return;
    }

    /* ---- UP/DOWN: MODE 4 调球目标位置, 其他模式调速 (仅 IDLE) ---- */
    if (g_State == STATE_IDLE)
    {
        if (g_TaskMode == 4)
        {
            key = Key_GetState(KEY_ID_UP);
            if (key == KEY_STATE_SHORT)      { g_BallTarget_cm += 1.0f; }
            else if (key == KEY_STATE_LONG)  { g_BallTarget_cm += 0.5f; }
            key = Key_GetState(KEY_ID_DOWN);
            if (key == KEY_STATE_SHORT)      { g_BallTarget_cm -= 1.0f; }
            else if (key == KEY_STATE_LONG)  { g_BallTarget_cm -= 0.5f; }
            if (g_BallTarget_cm >  10.0f) g_BallTarget_cm =  10.0f;
            if (g_BallTarget_cm < -10.0f) g_BallTarget_cm = -10.0f;
        }
        else
        {
            key = Key_GetState(KEY_ID_UP);
            if (key == KEY_STATE_SHORT) Speed_Adjust(100);
            key = Key_GetState(KEY_ID_DOWN);
            if (key == KEY_STATE_SHORT) Speed_Adjust(-100);
        }
    }

    /* ---- MODE 短按: 赛题模式 0→1→2→3→4→0 循环 ---- */
    key = Key_GetState(KEY_ID_MODE);
    if (key == KEY_STATE_SHORT && g_State == STATE_IDLE)
    {
        g_TaskMode++;
        if (g_TaskMode > 4) g_TaskMode = 0;
    }
    /* ---- MODE 长按: 进入舵机校准 ---- */
    else if (key == KEY_STATE_LONG && g_State == STATE_IDLE)
    {
        g_CalibMode = 1;
        Balance_SetRawPWM((u16)g_ServoCenterPWM);
    }
}

/* ================================================================
 * Speed_Adjust
 * ================================================================ */
static void Speed_Adjust(int delta)
{
    g_Speed += delta;
    if (g_Speed > SPEED_MAX) g_Speed = SPEED_MAX;
    if (g_Speed < SPEED_MIN) g_Speed = SPEED_MIN;
}

/* ================================================================
 * OLED_Display (每 200ms)
 *
 * 布局:
 *   y=0:  时间 + 圈数 (8x16)
 *   y=18: 速度 + 状态 + 模式 (6x8)
 *   y=30: 传感器条 (6x8)
 *   y=42: 任务信息 (6x8)
 *   y=54: 钢球 / 最佳圈速 (6x8)
 * ================================================================ */
static void OLED_Display(void)
{
    int L1 = 1, L2 = 1, R1 = 1, R2 = 1;
    int pos;
    u32 lap_ms;
    u8  pb7;

    OLED_Clear();

    /* 校准界面 */
    if (g_CalibMode)
    {
        OLED_ShowString(0, 0, "SERVO CALIB", 16);
        OLED_ShowString(0, 22, "PWM:", 12);
        OLED_ShowNum(30, 22, g_ServoCenterPWM, 4, 12);
        OLED_ShowString(0, 38, "UP/DN:+/-, START:ok", 12);
        OLED_Refresh();
        return;
    }

    GetLineWalking(&L1, &L2, &R1, &R2);

    /* ==== 第1行: 时间 + 圈数 (8x16) ==== */
    if (g_State == STATE_RUNNING)
    {
        lap_ms = GetLapTimeMs();
        OLED_ShowString(0, 0, "T:", 16);
        OLED_ShowFloat(18, 0, lap_ms / 1000.0f, 2, 1, 16);
        OLED_ShowChar(60, 0, 's', 16);
    }
    else if (g_State == STATE_FINISHED)
    {
        /* 停车后冻结时间 */
        OLED_ShowString(0, 0, "T:", 16);
        OLED_ShowFloat(18, 0, g_LapTimeMs / 1000.0f, 2, 1, 16);
        OLED_ShowChar(60, 0, 's', 16);
    }
    else
    {
        OLED_ShowString(0, 0, "H-Ball Car", 16);
    }

    /* ==== 第2行: 速度 + 状态 + 模式 ==== */
    OLED_ShowString(0, 20, "S:", 12);
    OLED_ShowNum(12, 20, g_Speed, 4, 12);

    /* 状态缩写 */
    switch (g_State)
    {
    case STATE_IDLE:      OLED_ShowString(40, 20, "IDL", 12);  break;
    case STATE_COUNTDOWN: OLED_ShowString(40, 20, "CD", 12);   break;
    case STATE_RUNNING:   OLED_ShowString(40, 20, "RUN", 12);  break;
    case STATE_FINISHED:  OLED_ShowString(40, 20, "DONE", 12); break;
    }

    /* 任务模式 */
    OLED_ShowString(70, 20, "T:", 12);
    OLED_ShowNum(82, 20, g_TaskMode, 1, 12);

    /* ==== 第3行: 传感器条 ==== */
    OLED_ShowString(0, 30, "S:[", 12);
    OLED_ShowChar(18, 30, (L1 == LOW) ? '#' : '.', 12);
    OLED_ShowChar(24, 30, (L2 == LOW) ? '#' : '.', 12);
    OLED_ShowChar(36, 30, (R1 == LOW) ? '#' : '.', 12);
    OLED_ShowChar(42, 30, (R2 == LOW) ? '#' : '.', 12);
    OLED_ShowString(50, 30, "]", 12);

    pos  = (L1 == LOW) ? -3 : 0;
    pos += (L2 == LOW) ? -1 : 0;
    pos += (R1 == LOW) ? +1 : 0;
    pos += (R2 == LOW) ? +3 : 0;
    OLED_ShowString(64, 30, "P:", 12);
    if (pos >= 0) OLED_ShowChar(76, 30, '+', 12);
    else         { OLED_ShowChar(76, 30, '-', 12); pos = -pos; }
    OLED_ShowNum(82, 30, pos, 1, 12);

    /* ==== 第4行: 任务描述 ==== */
    switch (g_TaskMode)
    {
    case 0: OLED_ShowString(0, 42, "Task2: 1Lap<20s", 12);  break;
    case 1: OLED_ShowString(0, 42, "Task3: +-5cm<5s", 12);  break;
    case 2: OLED_ShowString(0, 42, "Task4: AB<8s", 12);     break;
    case 3: OLED_ShowString(0, 42, "Task5: Lap+Ball<30s",12); break;
    case 4: OLED_ShowString(0, 42, "Task6: Ball@pos<30s",12); break;
    }

    /* ==== 第5行: 球位置 / 圈速诊断 ==== */
    pb7 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7);
    if ((g_TaskMode >= 1) && g_BallDataReady)
    {
        OLED_ShowString(0, 54, "Ball:", 12);
        OLED_ShowFloat(30, 54, g_BallCurrent_cm, 2, 1, 12);
        OLED_ShowString(56, 54, "T:", 12);
        OLED_ShowFloat(66, 54, g_BallTarget_cm, 1, 1, 12);
        OLED_ShowString(80, 54, "cm", 12);
    }
    else if (g_State == STATE_FINISHED)
    {
        OLED_ShowString(0, 54, "Time:", 12);
        OLED_ShowFloat(30, 54, g_LapTimeMs / 1000.0f, 2, 1, 12);
        OLED_ShowChar(56, 54, 's', 12);
    }
    else if (g_UART_RxCount > 0 || g_PB7_ToggleCount > 0)
    {
        OLED_ShowString(0, 54, "RX:", 12);
        OLED_ShowNum(18, 54, g_UART_RxCount, 4, 12);
        OLED_ShowString(46, 54, "T:", 12);
        OLED_ShowNum(58, 54, g_PB7_ToggleCount, 4, 12);
        OLED_ShowString(82, 54, "P:", 12);
        OLED_ShowNum(94, 54, pb7, 1, 12);
    }
    else if (g_State == STATE_IDLE)
    {
        if (g_TaskMode == 4)
        {
            OLED_ShowString(0, 54, "Target:", 12);
            OLED_ShowFloat(42, 54, g_BallTarget_cm, 2, 1, 12);
            OLED_ShowString(72, 54, "cm", 12);
        }
        else
        {
            OLED_ShowString(0, 54, "MODE: task START:go", 12);
        }
    }

    OLED_Refresh();
}
