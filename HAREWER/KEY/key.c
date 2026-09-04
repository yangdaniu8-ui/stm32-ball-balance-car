#include "key.h"

typedef struct {
    u16 debounce_cnt;
    u8  pressed      : 1;
    u8  last_pressed : 1;
    u8  state        : 2;
    u8  state_read   : 1;
} KeyInfo_t;

static KeyInfo_t KeyBuf[KEY_COUNT];

/* 读取宏 */
#define KEY_START_RD()  GPIO_ReadInputDataBit(KEY_START_PORT, KEY_START_PIN)
#define KEY_MODE_RD()   GPIO_ReadInputDataBit(KEY_MODE_PORT,  KEY_MODE_PIN)
#define KEY_UP_RD()     GPIO_ReadInputDataBit(KEY_UP_PORT,    KEY_UP_PIN)
#define KEY_DOWN_RD()   GPIO_ReadInputDataBit(KEY_DOWN_PORT,  KEY_DOWN_PIN)

#define DEBOUNCE_MS     20
#define LONG_PRESS_MS   800
#define HOLD_MS         200

void Key_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(KEY_START_RCC | KEY_MODE_RCC |
                           KEY_UP_RCC   | KEY_DOWN_RCC, ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    /* PA11(START), PA12(MODE) */
    GPIO_InitStructure.GPIO_Pin = KEY_START_PIN | KEY_MODE_PIN;
    GPIO_Init(KEY_START_PORT, &GPIO_InitStructure);

    /* PA15(DOWN) */
    GPIO_InitStructure.GPIO_Pin = KEY_DOWN_PIN;
    GPIO_Init(KEY_DOWN_PORT, &GPIO_InitStructure);

    /* PB3(UP) — 需单独使能 GPIOB */
    GPIO_InitStructure.GPIO_Pin = KEY_UP_PIN;
    GPIO_Init(KEY_UP_PORT, &GPIO_InitStructure);

    /* 初始化按键状态为当前实际电平，防止上电误判长按 */
    KeyBuf[KEY_ID_START].last_pressed = KEY_START_RD();
    KeyBuf[KEY_ID_MODE].last_pressed  = KEY_MODE_RD();
    KeyBuf[KEY_ID_UP].last_pressed    = KEY_UP_RD();
    KeyBuf[KEY_ID_DOWN].last_pressed  = KEY_DOWN_RD();
}

void Key_Scan(void)
{
    u8 i, cur;

    for (i = 0; i < KEY_COUNT; i++)
    {
        switch (i)
        {
        case KEY_ID_START: cur = KEY_START_RD(); break;
        case KEY_ID_MODE:  cur = KEY_MODE_RD();  break;
        case KEY_ID_UP:    cur = KEY_UP_RD();    break;
        case KEY_ID_DOWN:  cur = KEY_DOWN_RD();  break;
        default:           cur = 1;              break;
        }

        if (cur != KeyBuf[i].last_pressed) {
            KeyBuf[i].debounce_cnt = 0;
            KeyBuf[i].last_pressed = cur;
        } else {
            if (KeyBuf[i].debounce_cnt < 0xFFFF)
                KeyBuf[i].debounce_cnt++;
        }

        if (KeyBuf[i].debounce_cnt >= (DEBOUNCE_MS / 10))
        {
            if (cur == 0) {
                if (KeyBuf[i].pressed == 0) {
                    KeyBuf[i].pressed = 1;
                    KeyBuf[i].debounce_cnt = 0;
                } else {
                    if (KeyBuf[i].debounce_cnt >= (LONG_PRESS_MS / 10)) {
                        if (KeyBuf[i].state == KEY_STATE_NONE) {
                            KeyBuf[i].state = KEY_STATE_LONG;
                            KeyBuf[i].state_read = 0;
                            KeyBuf[i].debounce_cnt = 0;
                        } else if (KeyBuf[i].debounce_cnt >= (HOLD_MS / 10)) {
                            KeyBuf[i].state = KEY_STATE_HOLD;
                            KeyBuf[i].state_read = 0;
                            KeyBuf[i].debounce_cnt = 0;
                        }
                    }
                }
            } else {
                if (KeyBuf[i].pressed == 1) {
                    KeyBuf[i].pressed = 0;
                    KeyBuf[i].debounce_cnt = 0;
                    if (KeyBuf[i].state == KEY_STATE_NONE) {
                        KeyBuf[i].state = KEY_STATE_SHORT;
                        KeyBuf[i].state_read = 0;
                    } else {
                        KeyBuf[i].state = KEY_STATE_NONE;
                    }
                }
            }
        }
    }
}

u8 Key_GetState(u8 key_id)
{
    u8 s;
    if (key_id >= KEY_COUNT) return KEY_STATE_NONE;
    s = KeyBuf[key_id].state;
    if (s == KEY_STATE_SHORT || s == KEY_STATE_LONG || s == KEY_STATE_HOLD) {
        KeyBuf[key_id].state = KEY_STATE_NONE;
    }
    return s;
}

u8 Key_IsPressed(u8 key_id) {
    return (key_id < KEY_COUNT) ? KeyBuf[key_id].pressed : 0;
}

u8 Key_AnyPressed(void) {
    u8 i;
    for (i = 0; i < KEY_COUNT; i++)
        if (KeyBuf[i].pressed) return 1;
    return 0;
}
