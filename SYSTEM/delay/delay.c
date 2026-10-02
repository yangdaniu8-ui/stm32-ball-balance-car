/* SysTick belongs exclusively to FreeRTOS. */
#include "delay.h"
#include "FreeRTOS.h"
#include "task.h"
#define DELAY_DWT_CTRL   (*(volatile uint32_t *)0xE0001000u)
#define DELAY_DWT_CYCCNT (*(volatile uint32_t *)0xE0001004u)
void delay_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DELAY_DWT_CYCCNT = 0;
    DELAY_DWT_CTRL |= 1u;
}
void delay_us(u32 nus)
{
    uint32_t start, cycles;
    while (nus != 0u) {
        uint32_t chunk = nus > 1000u ? 1000u : nus;
        cycles = chunk * (SystemCoreClock / 1000000u);
        start = DELAY_DWT_CYCCNT;
        while ((uint32_t)(DELAY_DWT_CYCCNT - start) < cycles) { }
        nus -= chunk;
    }
}
void delay_ms(u16 nms)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        vTaskDelay(pdMS_TO_TICKS(nms));
    else
        while (nms-- != 0u) delay_us(1000u);
}








































