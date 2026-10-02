#include "app_core.h"
#include <string.h>

static int clamp_int(int value, int low, int high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

int App_VisionFresh(const AppInputs *in)
{
    return in->vision_valid &&
           (uint32_t)(in->now_ms - in->ball.timestamp_ms) <= APP_VISION_TIMEOUT_MS;
}

static void enter_state(AppCore *app, AppState state, uint32_t now_ms)
{
    if (app->state == APP_RUNNING)
        app->elapsed_ms = now_ms - app->start_ms;
    app->state = state;
    app->line_lost_active = 0;
    if (state == APP_IDLE || state == APP_RUNNING || state == APP_FAULT)
        app->reset_sequence++;
    if (state == APP_IDLE) app->fault = APP_FAULT_NONE;
    if (state == APP_RUNNING) {
        app->start_ms = now_ms;
        app->elapsed_ms = 0;
        app->fault = APP_FAULT_NONE;
    }
}

void App_Init(AppCore *app)
{
    static const int defaults[5] = {
        APP_T0_DEFAULT_PWM, APP_T1_DEFAULT_PWM, APP_T2_DEFAULT_PWM,
        APP_T3_DEFAULT_PWM, APP_T4_DEFAULT_PWM
    };
    unsigned int i;
    memset(app, 0, sizeof(*app));
    for (i = 0; i < 5; ++i) app->speed[i] = defaults[i];
    app->center_us = APP_SERVO_CENTER_US;
}

void App_EmergencyStop(AppCore *app, uint32_t now_ms)
{
    app->calibrating = 0;
    enter_state(app, APP_IDLE, now_ms);
}

void App_HandleKey(AppCore *app, const AppKeyEvent *event, const AppInputs *in)
{
    int direction;
    if (event->key == 0) {
        if (event->kind == 2) {
            App_EmergencyStop(app, in->now_ms);
        } else if (event->kind == 1) {
            if (app->calibrating) {
                app->calibrating = 0;
                app->reset_sequence++;
            } else if (app->state != APP_IDLE) {
                App_EmergencyStop(app, in->now_ms);
            } else if (app->mode != 0 && !App_VisionFresh(in)) {
                enter_state(app, APP_FAULT, in->now_ms);
                app->fault = APP_FAULT_VISION;
            } else {
                enter_state(app, APP_RUNNING, in->now_ms);
            }
        }
        return;
    }
    if (app->state != APP_IDLE) return;
    if (event->key == 1 && !app->calibrating) {
        if (event->kind == 1) {
            app->mode = (uint8_t)((app->mode + 1u) % 5u);
            app->reset_sequence++;
        } else if (event->kind == 2) {
            app->calibrating = 1;
            app->reset_sequence++;
        }
        return;
    }
    if (event->key != 2 && event->key != 3) return;
    direction = event->key == 2 ? 1 : -1;
    if (app->calibrating) {
        app->center_us = clamp_int(app->center_us + direction *
            (event->kind == 1 ? 10 : 5), APP_SERVO_MIN_US, APP_SERVO_MAX_US);
    } else if (app->mode == 4) {
        app->arbitrary_target_cm += direction * (event->kind == 1 ? 1.0f : 0.5f);
        if (app->arbitrary_target_cm > 10.0f) app->arbitrary_target_cm = 10.0f;
        if (app->arbitrary_target_cm < -10.0f) app->arbitrary_target_cm = -10.0f;
    } else if (event->kind == 1 && app->mode != 1) {
        app->speed[app->mode] = clamp_int(app->speed[app->mode] + direction * 100,
                                        APP_PWM_MIN, APP_PWM_MAX);
    }
}

void App_Step(AppCore *app, const AppInputs *in, AppOutputs *out)
{
    static const uint32_t stop_ms[5] = {
        APP_T0_STOP_MS, APP_T1_FINISH_MS, APP_T2_STOP_MS,
        APP_T3_STOP_MS, APP_T4_STOP_MS
    };
    memset(out, 0, sizeof(*out));
    out->center_us = app->center_us;
    out->calibrating = app->calibrating;
    if (app->state == APP_RUNNING) {
        app->elapsed_ms = in->now_ms - app->start_ms;
        if (app->elapsed_ms >= stop_ms[app->mode]) {
            enter_state(app, APP_FINISHED, in->now_ms);
            if (app->mode == 0 && (app->best_t0_ms == 0 ||
                                  app->elapsed_ms < app->best_t0_ms))
                app->best_t0_ms = app->elapsed_ms;
        }
    }
    if (app->mode != 0 && (app->state == APP_RUNNING || app->state == APP_FINISHED)
        && !App_VisionFresh(in)) {
        enter_state(app, APP_FAULT, in->now_ms);
        app->fault = APP_FAULT_VISION;
    }
    if (app->state == APP_RUNNING && app->mode != 1) {
        out->base_pwm = app->speed[app->mode];
        if (app->elapsed_ms < APP_START_RAMP_MS) {
            out->base_pwm = (int)((uint32_t)out->base_pwm * app->elapsed_ms /
                                  APP_START_RAMP_MS);
            if (app->mode != 0)
                out->accel_ff_deg = APP_ACCEL_FF_DEG * app->speed[app->mode] / 2500.0f;
        }
        if (in->line_mask == 0 && out->base_pwm >= APP_PWM_MIN) {
            if (!app->line_lost_active) {
                app->line_lost_active = 1;
                app->line_lost_since = in->now_ms;
            } else if ((uint32_t)(in->now_ms - app->line_lost_since) >= APP_LINE_LOST_MS) {
                enter_state(app, APP_FAULT, in->now_ms);
                app->fault = APP_FAULT_LINE;
                out->base_pwm = 0;
                out->accel_ff_deg = 0.0f;
            }
        } else app->line_lost_active = 0;
        out->motor_enabled = app->state == APP_RUNNING;
    }
    out->ball_enabled = app->mode != 0 &&
        (app->state == APP_RUNNING || app->state == APP_FINISHED);
    if (app->mode == 1 && out->ball_enabled)
        out->target_cm = app->elapsed_ms < APP_T1_REVERSE_MS ? 5.0f : -5.0f;
    else if (app->mode == 4) out->target_cm = app->arbitrary_target_cm;
    if (app->state != APP_RUNNING) out->accel_ff_deg = 0.0f;
}
