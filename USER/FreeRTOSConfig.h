#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
#include <stdint.h>
extern uint32_t SystemCoreClock;
void AppRtos_Assert(const char *file, unsigned long line);
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configCPU_CLOCK_HZ                      SystemCoreClock
#define configTICK_RATE_HZ                      1000
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                128
#define configMAX_TASK_NAME_LEN                 12
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                  1
#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        0
#define configUSE_TIMERS                        0
#define configUSE_MUTEXES                       0
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_TRACE_FACILITY                0
#define configUSE_NEWLIB_REENTRANT              0
#define configUSE_EVENT_GROUPS                  0
#define configUSE_STREAM_BUFFERS                0
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xTaskDelayUntil                 1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_uxTaskGetStackHighWaterMark      1
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configASSERT(condition) do { if (!(condition)) \
    AppRtos_Assert(__FILE__, __LINE__); } while (0)
#define vPortSVCHandler      SVC_Handler
#define xPortPendSVHandler   PendSV_Handler
#define xPortSysTickHandler  SysTick_Handler
#endif
