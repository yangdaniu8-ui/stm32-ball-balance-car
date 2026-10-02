#include "key.h"
#include <string.h>
typedef struct {
    u8 raw, stable, debounce, long_sent, event;
    u16 held_ms, repeat_ms;
} KeyInfo;
static KeyInfo keys[KEY_COUNT];
static u8 read_key(u8 id)
{
    switch (id) {
    case KEY_ID_START: return GPIO_ReadInputDataBit(KEY_START_PORT, KEY_START_PIN);
    case KEY_ID_MODE: return GPIO_ReadInputDataBit(KEY_MODE_PORT, KEY_MODE_PIN);
    case KEY_ID_UP: return GPIO_ReadInputDataBit(KEY_UP_PORT, KEY_UP_PIN);
    default: return GPIO_ReadInputDataBit(KEY_DOWN_PORT, KEY_DOWN_PIN);
    }
}
void Key_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio;
    u8 i;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin = KEY_START_PIN | KEY_MODE_PIN | KEY_DOWN_PIN;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = KEY_UP_PIN;
    GPIO_Init(GPIOB, &gpio);
    memset(keys, 0, sizeof(keys));
    for (i = 0; i < KEY_COUNT; ++i) {
        keys[i].raw = keys[i].stable = read_key(i);
        /* An already-held key at power-on must first be released. */
        keys[i].long_sent = keys[i].stable == 0;
    }
}
/* Called only by KeyTask, every 10 ms. */
void Key_Scan(void)
{
    u8 i, raw;
    KeyInfo *key;
    for (i = 0; i < KEY_COUNT; ++i) {
        key = &keys[i];
        raw = read_key(i);
        if (raw != key->raw) { key->raw = raw; key->debounce = 0; }
        else if (key->debounce < 2) key->debounce++;
        if (key->debounce >= 2 && key->stable != raw) {
            key->stable = raw;
            if (raw == 0) {
                key->held_ms = key->repeat_ms = 0;
                key->long_sent = 0;
            } else {
                if (!key->long_sent) key->event = KEY_STATE_SHORT;
                key->held_ms = key->repeat_ms = 0;
            }
        }
        if (key->stable == 0) {
            if (!key->long_sent) {
                key->held_ms += 10;
                if (key->held_ms >= 800) {
                    key->long_sent = 1;
                    key->event = KEY_STATE_LONG;
                    key->repeat_ms = 0;
                }
            } else if (i == KEY_ID_UP || i == KEY_ID_DOWN) {
                key->repeat_ms += 10;
                if (key->repeat_ms >= 200) {
                    key->repeat_ms = 0;
                    key->event = KEY_STATE_HOLD;
                }
            }
        }
    }
}
u8 Key_GetState(u8 id)
{
    u8 event;
    if (id >= KEY_COUNT) return KEY_STATE_NONE;
    event = keys[id].event;
    keys[id].event = KEY_STATE_NONE;
    return event;
}
u8 Key_IsPressed(u8 id) { return id < KEY_COUNT && keys[id].stable == 0; }
u8 Key_AnyPressed(void)
{
    u8 i;
    for (i = 0; i < KEY_COUNT; ++i) if (Key_IsPressed(i)) return 1;
    return 0;
}
