#include "app_tasks.h"
#include "app_core.h"
#include "control_math.h"
#include "vision_protocol.h"
#include "bsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define KEY_QUEUE_LENGTH 8u
typedef struct {
    AppCore app;
    AppOutputs outputs;
    BallSample ball;
    uint8_t vision_fresh;
    uint8_t line_mask;
} DisplaySnapshot;
static QueueHandle_t key_queue, ball_queue, display_queue;
static StaticQueue_t key_queue_cb, ball_queue_cb, display_queue_cb;
static uint8_t key_storage[KEY_QUEUE_LENGTH * sizeof(AppKeyEvent)];
static uint8_t ball_storage[sizeof(BallSample)];
static uint8_t display_storage[sizeof(DisplaySnapshot)];
static StaticTask_t task_cb[4], idle_cb;
static StackType_t control_stack[512], vision_stack[384];
static StackType_t key_stack[192], display_stack[512], idle_stack[128];
static TaskHandle_t task_handles[4];
static volatile uint8_t stop_requested;
volatile AppDiagnostics g_AppDiagnostics;

static uint8_t read_line_mask(void)
{
    int l1, l2, r1, r2;
    GetLineWalking(&l1, &l2, &r1, &r2);
    return (uint8_t)((l1 == LOW ? 1u : 0u) | (l2 == LOW ? 2u : 0u) |
                     (r1 == LOW ? 4u : 0u) | (r2 == LOW ? 8u : 0u));
}

static void ControlTask(void *argument)
{
    AppCore app;
    AppInputs in = {0};
    AppOutputs out;
    AppKeyEvent event;
    DisplaySnapshot snapshot;
    LineController line;
    BallController ball;
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t last_reset = 0;
    float servo_angle = 0.0f, desired_angle;
    int right_pwm, left_pwm;
    (void)argument;
    App_Init(&app);
    LineController_Reset(&line);
    BallController_Reset(&ball);
    for (;;) {
        in.now_ms = (uint32_t)xTaskGetTickCount();
        in.line_mask = read_line_mask();
        if (xQueueReceive(ball_queue, &in.ball, 0) == pdPASS) in.vision_valid = 1;
        if (stop_requested) {
            stop_requested = 0;
            /* Drop previously queued starts so an emergency stop cannot restart. */
            while (xQueueReceive(key_queue, &event, 0) == pdPASS) { }
            App_EmergencyStop(&app, in.now_ms);
        } else {
            while (xQueueReceive(key_queue, &event, 0) == pdPASS)
                App_HandleKey(&app, &event, &in);
        }
        App_Step(&app, &in, &out);
        if (last_reset != app.reset_sequence) {
            LineController_Reset(&line);
            BallController_Reset(&ball);
            servo_angle = 0.0f;
            Stop();
            last_reset = app.reset_sequence;
        }
        g_ServoCenterPWM = out.center_us;
        g_Speed = app.speed[app.mode];
        g_Mode = app.mode;
        g_Running = app.state == APP_RUNNING;
        if (out.motor_enabled) {
            LineController_Update(&line, in.line_mask, out.base_pwm,
                                  APP_CONTROL_PERIOD_MS / 1000.0f, &right_pwm, &left_pwm);
            Moto_SetM1Speed(right_pwm);
            Moto_SetM2Speed(left_pwm);
            Moto_RampUpdate();
        } else Stop();
        if (out.calibrating) {
            Balance_SetRawPWM((u16)out.center_us);
        } else if (out.ball_enabled) {
            BallController_Update(&ball, &in.ball, out.target_cm);
            desired_angle = ball.command_deg + out.accel_ff_deg;
            if (desired_angle > APP_SERVO_LIMIT_DEG) desired_angle = APP_SERVO_LIMIT_DEG;
            if (desired_angle < -APP_SERVO_LIMIT_DEG) desired_angle = -APP_SERVO_LIMIT_DEG;
            servo_angle = Control_Slew(servo_angle, desired_angle,
                                      APP_SERVO_SLEW_DEG_S, APP_CONTROL_PERIOD_MS / 1000.0f);
            Balance_SetServo(servo_angle);
        } else {
            servo_angle = 0.0f;
            Balance_SetServo(0.0f);
        }
        snapshot.app = app;
        snapshot.outputs = out;
        snapshot.ball = in.ball;
        snapshot.vision_fresh = (uint8_t)App_VisionFresh(&in);
        snapshot.line_mask = in.line_mask;
        xQueueOverwrite(display_queue, &snapshot);
        if (xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_CONTROL_PERIOD_MS)) == pdFALSE) {
            g_AppDiagnostics.control_overruns++;
            /* Do not execute repeated catch-up PID steps with fictional dt. */
            last_wake = xTaskGetTickCount();
        }
    }
}

