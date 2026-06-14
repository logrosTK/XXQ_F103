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

### 串口调试

调试串口：

- USART1: `115200 8N1`
- TX: `PA9`
- RX: `PA10`

说明：板载 CH340 USB 转串口连接到 USART1，也就是用户通过 USB 线接电脑后看到的调试串口。
USART3(PB10/PB11) 已保留给八路循迹模块，不作为用户 USB 命令口使用。

上电后可直接发送：

```text
$HELP!
```

查询完整命令表。

默认调试行为：

- 电机闭环日志默认开启，周期 `3000 ms`
- 超声波周期日志默认关闭，但距离缓存仍会后台刷新
- 日志周期可运行时通过串口命令修改，无需重新编译

开启闭环且电机日志使能后，会周期输出电机闭环状态：

```text
CL t=... target[A,B,C,D]=[...] speed[A,B,C,D]=[...] pwm[A,B,C,D]=[...]
```

常用日志控制命令：

```text
$LOG:GET!
$LOG:MOTOR:1,1000!
$LOG:ULTRA:1,500!
$LOG:ULTRA:0!
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

### UART 调试命令

串口命令解析在 `User/Components/y_global/y_global.c`。

#### 新增调试命令

| 命令 | 作用 | 说明 |
| --- | --- | --- |
| `$HELP!` | 输出命令总览 | 上电后建议先发一次 |
| `$STATUS!` | 输出完整系统状态 | 包含闭环、AI 模式、PID、四轮目标/实测/PWM、超声波、循迹 |
| `$PID:GET!` | 读取 PID 参数 | 输出 `kp/ki/kd` |
| `$PID:RST!` | 复位 PID 状态 | 清积分和历史误差 |
| `$PID:kp,ki,kd!` | 设置 PID 参数 | 例：`$PID:800,35,400!` |
| `$ULTRA:GET!` | 立即刷新并输出超声波距离 | 输出厘米值或失败状态 |
| `$TRACK:GET!` | 输出循迹状态 | 包含在线状态、模式、mask、8 路数据、十字计数 |
| `$LOG:GET!` | 输出当前日志配置 | 查看电机/超声波日志开关和周期 |
| `$LOG:MOTOR:en,period!` | 配置电机日志 | `en=0/1`，`period` 单位 ms |
| `$LOG:ULTRA:en,period!` | 配置超声波日志 | `en=0/1`，`period` 单位 ms |
| `$MOTOR:CL:x!` | 设置闭环开关 | `x=0/1` |
| `$MOTOR:SET:a,b,c,d!` | 仅设置四轮目标速度 | 不主动打开闭环 |
| `$MOTOR:RUN:a,b,c,d!` | 设置四轮目标速度并打开闭环 | 最常用的直驱调试命令 |
| `$MOTOR:STOP!` | 停车并关闭闭环 | 会立即执行一次停止输出 |
| `$AI:STOP!` | 退出 AI 模式并停车 | 停止循迹/避障/跟随 |
| `$AI:TRACK!` | 进入普通循迹模式 | 自动打开闭环 |
| `$AI:TRACKPRO!` | 进入增强循迹模式 | 自动打开闭环 |
| `$AI:AVOID!` | 进入自由避障模式 | 自动打开闭环 |
| `$AI:FOLLOW!` | 进入定距跟随模式 | 自动打开闭环 |

#### 已兼容的旧运动命令

这些旧命令现在也会自动处理电机闭环，不再需要额外手动调用 `app_motor_set_closed_loop(1)`：

| 命令 | 作用 |
| --- | --- |
| `$Car:a,b,c,d!` | 设置四轮目标速度并自动打开闭环 |
| `$DST!` | 全部停止，关闭闭环，并停止所有舵机 |
| `$ZNXJ!` | 普通智能循迹 |
| `$ZYBZ!` | 自由避障 |
| `$DJGS!` | 定距跟随 |
| `$QJ!` | 前进 |
| `$HT!` | 后退 |
| `$ZZ!` | 左转 |
| `$YZ!` | 右转 |
| `$ZPY!` | 左平移 |
| `$YPY!` | 右平移 |
| `$TZ!` | 停止并关闭闭环 |

#### 已保留的原有系统/舵机/动作组命令

| 命令 | 作用 |
| --- | --- |
| `$DST:x!` | 停止指定舵机 |
| `$RST!` | 软件复位 STM32 |
| `$DGS:x!` | 执行第 `x` 个动作组一次 |
| `$DGT:x-y,z!` | 执行 `x` 到 `y` 号动作组，共 `z` 次 |
| `$DJR!` | 所有舵机回中位，并停车 |
| `$GETA!` | 返回应答 `AAA` |
| `$DRS!` | 返回测试字符串 |
| `$SMART_STOP!` | 执行原有智能停止流程 |
| `$KMS:x,y,z,time!` | 机械臂逆运动学定位 |
| `$MVx!` | OLED 功能页显示切换 |
| `$RunStop!` | 退出 OLED 功能显示 |
| `#xxxPyyyyTzzzz!` | 单舵机调试命令 |
| `#xxxPSCK+bbb!` / `#xxxPSCK-bbb!` | 舵机偏置校准 |
| `#xxxPDST!` | 停止指定舵机 |
| `{#...!#...!}` | 多舵机联动调试 |
| `<$!` | 清除开机预执行命令 |
| `<$...!>` | 设置开机预执行命令 |
| `<Gxxxx#...!>` | 保存动作组到 Flash |

#### 常用调试示例

启动四轮闭环并以 `0.10 m/s` 前进：

```text
$MOTOR:RUN:0.10,0.10,0.10,0.10!
```

查看完整系统状态：

```text
$STATUS!
```

把电机日志改为每 `1000 ms` 输出一次：

