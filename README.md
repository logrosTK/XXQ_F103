# XXQ_F103

STM32F103-based mobile robot firmware with four-wheel motor control, encoder feedback, PID speed control, UART command parsing, OLED display, sensors, servos, and PS2 controller support.

This repository uses a STM32CubeMX/HAL generated project structure with user code under `User/`.

## Features

- STM32F103 HAL project with CMake/Ninja build support
- Four-wheel motor driver for mecanum-style chassis
- Closed-loop wheel speed control using encoder feedback and PID
- UART command parser for servo, car, and mode commands
- OLED display support
- PS2 controller input support
- Ultrasonic/tracking/sensor application modules

## Hardware Notes

Motor mapping used by the firmware:

| Motor | Position |
| --- | --- |
| A | Left front |
| B | Right front |
| C | Right rear |
| D | Left rear |

UART debug output:

- USART1: `115200 8N1`
- TX: `PA9`
- RX: `PA10`

When closed-loop control is enabled, the firmware prints a compact status line every few seconds:

```text
CL t=... target[A,B,C,D]=[...] speed[A,B,C,D]=[...] pwm[A,B,C,D]=[...]
```

The log period is configured in `User/app_main.c`:

```c
#define MOTOR_STATUS_PERIOD_MS 3000U
```

## Build

Configure and build the Debug target:

```sh
cmake --preset Debug
cmake --build ./build/Debug
```

Or build with the preset:

```sh
cmake --build --preset Debug
```

The project is configured by `CMakePresets.json` and expects an ARM GCC toolchain matching `cmake/gcc-arm-none-eabi.cmake`.

## Project Structure

```text
Core/                       STM32CubeMX generated startup, HAL init, main loop
Drivers/                    STM32 HAL/CMSIS drivers
User/                       Application and board support code
User/app_main.c             Application init/loop and closed-loop status log
User/app_motor.c            Closed-loop motor application layer
User/Components/y_motor/    PWM motor driver and PID implementation
User/Components/y_encoder/  Encoder driver
User/Components/y_timer/    SysTick timing and 20 ms motor control tick
User/Components/y_global/   UART command parser and common actions
```

Program entry:

- `Core/Src/main.c` calls `app_init()`
- The main loop calls `app_loop()`
- The SysTick path calls `app_motor_run()` every 20 ms when closed-loop control is enabled

## Important Control APIs

### Initialize Motor System

Declared in `User/app_motor.h`, implemented in `User/app_motor.c`.

```c
void app_motor_init(void);
```

Initializes motor PWM and encoders. This is already called from `app_init()`.

### Set Four Wheel Target Speeds

This is the main high-level speed control interface:

```c
void motor_speed_set(float A, float B, float C, float D);
```

Units are meters per second (`m/s`).

Example:

```c
motor_speed_set(0.10f, 0.10f, 0.10f, 0.10f);
```

Meaning:

| Argument | Motor | Unit |
| --- | --- | --- |
| `A` | Left front | m/s |
| `B` | Right front | m/s |
| `C` | Right rear | m/s |
| `D` | Left rear | m/s |

Positive values are treated as forward wheel speed. The per-wheel output polarity is handled inside `app_motor_run()`.

### Enable or Disable Closed Loop

```c
void app_motor_set_closed_loop(uint8_t enabled);
uint8_t app_motor_get_closed_loop(void);
```

Example: start closed-loop control at 100 mm/s:

```c
motor_speed_set(0.10f, 0.10f, 0.10f, 0.10f);
app_motor_set_closed_loop(1);
```

Example: stop motors:

```c
motor_speed_set(0.0f, 0.0f, 0.0f, 0.0f);
app_motor_set_closed_loop(0);
```

After closed loop is enabled, the control loop runs automatically every 20 ms from `User/Components/y_timer/y_timer.c`.

### Closed-Loop Update Function

```c
void app_motor_run(void);
```

This function reads encoder counters, calculates real wheel speed, runs PID, and writes PWM outputs.

Application code normally should not call this repeatedly by itself. The timer layer already calls it periodically when `app_motor_get_closed_loop()` returns true.

### PID Parameters

PID implementation is in `User/Components/y_motor/y_motor.c`.

Current default parameters:

```c
float motor_kp = 800.0f;
float motor_ki = 35.0f;
float motor_kd = 400.0f;
```

Runtime parameter update:

```c
void SPEED_PidSetParam(float kp, float ki, float kd);
```

Example:

```c
SPEED_PidSetParam(800.0f, 35.0f, 400.0f);
```

PID state reset helpers:

```c
void SPEED_PidReset(void);
void SPEED_PidResetA(void);
void SPEED_PidResetB(void);
void SPEED_PidResetC(void);
void SPEED_PidResetD(void);
```

## UART Motor Command

The UART command parser is implemented in `User/Components/y_global/y_global.c`.

Four-wheel target speed command:

```text
$Car:a,b,c,d!
```

Example:

```text
$Car:0.10,0.10,0.10,0.10!
```

This command updates the target speeds through `motor_speed_set(a, b, c, d)`.

Important: the current `$Car` command updates speed targets only. Closed-loop control must already be enabled in firmware with:

```c
app_motor_set_closed_loop(1);
```

Stop command:

```text
$DST!
```

## Where To Add Your Own Motor Control

Add high-level motion logic in application modules, not inside the PID or PWM driver.

Good places:

- `User/app_main.c`
- `User/app_sensor.c`
- `User/app_ps2.c`
- `User/Components/y_global/y_global.c` command handlers

Typical pattern:

```c
app_motor_set_closed_loop(1);

// Later, whenever the target changes:
motor_speed_set(0.10f, 0.10f, 0.10f, 0.10f);
```

Avoid putting application behavior inside:

- `app_motor_run()` unless changing closed-loop motor logic
- `SPEED_PidCtlA/B/C/D()` unless changing the PID algorithm
- `MOTOR_A/B/C/D_SetSpeed()` unless changing low-level PWM polarity or driver behavior

## Notes

- Wheel speed logs may show values such as `96` or `104` mm/s around a `100` mm/s target because encoder counts are discrete over the 20 ms PID sampling period.
- If a motor or encoder is not connected while closed-loop target speed is nonzero, PID will continue increasing PWM because measured speed remains zero.
- Keep closed-loop testing conservative when only one motor is connected.

