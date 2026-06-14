# XXQ_F103

中文 | [English](#english)

## 中文

这是一个基于 STM32F103 的移动机器人固件工程，包含四轮电机驱动、编码器反馈、PID 闭环速度控制、串口命令解析、OLED 显示、传感器、舵机和 PS2 手柄相关功能。

工程基于 STM32CubeMX/HAL 结构，用户代码主要在 `User/` 目录下。

### 功能特性

- STM32F103 HAL 工程，支持 CMake/Ninja 构建
- 四轮麦轮/小车电机驱动
- 编码器速度反馈
- PID 闭环轮速控制
- UART 串口命令解析
- OLED 显示
- PS2 手柄输入
- 超声波、循迹、传感器应用模块

### 电机映射

| 电机 | 位置 |
| --- | --- |
| A | 左前轮 |
| B | 右前轮 |
| C | 右后轮 |
| D | 左后轮 |

### 串口日志

调试串口：

- USART1: `115200 8N1`
- TX: `PA9`
- RX: `PA10`

开启闭环后，会周期输出电机闭环状态：

```text
CL t=... target[A,B,C,D]=[...] speed[A,B,C,D]=[...] pwm[A,B,C,D]=[...]
```

日志周期在 `User/app_main.c` 中设置：

```c
#define MOTOR_STATUS_PERIOD_MS 3000U
```

### 编译

配置并编译 Debug 版本：

```sh
cmake --preset Debug
cmake --build ./build/Debug
```

也可以使用 build preset：

```sh
cmake --build --preset Debug
```

工程使用 `CMakePresets.json`，ARM GCC 工具链配置在 `cmake/gcc-arm-none-eabi.cmake`。

### 目录结构

```text
Core/                       STM32CubeMX 生成的启动、HAL 初始化和 main loop
Drivers/                    STM32 HAL/CMSIS 驱动
User/                       应用层和板级用户代码
User/app_main.c             应用初始化、主循环、闭环状态日志
User/app_motor.c            电机闭环应用层
User/Components/y_motor/    PWM 电机驱动和 PID 实现
User/Components/y_encoder/  编码器驱动
User/Components/y_timer/    SysTick 计时和 20 ms 电机控制节拍
User/Components/y_global/   串口命令解析和通用动作
```

程序入口：

- `Core/Src/main.c` 调用 `app_init()`
- 主循环调用 `app_loop()`
- SysTick 中断路径在闭环开启时每 20 ms 调用一次 `app_motor_run()`

### 重要控制接口

#### 电机系统初始化

声明位置：`User/app_motor.h`

实现位置：`User/app_motor.c`

```c
void app_motor_init(void);
```

用于初始化电机 PWM 和编码器。当前已经在 `app_init()` 中自动调用。

#### 四轮目标速度设置接口

这是最重要的高层电机速度控制接口：

```c
void motor_speed_set(float A, float B, float C, float D);
```

单位是 `m/s`。

示例：

```c
motor_speed_set(0.10f, 0.10f, 0.10f, 0.10f);
```

参数含义：

| 参数 | 电机 | 单位 |
| --- | --- | --- |
| `A` | 左前轮 | m/s |
| `B` | 右前轮 | m/s |
| `C` | 右后轮 | m/s |
| `D` | 左后轮 | m/s |

正值表示前进方向的轮速，具体每个电机的输出极性在 `app_motor_run()` 中处理。

#### 开启或关闭闭环控制

```c
void app_motor_set_closed_loop(uint8_t enabled);
uint8_t app_motor_get_closed_loop(void);
```

启动 100 mm/s 四轮闭环：

```c
motor_speed_set(0.10f, 0.10f, 0.10f, 0.10f);
app_motor_set_closed_loop(1);
```

停止电机：

```c
motor_speed_set(0.0f, 0.0f, 0.0f, 0.0f);
app_motor_set_closed_loop(0);
```

闭环开启后，`User/Components/y_timer/y_timer.c` 会自动每 20 ms 调用一次控制循环。

#### 闭环控制更新函数

```c
void app_motor_run(void);
```

该函数读取编码器、计算实时速度、执行 PID、输出 PWM。

普通应用代码通常不需要反复调用它，因为定时器层已经在闭环开启时自动周期调用。

#### PID 参数

PID 实现在 `User/Components/y_motor/y_motor.c`。

当前默认参数：

```c
float motor_kp = 800.0f;
float motor_ki = 35.0f;
float motor_kd = 400.0f;
```

运行时修改 PID 参数：

```c
void SPEED_PidSetParam(float kp, float ki, float kd);
```

示例：

```c
SPEED_PidSetParam(800.0f, 35.0f, 400.0f);
```

PID 状态复位接口：

```c
void SPEED_PidReset(void);
void SPEED_PidResetA(void);
void SPEED_PidResetB(void);
void SPEED_PidResetC(void);
void SPEED_PidResetD(void);
```

### UART 电机命令

串口命令解析在 `User/Components/y_global/y_global.c`。

四轮目标速度命令：

```text
$Car:a,b,c,d!
```

示例：

```text
$Car:0.10,0.10,0.10,0.10!
```

该命令会调用：

```c
motor_speed_set(a, b, c, d);
```

注意：当前 `$Car` 命令只更新目标速度，不自动开启闭环。闭环需要在固件中调用：

```c
app_motor_set_closed_loop(1);
```

停止命令：

```text
$DST!
```

### 应该在哪里写自己的电机控制逻辑

建议把高层运动逻辑写在应用层，不要写进 PID 或 PWM 底层驱动。

推荐位置：

- `User/app_main.c`
- `User/app_sensor.c`
- `User/app_ps2.c`
- `User/Components/y_global/y_global.c` 的命令处理分支

典型写法：

```c
app_motor_set_closed_loop(1);

// 后续需要改变速度时:
motor_speed_set(0.10f, 0.10f, 0.10f, 0.10f);
```

不建议把业务逻辑写进：

- `app_motor_run()`，除非你要修改闭环控制流程
- `SPEED_PidCtlA/B/C/D()`，除非你要修改 PID 算法
- `MOTOR_A/B/C/D_SetSpeed()`，除非你要修改底层 PWM 极性或驱动方式

### 注意事项

- 目标速度为 `100 mm/s` 时，日志里实际速度可能显示 `96` 或 `104 mm/s`，这是编码器计数和 20 ms 采样周期带来的离散误差。
- 如果某个电机或编码器没有连接，但目标速度不是 0，PID 会因为测得速度一直为 0 而继续增大 PWM。
- 只接单个电机时，请谨慎开启四轮闭环目标速度。

---

## English

This is STM32F103 firmware for a mobile robot. It includes four-wheel motor driving, encoder feedback, PID closed-loop speed control, UART command parsing, OLED display, sensors, servos, and PS2 controller support.

The project follows a STM32CubeMX/HAL layout. User code is mainly under `User/`.

### Features

- STM32F103 HAL project with CMake/Ninja build support
- Four-wheel mecanum/mobile robot motor driver
- Encoder speed feedback
- PID closed-loop wheel speed control
- UART command parser
- OLED display support
- PS2 controller input support
- Ultrasonic, tracking, and sensor application modules

### Motor Mapping

| Motor | Position |
| --- | --- |
| A | Left front |
| B | Right front |
| C | Right rear |
| D | Left rear |

### UART Log

Debug UART:

- USART1: `115200 8N1`
- TX: `PA9`
- RX: `PA10`

When closed-loop control is enabled, the firmware periodically prints:

```text
CL t=... target[A,B,C,D]=[...] speed[A,B,C,D]=[...] pwm[A,B,C,D]=[...]
```

The log period is configured in `User/app_main.c`:

```c
#define MOTOR_STATUS_PERIOD_MS 3000U
```

### Build

Configure and build the Debug target:

```sh
cmake --preset Debug
cmake --build ./build/Debug
```

Or use the build preset:

```sh
cmake --build --preset Debug
```

The project uses `CMakePresets.json`. The ARM GCC toolchain file is `cmake/gcc-arm-none-eabi.cmake`.

### Project Structure

```text
Core/                       STM32CubeMX generated startup, HAL init, and main loop
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

### Important Control APIs

#### Motor System Initialization

Declared in `User/app_motor.h`.

Implemented in `User/app_motor.c`.

```c
void app_motor_init(void);
```

Initializes motor PWM and encoders. It is already called by `app_init()`.

#### Four-Wheel Target Speed Interface

This is the primary high-level motor speed control API:

```c
void motor_speed_set(float A, float B, float C, float D);
```

Unit: `m/s`.

Example:

```c
motor_speed_set(0.10f, 0.10f, 0.10f, 0.10f);
```

Arguments:

| Argument | Motor | Unit |
| --- | --- | --- |
| `A` | Left front | m/s |
| `B` | Right front | m/s |
| `C` | Right rear | m/s |
| `D` | Left rear | m/s |

Positive values represent forward wheel speed. Per-wheel output polarity is handled inside `app_motor_run()`.

#### Enable or Disable Closed Loop

```c
void app_motor_set_closed_loop(uint8_t enabled);
uint8_t app_motor_get_closed_loop(void);
```

Start four-wheel closed-loop control at 100 mm/s:

```c
motor_speed_set(0.10f, 0.10f, 0.10f, 0.10f);
app_motor_set_closed_loop(1);
```

Stop motors:

```c
motor_speed_set(0.0f, 0.0f, 0.0f, 0.0f);
app_motor_set_closed_loop(0);
```

After closed loop is enabled, `User/Components/y_timer/y_timer.c` automatically calls the control loop every 20 ms.

#### Closed-Loop Update Function

```c
void app_motor_run(void);
```

This function reads encoder counters, calculates real wheel speed, runs PID, and writes PWM outputs.

Application code normally should not call it repeatedly because the timer layer already calls it periodically when closed loop is enabled.

#### PID Parameters

PID implementation is in `User/Components/y_motor/y_motor.c`.

Current default parameters:

```c
float motor_kp = 800.0f;
float motor_ki = 35.0f;
float motor_kd = 400.0f;
```

Runtime PID parameter update:

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

### UART Motor Command

The UART command parser is implemented in `User/Components/y_global/y_global.c`.

Four-wheel target speed command:

```text
$Car:a,b,c,d!
```

Example:

```text
$Car:0.10,0.10,0.10,0.10!
```

This command calls:

```c
motor_speed_set(a, b, c, d);
```

Important: the current `$Car` command updates target speeds only. It does not automatically enable closed-loop control. Closed loop must be enabled in firmware with:

```c
app_motor_set_closed_loop(1);
```

Stop command:

```text
$DST!
```

### Where To Add Your Own Motor Control

Put high-level motion logic in application modules, not inside the PID or low-level PWM driver.

Recommended locations:

- `User/app_main.c`
- `User/app_sensor.c`
- `User/app_ps2.c`
- Command branches in `User/Components/y_global/y_global.c`

Typical pattern:

```c
app_motor_set_closed_loop(1);

// Later, whenever the target changes:
motor_speed_set(0.10f, 0.10f, 0.10f, 0.10f);
```

Avoid putting application behavior inside:

- `app_motor_run()`, unless changing the closed-loop control flow
- `SPEED_PidCtlA/B/C/D()`, unless changing the PID algorithm
- `MOTOR_A/B/C/D_SetSpeed()`, unless changing low-level PWM polarity or motor driver behavior

### Notes

- With a `100 mm/s` target, logs may show values such as `96` or `104 mm/s` because encoder counts are discrete over the 20 ms sampling period.
- If a motor or encoder is not connected while its target speed is nonzero, PID will continue increasing PWM because measured speed remains zero.
- Be conservative when testing closed-loop control with only one motor connected.

