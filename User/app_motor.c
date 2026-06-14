#include "app_motor.h"

#define MOTOR_A_ENCODER_SIGN (1.0f)
#define MOTOR_B_ENCODER_SIGN (1.0f)
#define MOTOR_C_ENCODER_SIGN (1.0f)
#define MOTOR_D_ENCODER_SIGN (1.0f)

static volatile uint8_t motor_closed_loop_enabled;

void app_motor_set_closed_loop(uint8_t enabled)
{
    uint8_t next_state = enabled ? 1U : 0U;
    uint8_t previous_state;
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    previous_state = motor_closed_loop_enabled;
    motor_closed_loop_enabled = next_state;
    if ((next_state != previous_state) || (next_state == 0U))
    {
        SPEED_PidReset();
    }
    if (primask == 0U)
    {
        __enable_irq();
    }
}

uint8_t app_motor_get_closed_loop(void)
{
    return motor_closed_loop_enabled;
}

void motor_speed_set(float A, float B, float C, float D)
{
    Wheel_A.TG = A;
    Wheel_B.TG = B;
    Wheel_C.TG = C;
    Wheel_D.TG = D;
}

void app_motor_init(void)
{
    motor_init();
    Encoder_Init();
}

static uint8_t motor_target_is_zero(void)
{
    return (Wheel_A.TG == 0.0f) &&
           (Wheel_B.TG == 0.0f) &&
           (Wheel_C.TG == 0.0f) &&
           (Wheel_D.TG == 0.0f);
}

void app_motor_run(void)
{
    Wheel_A.RT = (float)(MOTOR_A_ENCODER_SIGN * ENCODER_A_GetCounter() * MEC_WHEEL_SCALE);
    Wheel_B.RT = (float)(MOTOR_B_ENCODER_SIGN * ENCODER_B_GetCounter() * MEC_WHEEL_SCALE);
    Wheel_C.RT = (float)(MOTOR_C_ENCODER_SIGN * ENCODER_C_GetCounter() * MEC_WHEEL_SCALE);
    Wheel_D.RT = (float)(MOTOR_D_ENCODER_SIGN * ENCODER_D_GetCounter() * MEC_WHEEL_SCALE);

    if (motor_target_is_zero())
    {
        Wheel_A.PWM = 0;
        Wheel_B.PWM = 0;
        Wheel_C.PWM = 0;
        Wheel_D.PWM = 0;
        SPEED_PidReset();
        MOTOR_A_SetSpeed(0);
        MOTOR_B_SetSpeed(0);
        MOTOR_C_SetSpeed(0);
        MOTOR_D_SetSpeed(0);
        return;
    }

    if (Wheel_A.TG == 0.0f)
    {
        Wheel_A.PWM = 0;
        SPEED_PidResetA();
    }
    else
    {
        Wheel_A.PWM = SPEED_PidCtlA(Wheel_A.TG, Wheel_A.RT);
    }

    if (Wheel_B.TG == 0.0f)
    {
        Wheel_B.PWM = 0;
        SPEED_PidResetB();
    }
    else
    {
        Wheel_B.PWM = SPEED_PidCtlB(Wheel_B.TG, Wheel_B.RT);
    }

    if (Wheel_C.TG == 0.0f)
    {
        Wheel_C.PWM = 0;
        SPEED_PidResetC();
    }
    else
    {
        Wheel_C.PWM = SPEED_PidCtlC(-Wheel_C.TG, Wheel_C.RT);
    }

    if (Wheel_D.TG == 0.0f)
    {
        Wheel_D.PWM = 0;
        SPEED_PidResetD();
    }
    else
    {
        Wheel_D.PWM = SPEED_PidCtlD(Wheel_D.TG, Wheel_D.RT);
    }

    MOTOR_A_SetSpeed(Wheel_A.PWM);
    MOTOR_B_SetSpeed(-Wheel_B.PWM);
    MOTOR_C_SetSpeed(-Wheel_C.PWM);
    MOTOR_D_SetSpeed(-Wheel_D.PWM);
}
