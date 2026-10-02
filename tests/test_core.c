#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "app_core.h"
#include "control_math.h"
#include "vision_protocol.h"

static void fresh(AppInputs *in, uint32_t now)
{
    in->now_ms = now;
    in->vision_valid = 1;
    in->ball.timestamp_ms = now;
    in->ball.sequence++;
    in->ball.position_cm = 0.0f;
    in->line_mask = 6;
}
static void key(AppCore *app, AppInputs *in, uint8_t id, uint8_t kind)
{
    AppKeyEvent event;
    event.key = id;
    event.kind = kind;
    App_HandleKey(app, &event, in);
}
static void test_tasks(void)
{
    AppCore app;
    AppInputs in = {0};
    AppOutputs out;
    unsigned int i;
    static const uint32_t duration[5] = {
        APP_T0_STOP_MS, APP_T1_FINISH_MS, APP_T2_STOP_MS, APP_T3_STOP_MS, APP_T4_STOP_MS
    };
    for (i = 0; i < 5; ++i) {
        App_Init(&app);
        app.mode = (uint8_t)i;
        app.arbitrary_target_cm = 3.0f;
        fresh(&in, 100);
        key(&app, &in, 0, 1);
        assert(app.state == APP_RUNNING && app.start_ms == 100);
        App_Step(&app, &in, &out);
        assert(app.elapsed_ms == 0 && out.base_pwm == 0);
        if (i == 1) assert(out.target_cm == 5.0f && !out.motor_enabled);
        fresh(&in, 2600);
        App_Step(&app, &in, &out);
        if (i == 1) assert(out.target_cm == -5.0f);
        fresh(&in, 100 + duration[i]);
        App_Step(&app, &in, &out);
        assert(app.state == APP_FINISHED && app.elapsed_ms == duration[i]);
        assert(!out.motor_enabled && out.accel_ff_deg == 0.0f);
        if (i == 1) assert(out.ball_enabled && out.target_cm == -5.0f);
        if (i == 4) assert(out.ball_enabled && out.target_cm == 3.0f);
        fresh(&in, 100 + duration[i] + 100);
        App_Step(&app, &in, &out);
        assert(app.elapsed_ms == duration[i]);
        key(&app, &in, 0, 1);
        assert(app.state == APP_IDLE);
        key(&app, &in, 0, 1);
        App_Step(&app, &in, &out);
        assert(app.state == APP_RUNNING && app.elapsed_ms == 0);
    }
    App_Init(&app);
    fresh(&in, 100);
    key(&app, &in, 1, 1);
    in.vision_valid = 0;
    key(&app, &in, 0, 1);
    assert(app.state == APP_FAULT && app.fault == APP_FAULT_VISION);
    key(&app, &in, 0, 1);
    fresh(&in, 1000);
    key(&app, &in, 0, 1);
    in.now_ms = 1000 + APP_VISION_TIMEOUT_MS + 1;
    App_Step(&app, &in, &out);
    assert(app.state == APP_FAULT && !out.motor_enabled && !out.ball_enabled);
    App_Init(&app);
    fresh(&in, 0xfffffff0u);
    key(&app, &in, 0, 1);
    fresh(&in, (uint32_t)(0xfffffff0u + APP_T0_STOP_MS));
    App_Step(&app, &in, &out);
    assert(app.state == APP_FINISHED && app.elapsed_ms == APP_T0_STOP_MS);
    App_Init(&app);
    fresh(&in, 0);
    key(&app, &in, 0, 1);
    fresh(&in, 4000);
    in.line_mask = 0;
    App_Step(&app, &in, &out);
    in.now_ms += APP_LINE_LOST_MS;
    App_Step(&app, &in, &out);
    assert(app.state == APP_FAULT && app.fault == APP_FAULT_LINE);
    assert(!out.motor_enabled && out.base_pwm == 0);
    key(&app, &in, 0, 2);
    assert(app.state == APP_IDLE);
    key(&app, &in, 1, 2);
    assert(app.calibrating);
    for (i = 0; i < 500; ++i) key(&app, &in, 2, 1);
    assert(app.center_us == APP_SERVO_MAX_US);
    for (i = 0; i < 500; ++i) key(&app, &in, 3, 1);
    assert(app.center_us == APP_SERVO_MIN_US);
    key(&app, &in, 0, 1);
    assert(!app.calibrating && app.state == APP_IDLE);
    App_Init(&app);
    app.mode = 2;
    assert(app.speed[2] == 1700);
    fresh(&in, 0);
    key(&app, &in, 0, 1);
    fresh(&in, 1500);
    App_Step(&app, &in, &out);
    assert(out.base_pwm == 850 && out.accel_ff_deg > 0);
    key(&app, &in, 0, 2);
    App_Step(&app, &in, &out);
    assert(!out.motor_enabled && out.accel_ff_deg == 0.0f);
    for (i = 0; i < 5; ++i) key(&app, &in, 1, 1);
    assert(app.mode == 2);
}

