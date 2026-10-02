#ifndef CONTROL_MATH_H
#define CONTROL_MATH_H
#include "app_core.h"
typedef struct {
    float integral, previous_position, filtered_right, filtered_left;
    uint8_t initialized;
} LineController;
typedef struct {
    float position, velocity, position_integral, velocity_integral;
    float command_deg, previous_target;
    uint32_t timestamp_ms, sequence;
    uint8_t initialized;
} BallController;
void LineController_Reset(LineController *controller);
void LineController_Update(LineController *controller, uint8_t mask, int base_pwm,
                           float dt_s, int *right_pwm, int *left_pwm);
void BallController_Reset(BallController *controller);
/* New frames update measurements/integrals; target changes use existing state.
 * Returns 1 when the command is recomputed. Excludes feedforward.
 */
int BallController_Update(BallController *controller, const BallSample *sample,
                          float target_cm);
float Control_Slew(float current, float target, float rate_deg_s, float dt_s);
#endif
