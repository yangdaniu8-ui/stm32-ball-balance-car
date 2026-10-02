#ifndef APP_TASKS_H
#define APP_TASKS_H
#include <stdint.h>
typedef struct {
    uint32_t control_overruns;
    uint32_t key_queue_drops;
    uint32_t rejected_frames;
    uint32_t uart_errors;
    uint32_t stack_free_words[4]; /* control, vision, keys, display */
    const char *fatal_file;
    unsigned long fatal_line;
} AppDiagnostics;
extern volatile AppDiagnostics g_AppDiagnostics;
void AppTasks_Start(void);
void AppRtos_Assert(const char *file, unsigned long line);
#endif