static int frame(VisionParser *parser, const char *text, BallSample *sample)
{
    int accepted = 0;
    while (*text) accepted += VisionParser_Push(parser, *text++, 1234, sample);
    return accepted;
}
static void test_protocol(void)
{
    VisionParser parser;
    BallSample sample;
    VisionParser_Init(&parser);
    assert(frame(&parser, "4.7\n", &sample) == 1);
    assert(fabsf(sample.position_cm - 4.7f) < 0.001f && sample.timestamp_ms == 1234);
    assert(frame(&parser, "-12.50\r\n", &sample) == 1);
    assert(sample.position_cm == -12.5f && sample.sequence == 2);
    assert(frame(&parser, "+0.0\n\n", &sample) == 1);
    assert(frame(&parser, "nan\ninf\nhello\n12.51\n-13\n+\n.\n1.2.3\n", &sample) == 0);
    assert(frame(&parser, "000000000000000000000000000000000000000\n", &sample) == 0);
    assert(frame(&parser, "7.4\n", &sample) == 1 && sample.sequence == 4);
    VisionParser_Discard(&parser);
    assert(frame(&parser, "99\n", &sample) == 0);
    assert(frame(&parser, "1.2\n", &sample) == 1);
}

static void test_control(void)
{
    LineController line;
    BallController ball;
    BallSample sample = {0.0f, 0, 1};
    unsigned int i;
    int right, left;
    float saved, velocity;
    LineController_Reset(&line);
    for (i = 0; i < 10000; ++i) {
        LineController_Update(&line, (uint8_t)(i % 16), 2500, 0.01f, &right, &left);
        assert(right >= -800 && right <= APP_PWM_MAX);
        assert(left >= -800 && left <= APP_PWM_MAX);
        assert(fabsf(line.integral) <= APP_LINE_I_LIMIT_S);
    }
    LineController_Update(&line, 6, 0, 0.01f, &right, &left);
    assert(right == 0 && left == 0 && !line.initialized);
    BallController_Reset(&ball);
    assert(BallController_Update(&ball, &sample, 5.0f));
    saved = ball.position_integral;
    for (i = 0; i < 100; ++i) assert(!BallController_Update(&ball, &sample, 5.0f));
    assert(ball.position_integral == saved);
    assert(BallController_Update(&ball, &sample, -5.0f));
    assert(ball.position_integral == saved && ball.position == 0.0f);
    assert(!BallController_Update(&ball, &sample, -5.0f));
    sample.position_cm = 1.0f;
    sample.timestamp_ms = 50;
    sample.sequence++;
    assert(BallController_Update(&ball, &sample, 0.0f));
    assert(ball.velocity > 0.0f);
    velocity = ball.velocity;
    for (i = 0; i < 100; ++i) {
        sample.timestamp_ms += 50;
        sample.sequence++;
        assert(BallController_Update(&ball, &sample, -5.0f));
        assert(fabsf(ball.command_deg) <= APP_SERVO_LIMIT_DEG);
        assert(fabsf(ball.position_integral) <= APP_POS_I_LIMIT);
        assert(fabsf(ball.velocity_integral) <= APP_VEL_I_LIMIT);
    }
    assert(fabsf(ball.velocity) < velocity * 0.01f);
    assert(fabsf(Control_Slew(0, 12, 60, 0.01f) - 0.6f) < 0.001f);
    BallController_Reset(&ball);
    assert(!ball.initialized && ball.command_deg == 0.0f);
}
int main(void)
{
    test_tasks();
    test_protocol();
    test_control();
    puts("PASS: task modes, timing, restart, faults, parser, frame timing and controllers");
    return 0;
}
