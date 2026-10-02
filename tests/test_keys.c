#include <assert.h>
#include <stdio.h>
#include "key.h"
GPIO_TypeDef test_gpio_a = {0xffff}, test_gpio_b = {0xffff};
void RCC_APB2PeriphClockCmd(uint32_t mask, int enabled) { (void)mask; (void)enabled; }
void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *configuration)
{ (void)port; (void)configuration; }
u8 GPIO_ReadInputDataBit(GPIO_TypeDef *port, uint16_t pin)
{ return (port->inputs & pin) != 0; }
static unsigned int shorts, longs, holds;
static void scan(unsigned int count, u8 id)
{
    while (count-- != 0) {
        u8 state;
        Key_Scan();
        state = Key_GetState(id);
        if (state == KEY_STATE_SHORT) shorts++;
        if (state == KEY_STATE_LONG) longs++;
        if (state == KEY_STATE_HOLD) holds++;
    }
}
int main(void)
{
    Key_GPIO_Init();
    test_gpio_a.inputs &= (uint16_t)~KEY_START_PIN;
    scan(150, KEY_ID_START);
    assert(longs == 1 && shorts == 0 && holds == 0);
    test_gpio_a.inputs |= KEY_START_PIN;
    scan(10, KEY_ID_START);
    assert(shorts == 0); /* releasing a long press must not restart the car */
    test_gpio_a.inputs &= (uint16_t)~KEY_START_PIN;
    scan(10, KEY_ID_START);
    test_gpio_a.inputs |= KEY_START_PIN;
    scan(10, KEY_ID_START);
    assert(shorts == 1 && longs == 1);
    test_gpio_b.inputs &= (uint16_t)~KEY_UP_PIN;
    scan(150, KEY_ID_UP);
    assert(longs == 2 && holds >= 2);
    /* A key held during power-on is ignored until released. */
    test_gpio_a.inputs &= (uint16_t)~KEY_START_PIN;
    Key_GPIO_Init();
    scan(150, KEY_ID_START);
    test_gpio_a.inputs |= KEY_START_PIN;
    scan(10, KEY_ID_START);
    assert(shorts == 1 && longs == 2);
    puts("PASS: debounce, long-press suppression, hold events and power-on held keys");
    return 0;
}