static void VisionTask(void *argument)
{
    VisionParser parser;
    BallSample sample;
    char ch;
    uint32_t timestamp_ms, errors, last_errors = 0;
    (void)argument;
    VisionParser_Init(&parser);
    Balance_StartRx(xTaskGetCurrentTaskHandle());
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while (Balance_ReadRx(&ch, &timestamp_ms)) {
            errors = Balance_RxErrors();
            if (errors != last_errors) {
                VisionParser_Discard(&parser);
                last_errors = errors;
            }
            if (VisionParser_Push(&parser, ch, timestamp_ms, &sample))
                xQueueOverwrite(ball_queue, &sample);
        }
        g_AppDiagnostics.rejected_frames = parser.rejected;
        g_AppDiagnostics.uart_errors = Balance_RxErrors();
    }
}

static void KeyTask(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();
    AppKeyEvent event;
    uint8_t i;
    (void)argument;
    for (;;) {
        Key_Scan();
        for (i = 0; i < KEY_COUNT; ++i) {
            event.key = i;
            event.kind = Key_GetState(i);
            if (event.kind == KEY_STATE_NONE) continue;
            if (i == KEY_ID_START && event.kind == KEY_STATE_LONG) stop_requested = 1;
            else if (xQueueSend(key_queue, &event, 0) != pdPASS)
                g_AppDiagnostics.key_queue_drops++;
        }
        xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_KEY_PERIOD_MS));
    }
}

static void signed_cm(uint8_t x, uint8_t y, float cm)
{
    int tenths;
    OLED_ShowChar(x, y, cm < 0.0f ? '-' : '+', 12);
    if (cm < 0.0f) cm = -cm;
    tenths = (int)(cm * 10.0f + 0.5f);
    OLED_ShowNum(x + 6, y, tenths / 10, 2, 12);
    OLED_ShowChar(x + 18, y, '.', 12);
    OLED_ShowNum(x + 24, y, tenths % 10, 1, 12);
}

