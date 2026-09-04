#ifndef __LINEWALKING_H__
#define __LINEWALKING_H__

/*
 * 四路巡线传感器布局（从左到右）：
 *   L1  L2  |车头|  R1  R2
 *
 * 传感器读数：LOW(0)=检测到黑线, HIGH(1)=白色地面
 */

/* ---- 传感器启用开关 ---- */
#define USE_LINE_L1
#define USE_LINE_L2
#define USE_LINE_R1
#define USE_LINE_R2

/* ---- 引脚定义 ---- */
#define LineWalk_L1_RCC     RCC_APB2Periph_GPIOC
#define LineWalk_L2_RCC     RCC_APB2Periph_GPIOB   /* PB4 */
#define LineWalk_R1_RCC     RCC_APB2Periph_GPIOC
#define LineWalk_R2_RCC     RCC_APB2Periph_GPIOB

#define LineWalk_L1_PIN     GPIO_Pin_14       /* PC14 */
#define LineWalk_L2_PIN     GPIO_Pin_4        /* PB4 (原 PC13 有 LED 冲突) */
#define LineWalk_R1_PIN     GPIO_Pin_15       /* PC15 */
#define LineWalk_R2_PIN     GPIO_Pin_12       /* PB12 */

#define LineWalk_L1_PORT    GPIOC
#define LineWalk_L2_PORT    GPIOB
#define LineWalk_R1_PORT    GPIOC
#define LineWalk_R2_PORT    GPIOB

/* ---- PID 控制参数（实地调参改这里）---- */
#define LINE_KP             50    /* 比例: 直道不摆 */
#define LINE_KI             1.57      /* 积分 */
#define LINE_KD             6050    /* 微分: 强力抑制摆动 */
#define LINE_I_MAX          2000    /* 积分上限 */
#define LINE_TURN_MIN       5200    /* 弯道最小差速 */
#define LINE_BIAS           382     /* 重量偏置: 正值=右轮补偿 */

/* ---- 启停线检测 ---- */
#define STARTLINE_CONFIRM   5       /* 连续检测到 N 次才确认启停线 */

/* ---- 函数声明 ---- */
void LineWalking_GPIO_Init(void);
void GetLineWalking(int *p_iL1, int *p_iL2, int *p_iR1, int *p_iR2);

/* PID 巡线（使用全局 g_Speed） */
void LineWalking_PID(int baseSpeed);

/* 检测启停线（粗黑线：四路同时为 LOW） */
u8 Check_StartLine(void);

/* 旧版巡线（保留兼容） */
void LineWalking(void);

#endif
