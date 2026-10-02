#include "control_math.h"
#include <string.h>

static float limit(float value, float maximum)
{
    if (value > maximum) return maximum;
    if (value < -maximum) return -maximum;
    return value;
}

void LineController_Reset(LineController *controller)
{
    memset(controller, 0, sizeof(*controller));
}

void LineController_Update(LineController *c, uint8_t mask, int base_pwm,
                           float dt_s, int *right_pwm, int *left_pwm)
{
    float position = 0.0f, correction, derivative, max_correction;
    float right, left;
    if (dt_s <= 0.0f || base_pwm <= 0) {
        LineController_Reset(c);
        *right_pwm = *left_pwm = 0;
        return;
    }
    if (mask & 1u) position -= 3.0f;
    if (mask & 2u) position -= 1.0f;
    if (mask & 4u) position += 1.0f;
    if (mask & 8u) position += 3.0f;
    if (!c->initialized) {
        c->previous_position = position;
        c->initialized = 1;
    }
    c->integral = limit(c->integral + position * dt_s, APP_LINE_I_LIMIT_S);
    if (position == 0.0f) c->integral *= 0.75f;
    derivative = (position - c->previous_position) / dt_s;
    c->previous_position = position;
    correction = APP_LINE_KP * position + APP_LINE_KI_PER_S * c->integral
               + APP_LINE_KD_S * derivative + APP_LINE_BIAS_PWM;
    max_correction = (float)(base_pwm + APP_LINE_TURN_EXTRA_PWM);
    if (base_pwm < APP_PWM_MIN) max_correction = (float)base_pwm;
    correction = limit(correction, max_correction);
    right = base_pwm + correction;
    left = base_pwm - correction;
    if (right > APP_PWM_MAX) right = APP_PWM_MAX;
    if (left > APP_PWM_MAX) left = APP_PWM_MAX;
    if (right < -800) right = -800;
    if (left < -800) left = -800;
    c->filtered_right += APP_LINE_FILTER_ALPHA * (right - c->filtered_right);
    c->filtered_left += APP_LINE_FILTER_ALPHA * (left - c->filtered_left);
    *right_pwm = (int)c->filtered_right;
    *left_pwm = (int)c->filtered_left;
}

void BallController_Reset(BallController *controller)
{
    memset(controller, 0, sizeof(*controller));
}

int BallController_Update(BallController *c, const BallSample *sample, float target_cm)
{
    float dt_s, previous_position, raw_velocity, error, desired_velocity;
    float velocity_error, pi, vi, output, mechanical_output;
    uint32_t elapsed_ms;
    if (sample->position_cm > APP_ROD_HALF_CM || sample->position_cm < -APP_ROD_HALF_CM)
        return 0;
    if (c->initialized && sample->sequence == c->sequence) {
        if (target_cm == c->previous_target) return 0;
        dt_s = 0.0f; /* Immediate target response, no repeated-frame integration. */
        goto compute_command;
    }
    if (!c->initialized) {
        c->position = sample->position_cm;
        c->velocity = 0.0f;
        dt_s = 0.05f;
        c->initialized = 1;
    } else {
        elapsed_ms = sample->timestamp_ms - c->timestamp_ms;
        if (elapsed_ms < 5u) return 0;
        if (elapsed_ms > APP_VISION_TIMEOUT_MS) {
            BallController_Reset(c);
            return BallController_Update(c, sample, target_cm);
        }
        dt_s = elapsed_ms / 1000.0f;
        previous_position = c->position;
        c->position += APP_BALL_FILTER_ALPHA * (sample->position_cm - c->position);
        raw_velocity = (c->position - previous_position) / dt_s;
        c->velocity += APP_VELOCITY_FILTER_ALPHA * (raw_velocity - c->velocity);
    }
    c->timestamp_ms = sample->timestamp_ms;
    c->sequence = sample->sequence;
compute_command:
    c->previous_target = target_cm;
    error = target_cm - c->position;
    if (error < APP_BALL_DEADZONE_CM && error > -APP_BALL_DEADZONE_CM) error = 0.0f;
    pi = limit(c->position_integral + error * dt_s, APP_POS_I_LIMIT);
    desired_velocity = limit(APP_POS_KP * error + APP_POS_KI * pi,
                             APP_VEL_TARGET_MAX_CM_S);
    velocity_error = desired_velocity - c->velocity;
    vi = limit(c->velocity_integral + velocity_error * dt_s, APP_VEL_I_LIMIT);
    output = APP_VEL_KP * velocity_error + APP_VEL_KI * vi
           - APP_VEL_DAMPING * c->velocity;
    if ((error > 1.0f || error < -1.0f) && c->velocity < 0.3f && c->velocity > -0.3f)
        output += error > 0.0f ? APP_FRICTION_BOOST_DEG : -APP_FRICTION_BOOST_DEG;
    /* Freeze integration if it would drive an already saturated output further. */
    if (!((output > APP_SERVO_LIMIT_DEG && error > 0.0f) ||
          (output < -APP_SERVO_LIMIT_DEG && error < 0.0f))) {
        c->position_integral = pi;
        c->velocity_integral = vi;
    }
    mechanical_output = APP_SERVO_INVERT ? -output : output;
    c->command_deg = limit(mechanical_output + APP_SERVO_BIAS_DEG, APP_SERVO_LIMIT_DEG);
    return 1;
}

float Control_Slew(float current, float target, float rate_deg_s, float dt_s)
{
    return current + limit(target - current, rate_deg_s * dt_s);
}