```text
$LOG:MOTOR:1,1000!
```

开启增强循迹：

```text
$AI:TRACKPRO!
```

兼容旧协议前进：

```text
$QJ!
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

### UART Debug

Debug UART:

- USART1: `115200 8N1`
- TX: `PA9`
- RX: `PA10`

Note: the onboard CH340 USB-to-UART bridge is wired to USART1, which is the user-facing USB serial port on the PC side.
USART3 (PB10/PB11) is reserved for the 8-channel tracking module and is not used as the user USB command port.

After boot, send:

```text
$HELP!
```

to print the full UART debug command list.

Default debug behavior:

- Motor closed-loop log is enabled by default with a `3000 ms` period
- Periodic ultrasonic log is disabled by default, but the cached distance still refreshes in the background
- Log periods can be changed at runtime over UART

When closed-loop control and motor logging are enabled, the firmware periodically prints:

```text
CL t=... target[A,B,C,D]=[...] speed[A,B,C,D]=[...] pwm[A,B,C,D]=[...]
```

Typical log control commands:

```text
$LOG:GET!
$LOG:MOTOR:1,1000!
$LOG:ULTRA:1,500!
$LOG:ULTRA:0!
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

### UART Debug Commands

The UART command parser is implemented in `User/Components/y_global/y_global.c`.

#### New debug commands

| Command | Function | Notes |
| --- | --- | --- |
| `$HELP!` | Print the command summary | Recommended first command after boot |
| `$STATUS!` | Print full system status | Includes closed-loop, AI mode, PID, wheel target/real/PWM, ultrasonic, and tracking state |
| `$PID:GET!` | Read PID parameters | Prints `kp/ki/kd` |
| `$PID:RST!` | Reset PID state | Clears integral/history |
| `$PID:kp,ki,kd!` | Update PID parameters | Example: `$PID:800,35,400!` |
| `$ULTRA:GET!` | Refresh and print ultrasonic distance | Prints distance in cm or failure |
| `$TRACK:GET!` | Print tracking status | Includes online state, mode, mask, 8-channel data, and cross counters |
| `$LOG:GET!` | Print current log configuration | Shows motor/ultrasonic log switches and periods |
| `$LOG:MOTOR:en,period!` | Configure motor log | `en=0/1`, `period` in ms |
| `$LOG:ULTRA:en,period!` | Configure ultrasonic log | `en=0/1`, `period` in ms |
| `$MOTOR:CL:x!` | Set closed-loop enable | `x=0/1` |
| `$MOTOR:SET:a,b,c,d!` | Set target wheel speeds only | Does not force closed-loop on |
| `$MOTOR:RUN:a,b,c,d!` | Set target wheel speeds and enable closed-loop | Best direct-drive debug command |
| `$MOTOR:STOP!` | Stop motors and disable closed-loop | Forces one immediate stop update |
| `$AI:STOP!` | Leave AI mode and stop | Stops tracking/avoid/follow modes |
| `$AI:TRACK!` | Enter normal tracking mode | Automatically enables closed-loop |
| `$AI:TRACKPRO!` | Enter aggressive tracking mode | Automatically enables closed-loop |
| `$AI:AVOID!` | Enter obstacle-avoid mode | Automatically enables closed-loop |
| `$AI:FOLLOW!` | Enter follow-distance mode | Automatically enables closed-loop |

#### Legacy motion commands kept for compatibility

These legacy commands now also manage motor closed-loop automatically, so no extra firmware-side `app_motor_set_closed_loop(1)` call is required.

| Command | Function |
| --- | --- |
| `$Car:a,b,c,d!` | Set four-wheel target speeds and automatically enable closed-loop |
| `$DST!` | Stop everything, disable closed-loop, and stop all servos |
| `$ZNXJ!` | Normal tracking mode |
| `$ZYBZ!` | Obstacle avoidance |
| `$DJGS!` | Distance following |
| `$QJ!` | Forward |
| `$HT!` | Backward |
| `$ZZ!` | Turn left |
| `$YZ!` | Turn right |
| `$ZPY!` | Translate left |
| `$YPY!` | Translate right |
| `$TZ!` | Stop and disable closed-loop |

#### Existing system, servo, and action-group commands still supported

| Command | Function |
| --- | --- |
| `$DST:x!` | Stop a specific servo |
| `$RST!` | Software-reset the STM32 |
| `$DGS:x!` | Run action group `x` once |
| `$DGT:x-y,z!` | Run action groups `x` through `y`, `z` times |
| `$DJR!` | Recenter all servos and stop motors |
| `$GETA!` | Return `AAA` |
| `$DRS!` | Return the test string |
| `$SMART_STOP!` | Execute the original smart-stop flow |
| `$KMS:x,y,z,time!` | Inverse-kinematics positioning command |
| `$MVx!` | Switch OLED function page |
| `$RunStop!` | Leave OLED function display mode |
| `#xxxPyyyyTzzzz!` | Single-servo debug command |
| `#xxxPSCK+bbb!` / `#xxxPSCK-bbb!` | Servo bias calibration |
| `#xxxPDST!` | Stop a specific servo |
| `{#...!#...!}` | Multi-servo joint debug command |
| `<$!` | Clear power-on pre-command |
| `<$...!>` | Set power-on pre-command |
| `<Gxxxx#...!>` | Save an action group to Flash |

#### Typical examples

Start four-wheel closed-loop driving at `0.10 m/s`:

```text
$MOTOR:RUN:0.10,0.10,0.10,0.10!
```

Query the full system status:

```text
$STATUS!
```

Change motor log period to `1000 ms`:

```text
$LOG:MOTOR:1,1000!
```

Enter aggressive tracking mode:

```text
$AI:TRACKPRO!
```

Forward using the legacy protocol:

```text
$QJ!
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

