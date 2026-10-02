#ifndef APP_CORE_H
#define APP_CORE_H
#include <stdint.h>
#include "app_config.h"

typedef enum { APP_IDLE, APP_RUNNING, APP_FINISHED, APP_FAULT } AppState;
typedef enum { APP_FAULT_NONE, APP_FAULT_VISION, APP_FAULT_LINE } AppFault;
typedef struct {
    float position_cm;
    uint32_t timestamp_ms;
    uint32_t sequence;
} BallSample;
typedef struct {
    uint32_t now_ms;
    uint8_t vision_valid;
    BallSample ball;
    uint8_t line_mask; /* bit 0..3: L1,L2,R1,R2; 1 means black */
} AppInputs;
typedef struct {
    uint8_t key;   /* START=0, MODE=1, UP=2, DOWN=3 */
    uint8_t kind;  /* SHORT=1, LONG=2, HOLD=3 */
} AppKeyEvent;
typedef struct {
    AppState state;
    AppFault fault;
    uint8_t mode;
    uint8_t calibrating;
    int speed[5];
    int center_us;
    float arbitrary_target_cm;
    uint32_t start_ms;
    uint32_t elapsed_ms;
    uint32_t best_t0_ms;
    uint32_t line_lost_since;
    uint8_t line_lost_active;
    uint32_t reset_sequence;
} AppCore;
typedef struct {
    uint8_t motor_enabled;
    uint8_t ball_enabled;
    uint8_t calibrating;
    int base_pwm;
    int center_us;
    float target_cm;
    float accel_ff_deg;
} AppOutputs;

void App_Init(AppCore *app);
void App_HandleKey(AppCore *app, const AppKeyEvent *event, const AppInputs *in);
void App_EmergencyStop(AppCore *app, uint32_t now_ms);
void App_Step(AppCore *app, const AppInputs *in, AppOutputs *out);
int App_VisionFresh(const AppInputs *in);
#endif
