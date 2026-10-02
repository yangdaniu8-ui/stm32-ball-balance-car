#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* All time values are milliseconds unless the name specifies seconds. */
#define APP_CONTROL_PERIOD_MS       10u
#define APP_KEY_PERIOD_MS           10u
#define APP_DISPLAY_PERIOD_MS       200u
#define APP_VISION_TIMEOUT_MS       250u
#define APP_LINE_LOST_MS            300u
#define APP_START_RAMP_MS           3000u
#define APP_T0_STOP_MS              16600u
#define APP_T1_REVERSE_MS           2500u
#define APP_T1_FINISH_MS            5000u
#define APP_T2_STOP_MS              8000u
#define APP_T3_STOP_MS              28000u
#define APP_T4_STOP_MS              28000u
#define APP_T0_DEFAULT_PWM          2500
#define APP_T1_DEFAULT_PWM          0
#define APP_T2_DEFAULT_PWM          1700
#define APP_T3_DEFAULT_PWM          2500
#define APP_T4_DEFAULT_PWM          2500
#define APP_PWM_MIN                 800
#define APP_PWM_MAX                 7199
#define APP_SERVO_CENTER_US         1800
#define APP_SERVO_MIN_US            500
#define APP_SERVO_MAX_US            2500
#define APP_SERVO_LIMIT_DEG         12.0f
#define APP_SERVO_SLEW_DEG_S        60.0f
#define APP_SERVO_INVERT            1
#define APP_SERVO_BIAS_DEG          1.2f
#define APP_ACCEL_FF_DEG            1.2f
#define APP_ROD_HALF_CM             12.5f

/* Legacy line gains expressed against a defined 10 ms reference period.
 * The old unconstrained main-loop rate was unknown: field retuning is required.
 */
#define APP_LINE_KP                 50.0f
#define APP_LINE_KI_PER_S            15.7f
#define APP_LINE_KD_S                60.5f
#define APP_LINE_I_LIMIT_S          20.0f
#define APP_LINE_TURN_EXTRA_PWM      5200
#define APP_LINE_BIAS_PWM            382
#define APP_LINE_FILTER_ALPHA        0.25f

#define APP_POS_KP                  2.0f
#define APP_POS_KI                  0.18f
#define APP_POS_I_LIMIT             10.0f
#define APP_VEL_KP                  1.4f
#define APP_VEL_KI                  0.01f
/* This is velocity damping, NOT the derivative of velocity error. */
#define APP_VEL_DAMPING             1.8f
#define APP_VEL_I_LIMIT             10.0f
#define APP_VEL_TARGET_MAX_CM_S     20.0f
#define APP_BALL_DEADZONE_CM        0.18f
#define APP_BALL_FILTER_ALPHA       0.4f
#define APP_VELOCITY_FILTER_ALPHA   0.5f
#define APP_FRICTION_BOOST_DEG      2.0f

#endif
