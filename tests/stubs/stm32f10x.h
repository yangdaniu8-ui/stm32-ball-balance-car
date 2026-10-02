#ifndef TEST_STM32F10X_H
#define TEST_STM32F10X_H
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef struct { uint16_t inputs; } GPIO_TypeDef;
typedef struct { uint16_t GPIO_Pin; int GPIO_Mode, GPIO_Speed; } GPIO_InitTypeDef;
extern GPIO_TypeDef test_gpio_a, test_gpio_b;
#define GPIOA (&test_gpio_a)
#define GPIOB (&test_gpio_b)
#define GPIO_Pin_3 (1u << 3)
#define GPIO_Pin_11 (1u << 11)
#define GPIO_Pin_12 (1u << 12)
#define GPIO_Pin_15 (1u << 15)
#define RCC_APB2Periph_GPIOA 1
#define RCC_APB2Periph_GPIOB 2
#define GPIO_Mode_IPU 1
#define GPIO_Speed_50MHz 1
#define ENABLE 1
void RCC_APB2PeriphClockCmd(uint32_t mask, int enabled);
void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *configuration);
u8 GPIO_ReadInputDataBit(GPIO_TypeDef *port, uint16_t pin);
#endif
