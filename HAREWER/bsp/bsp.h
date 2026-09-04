#ifndef __BSP_H__
#define __BSP_H__

#include "delay.h"
#include "sys.h"
#include "moto.h"
#include "pwm.h"
#include "linewalking.h"
#include "oled.h"
#include "key.h"
#include "balance.h"

/* ---------- 全局变量 ---------- */
extern int  g_Speed;
extern int  g_Mode;
extern u8   g_Running;
extern volatile u32 g_SysTick;

/* ---------- 函数声明 ---------- */
void bsp_init(void);
void System_Init(void);
void SysTick_Handler(void);
u32  DWT_GetTick(void);

#endif
