/* STM32F103C8T6 / Keil ARMCC5 / FreeRTOS V11.1.0. */
#include "bsp.h"
#include "app_tasks.h"
int main(void)
{
    bsp_init();
    AppTasks_Start();
    for (;;) { }
}