static void draw_snapshot(const DisplaySnapshot *s)
{
    static const char *names[] = {"IDL", "RUN", "DONE", "ERR"};
    static const char *tasks[] = {"T0: Line 1 lap", "T1: 0 > +5 > -5",
        "T2: AB + Ball", "T3: Lap + Ball", "T4: Hold target"};
    unsigned int i;
    uint32_t milliseconds = s->app.elapsed_ms;
    OLED_Clear();
    if (s->app.calibrating) {
        OLED_ShowString(0, 0, "SERVO CALIB", 16);
        OLED_ShowString(0, 22, "PWM:", 12);
        OLED_ShowNum(30, 22, s->app.center_us, 4, 12);
        OLED_ShowString(0, 42, "UP/DN adjust", 12);
        OLED_ShowString(0, 54, "START: save (RAM)", 12);
    } else {
        OLED_ShowString(0, 0, "T:", 16);
        OLED_ShowNum(16, 0, milliseconds / 1000u, 2, 16);
        OLED_ShowChar(32, 0, '.', 16);
        OLED_ShowNum(40, 0, milliseconds % 1000u / 10u, 2, 16);
        OLED_ShowChar(56, 0, 's', 16);
        OLED_ShowString(0, 20, "SPD:", 12);
        OLED_ShowNum(24, 20, s->app.speed[s->app.mode], 4, 12);
        OLED_ShowString(54, 20, names[s->app.state], 12);
        OLED_ShowString(84, 20, "T", 12);
        OLED_ShowNum(90, 20, s->app.mode, 1, 12);
        OLED_ShowString(0, 30, "S:[", 12);
        for (i = 0; i < 4; ++i)
            OLED_ShowChar((u8)(18u + i * 6u), 30, s->line_mask & (1u << i) ? '#' : '.', 12);
        OLED_ShowChar(42, 30, ']', 12);
        OLED_ShowString(0, 42, tasks[s->app.mode], 12);
        if (s->app.state == APP_FAULT) {
            OLED_ShowString(0, 54, s->app.fault == APP_FAULT_VISION ?
                            "ERR: VISION timeout" : "ERR: LINE lost", 12);
        } else if (s->app.mode == 0) {
            OLED_ShowString(0, 54, "START:go/stop", 12);
        } else {
            OLED_ShowString(0, 54, s->vision_fresh ? "B:" : "?:", 12);
            signed_cm(12, 54, s->ball.position_cm);
            OLED_ShowString(54, 54, "G:", 12);
            signed_cm(66, 54, s->outputs.target_cm);
            OLED_ShowString(102, 54, "cm", 12);
        }
    }
    OLED_Refresh();
}

static void DisplayTask(void *argument)
{
    DisplaySnapshot snapshot;
    TickType_t last_wake = xTaskGetTickCount();
    unsigned int i;
    (void)argument;
    for (;;) {
        if (xQueuePeek(display_queue, &snapshot, 0) == pdPASS) draw_snapshot(&snapshot);
        for (i = 0; i < 4; ++i)
            g_AppDiagnostics.stack_free_words[i] = uxTaskGetStackHighWaterMark(task_handles[i]);
        xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_DISPLAY_PERIOD_MS));
    }
}

void vApplicationGetIdleTaskMemory(StaticTask_t **cb, StackType_t **stack, uint32_t *depth)
{
    *cb = &idle_cb;
    *stack = idle_stack;
    *depth = sizeof(idle_stack) / sizeof(idle_stack[0]);
}

void AppRtos_Assert(const char *file, unsigned long line)
{
    __disable_irq();
    g_AppDiagnostics.fatal_file = file;
    g_AppDiagnostics.fatal_line = line;
    Stop();
    MOTO_STBY_DIS();
    Balance_SetServo(0.0f);
    for (;;) { }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    AppRtos_Assert(name, 0);
}

void AppTasks_Start(void)
{
    key_queue = xQueueCreateStatic(KEY_QUEUE_LENGTH, sizeof(AppKeyEvent), key_storage, &key_queue_cb);
    ball_queue = xQueueCreateStatic(1, sizeof(BallSample), ball_storage, &ball_queue_cb);
    display_queue = xQueueCreateStatic(1, sizeof(DisplaySnapshot), display_storage, &display_queue_cb);
    configASSERT(key_queue && ball_queue && display_queue);
    task_handles[0] = xTaskCreateStatic(ControlTask, "control", 512, NULL, 4, control_stack, &task_cb[0]);
    task_handles[1] = xTaskCreateStatic(VisionTask, "vision", 384, NULL, 3, vision_stack, &task_cb[1]);
    task_handles[2] = xTaskCreateStatic(KeyTask, "keys", 192, NULL, 2, key_stack, &task_cb[2]);
    task_handles[3] = xTaskCreateStatic(DisplayTask, "display", 512, NULL, 1, display_stack, &task_cb[3]);
    configASSERT(task_handles[0] && task_handles[1] && task_handles[2] && task_handles[3]);
    vTaskStartScheduler();
    AppRtos_Assert("scheduler returned", 0);
}
