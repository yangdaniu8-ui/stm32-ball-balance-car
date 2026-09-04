#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"

/************************************************************
 * 0.96寸 OLED 显示驱动 (SSD1306, I2C 接口)
 *
 * 硬件连接：
 *   SCL → PB10 (I2C2_SCL)
 *   SDA → PB11 (I2C2_SDA)
 *
 * 分辨率：128 x 64
 * I2C 地址：0x3C (7-bit) → 0x78 (write) / 0x79 (read)
 ************************************************************/

/* OLED 尺寸 */
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_PAGES      (OLED_HEIGHT / 8)   /* 8 页，每页 8 像素高 */

/* I2C 地址 */
#define OLED_I2C_ADDR   0x78    /* 0x3C << 1 */

/* ---------- 函数声明 ---------- */
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);                    /* 将缓冲区刷新到 OLED */
void OLED_DrawPoint(u8 x, u8 y, u8 mode);  /* mode: 1=点亮, 0=熄灭 */
void OLED_ShowChar(u8 x, u8 y, u8 ch, u8 size);
void OLED_ShowString(u8 x, u8 y, const char *str, u8 size);
void OLED_ShowNum(u8 x, u8 y, int num, u8 len, u8 size);
void OLED_ShowFloat(u8 x, u8 y, float num, u8 intLen, u8 decLen, u8 size);
void OLED_Printf(u8 x, u8 y, u8 size, const char *fmt, ...);

#endif
