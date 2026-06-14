/*
 * ================================================================================
 * @文件名称: app_sensor.c
 * @功能描述: 应用层传感器管理模块
 *           管理多种智能模式: 循迹(AI_mode=1)、自由避障(AI_mode=2)、跟随(AI_mode=3)
 *           包含普通循迹和增强循迹两种模式切换
 * @所属模块: User层
 * @依赖: app_sensor.h, tracking.h, ultrasonic.h
 * @模式说明:
 *   AI_mode=0: 空闲
 *   AI_mode=1: 智能循迹 (xunji_mode=0普通/xunji_mode=1增强)
 *   AI_mode=2: 自由避障
 *   AI_mode=3: 跟随
 *   AI_mode=255: 停止所有模式
 * ================================================================================
 */
///*
// * @文件描述:
// * @作者: Q
// * @Date: 2023-02-13 14:01:12
// * @LastEditTime: 2023-03-28 09:32:51
// */
#include "app_sensor.h"

#define ULTRASONIC_DEBUG_PERIOD_MS 500U
#define TRACKING_BASE_SPEED_NORMAL 0.16f
#define TRACKING_BASE_SPEED_PRO 0.20f
#define TRACKING_MAX_SPEED 0.32f
#define TRACKING_TURN_GAIN_NORMAL 0.012f
#define TRACKING_TURN_GAIN_PRO 0.015f
#define TRACKING_CROSS_PASS_SPEED 0.16f
#define TRACKING_RECOVERY_SPEED_INNER 0.03f
#define TRACKING_RECOVERY_SPEED_OUTER 0.22f
#define TRACKING_CROSS_ACTION_1 ""
#define TRACKING_CROSS_ACTION_2 ""

static uint32_t g_ultrasonic_distance_x100 = 0U;
static uint8_t g_ultrasonic_distance_valid = 0U;
static uint8_t g_tracking_cross_hold = 0U;

extern uint8_t xunji_mode;
extern int T_cross;

static float app_sensor_clamp_speed(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }

    if (value > max_value)
    {
        return max_value;
    }

    return value;
}

static void app_sensor_set_track_speed(float left_speed, float right_speed)
{
    motor_speed_set(left_speed, right_speed, left_speed, right_speed);
}

static uint8_t app_sensor_tracking_fetch(uint8_t *tracking_data)
{
    if ((tracking_data == 0) || (!tracking_copy_digital(tracking_data, TRACKING_CHANNEL_COUNT)) || (!tracking_is_online()))
    {
        return 0U;
    }

    return 1U;
}

static uint8_t app_sensor_tracking_has_main_line(const uint8_t *tracking_data)
{
    return (uint8_t)((tracking_data[2] == 0U) || (tracking_data[3] == 0U) ||
                     (tracking_data[4] == 0U) || (tracking_data[5] == 0U));
}

static uint8_t app_sensor_tracking_is_cross_pattern(const uint8_t *tracking_data)
{
    uint8_t index;
    uint8_t black_count = 0U;

    for (index = 0; index < TRACKING_CHANNEL_COUNT; index++)
    {
        if (tracking_data[index] == 0U)
        {
            black_count++;
        }
    }

    if (black_count >= 6U)
    {
        return 1U;
    }

    return (uint8_t)((tracking_data[1] == 0U) && (tracking_data[2] == 0U) &&
                     (tracking_data[3] == 0U) && (tracking_data[4] == 0U) &&
                     (tracking_data[5] == 0U) && (tracking_data[6] == 0U));
}

static void app_sensor_tracking_run_cross_action(uint8_t cross_index)
{
    const char *action_cmd = 0;

    if (cross_index == 1U)
    {
        action_cmd = TRACKING_CROSS_ACTION_1;
    }
    else if (cross_index == 2U)
    {
        action_cmd = TRACKING_CROSS_ACTION_2;
    }

    if ((action_cmd == 0) || (action_cmd[0] == '\0'))
    {
        return;
    }

    memset(cmd_return, 0, sizeof(cmd_return));
    strncpy(cmd_return, action_cmd, CMD_RETURN_SIZE - 1U);
    parse_cmd(cmd_return);
}

static void app_sensor_tracking_reset_cross_state(void)
{
    T_cross = 0;
    g_tracking_cross_hold = 0U;
}

static void app_sensor_tracking_follow_with_data(const uint8_t *tracking_data, uint8_t aggressive)
{
    static const int8_t weights[TRACKING_CHANNEL_COUNT] = {-7, -5, -3, -1, 1, 3, 5, 7};
    static int8_t last_error = 0;
    uint8_t index;
    uint8_t black_count = 0;
    int32_t weighted_sum = 0;
    int8_t error = 0;
    float base_speed;
    float turn_gain;
    float left_speed;
    float right_speed;

    if ((tracking_data[0] == 0U) && (tracking_data[1] == 0U) && (tracking_data[2] == 0U) &&
        (tracking_data[3] == 0U) && (tracking_data[4] == 1U) && (tracking_data[5] == 1U) &&
        (tracking_data[6] == 1U) && (tracking_data[7] == 1U))
    {
        error = -15;
    }
    else if ((tracking_data[0] == 1U) && (tracking_data[1] == 1U) && (tracking_data[2] == 1U) &&
             (tracking_data[3] == 1U) && (tracking_data[4] == 0U) && (tracking_data[5] == 0U) &&
             (tracking_data[6] == 0U) && (tracking_data[7] == 0U))
    {
        error = 15;
    }
    else if ((tracking_data[0] == 0U) && (tracking_data[7] == 0U))
    {
        error = 0;
    }
    else
    {
        for (index = 0; index < TRACKING_CHANNEL_COUNT; index++)
        {
            if (tracking_data[index] == 0U)
            {
                black_count++;
                weighted_sum += weights[index];
            }
        }

        if (black_count == 0U)
        {
            if (last_error < 0)
            {
                app_sensor_set_track_speed(TRACKING_RECOVERY_SPEED_INNER, TRACKING_RECOVERY_SPEED_OUTER);
            }
            else
            {
                app_sensor_set_track_speed(TRACKING_RECOVERY_SPEED_OUTER, TRACKING_RECOVERY_SPEED_INNER);
            }
            return;
        }

        error = (int8_t)(weighted_sum / (int32_t)black_count);
    }

    last_error = error;
    base_speed = aggressive ? TRACKING_BASE_SPEED_PRO : TRACKING_BASE_SPEED_NORMAL;
    turn_gain = aggressive ? TRACKING_TURN_GAIN_PRO : TRACKING_TURN_GAIN_NORMAL;

    left_speed = app_sensor_clamp_speed(base_speed + turn_gain * (float)error, 0.0f, TRACKING_MAX_SPEED);
    right_speed = app_sensor_clamp_speed(base_speed - turn_gain * (float)error, 0.0f, TRACKING_MAX_SPEED);
    app_sensor_set_track_speed(left_speed, right_speed);
}

static void app_sensor_tracking_follow_run(uint8_t aggressive)
{
    uint8_t tracking_data[TRACKING_CHANNEL_COUNT];

    if (!app_sensor_tracking_fetch(tracking_data))
    {
        app_sensor_set_track_speed(0.0f, 0.0f);
        return;
    }

    app_sensor_tracking_follow_with_data(tracking_data, aggressive);
}

static uint32_t ultrasonic_distance_to_x100(float distance_cm)
{
    if (distance_cm <= 0.0f)
    {
        return 0U;
    }

    return (uint32_t)(distance_cm * 100.0f + 0.5f);
}

static void ultrasonic_debug_log(const char *message)
{
    printf("%s", message);
    uart2_send_str((char *)message);
}

uint8_t app_sensor_refresh_ultrasonic_distance(void)
{
    float distance_cm = 0.0f;

    if (ultrasonic_measure_once(&distance_cm))
    {
        g_ultrasonic_distance_x100 = ultrasonic_distance_to_x100(distance_cm);
        g_ultrasonic_distance_valid = 1U;
        return 1U;
    }

    g_ultrasonic_distance_x100 = 0U;
    g_ultrasonic_distance_valid = 0U;
    return 0U;
}

static void ultrasonic_periodic_debug_run(void)
{
    static u32 last_debug_ms = 0;
    char message[96];
    u32 now = millis();
    uint32_t period_ms;

    period_ms = app_uart_is_ultrasonic_log_enabled() ?
                    app_uart_get_ultrasonic_log_period_ms() :
                    ULTRASONIC_DEBUG_PERIOD_MS;

    if ((now - last_debug_ms) < period_ms)
    {
        return;
    }

    last_debug_ms = now;

    if (app_sensor_refresh_ultrasonic_distance())
    {
        if (!app_uart_is_ultrasonic_log_enabled())
        {
            return;
        }

        snprintf(message, sizeof(message), "ultrasonic: %lu.%02lu cm\r\n",
                 (unsigned long)(g_ultrasonic_distance_x100 / 100U),
                 (unsigned long)(g_ultrasonic_distance_x100 % 100U));
    }
    else
    {
        if (!app_uart_is_ultrasonic_log_enabled())
        {
            return;
        }

        snprintf(message, sizeof(message), "ultrasonic: read failed\r\n");
    }

    ultrasonic_debug_log(message);
}

uint32_t app_sensor_get_ultrasonic_distance_x100(void)
{
    return g_ultrasonic_distance_x100;
}

uint8_t app_sensor_is_ultrasonic_distance_valid(void)
{
    return g_ultrasonic_distance_valid;
}

uint8_t app_sensor_get_tracking_state(uint8_t *buffer, uint8_t buffer_len)
{
    return tracking_copy_digital(buffer, buffer_len);
}

uint8_t app_sensor_get_tracking_mask(void)
{
    return tracking_get_digital_mask();
}

uint8_t app_sensor_is_tracking_online(void)
{
    return tracking_is_online();
}

uint8_t app_sensor_get_tracking_cross_count(void)
{
    return (uint8_t)T_cross;
}

uint8_t app_sensor_get_tracking_cross_hold(void)
{
    return g_tracking_cross_hold;
}

uint8_t app_sensor_get_tracking_mode(void)
{
    return xunji_mode;
}

/** 前向声明：各AI模式函数 */
static void AI_xunji_moshi(void);
void AI_ziyou_bizhang(void);
static void AI_xunji_moshi_pro(void);
void AI_gensui_moshi(void);

/** 循迹模式选择: 0=普通循迹(十字路口), 1=增强循迹(锐角/直角) */
uint8_t xunji_mode = 0;
/** 四路红外探头原始数据: 0=检测到黑线, 1=未检测到 */
uint8_t IR_X3, IR_X2, IR_X4, IR_X1;
/** 增强循迹转弯状态: 0=直走, 1=左转中, 2=右转中 */
int trackState = 0;
/** 十字路口计数(T_cross)和S弯防抖标志(flag_F)及转弯限制标志(forbid_F) */
int T_cross = 0, flag_F, forbid_F;

/*
 * 函数名称: app_sensor_init
 * 功能描述: 初始化传感器相关设备（循迹探头和超声波传感器）
 *           初始化四路红外循迹，检测超声波设备是否存在
 *           设置超声波RGB灯为绿色
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在系统初始化时调用一次
 */
void app_sensor_init(void)
{
    app_sensor_tracking_reset_cross_state();
    tracking_init();
    soft_i2c_gpio_init();

    if (ultrasonic_Init())
    {
        ultrasonic_debug_log("ultrasonic_Init succeed\r\n");
    }
    else
    {
        ultrasonic_debug_log("ultrasonic_Init fail\r\n");
    }

    ultrasonic_rgb_r(0, 255, 0);
    ultrasonic_rgb_t(0, 255, 0);
}

/*
 * 函数名称: app_sensor_run
 * 功能描述: 传感器主循环，根据AI_mode选择执行不同智能模式
 *           AI_mode模式切换时自动设置group_do_ok标志
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 在主循环中周期调用
 *           当group_do_ok==0（正在执行动作组）时跳过传感器处理
 */
void app_sensor_run(void)
{
    static u8 AI_mode_bak;

    tracking_run();
    ultrasonic_periodic_debug_run();

    if (group_do_ok == 0)
        return;

    if (AI_mode == 0)
    {
    }
    else if (AI_mode == 1)
    {
        if (xunji_mode == 0)
            AI_xunji_moshi();
        else
            AI_xunji_moshi_pro();
    }
    else if (AI_mode == 2)
    {
        AI_ziyou_bizhang();
    }
    else if (AI_mode == 3)
    {
        AI_gensui_moshi();
    }
    else if (AI_mode == 10)
    {
        AI_mode = 255;
    }

    if (AI_mode_bak != AI_mode)
    {
        if ((AI_mode_bak == 1U) && (AI_mode != 1U))
        {
            app_sensor_tracking_reset_cross_state();
        }

        AI_mode_bak = AI_mode;
        group_do_ok = 1;
    }
}

/*
 * 函数名称: AI_xunji_moshi
 * 功能描述: 普通循迹模式（四路红外十字路口循迹）
 *           根据四路红外探头状态控制小车沿黑线行走
 *           支持最多3个十字路口的识别和处理
 * 状态说明:
 *   state=0: 普通循迹，根据四路探头状态控制方向
 *   state=1: 第一次十字路口
 *   state=2: 第二次十字路口
 *   state=3: 第三次十字路口（后退出循迹模式）
 *   cross: 十字路口计数(0/1/2)
 *   cross_flag: 十字路口执行完成标志
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 由app_sensor_run()在AI_mode=1且xunji_mode=0时调用
 *           须先正确初始化四路红外探头
 */
static void AI_xunji_moshi(void)
{
    static uint8_t cross_flag = 0U;
    static uint8_t cross_state = 0U;
    static uint8_t cross_cycle = 0U;
    uint8_t tracking_data[TRACKING_CHANNEL_COUNT];

    if (!app_sensor_tracking_fetch(tracking_data))
    {
        app_sensor_set_track_speed(0.0f, 0.0f);
        return;
    }

    g_tracking_cross_hold = cross_flag;

    if (cross_flag == 1U)
    {
        app_sensor_set_track_speed(TRACKING_CROSS_PASS_SPEED, TRACKING_CROSS_PASS_SPEED);
        if ((!app_sensor_tracking_is_cross_pattern(tracking_data)) && app_sensor_tracking_has_main_line(tracking_data))
        {
            cross_flag = 0U;
            g_tracking_cross_hold = 0U;
        }
        return;
    }

    if (app_sensor_tracking_is_cross_pattern(tracking_data))
    {
        motor_speed_set(0.0f, 0.0f, 0.0f, 0.0f);
        if (cross_cycle == 0U)
        {
            cross_state = 1U;
        }
        else if (cross_cycle == 1U)
        {
            cross_state = 2U;
        }
        else
        {
            cross_state = 3U;
        }
    }
    else
    {
        app_sensor_tracking_follow_with_data(tracking_data, 0U);
    }

    switch (cross_state)
    {
    case 1:
        T_cross = 1;
        app_sensor_tracking_run_cross_action(1U);
        cross_state = 0U;
        cross_cycle = 1U;
        cross_flag = 1U;
        g_tracking_cross_hold = 1U;
        break;
    case 2:
        T_cross = 2;
        app_sensor_tracking_run_cross_action(2U);
        cross_state = 0U;
        cross_cycle = 2U;
        cross_flag = 1U;
        g_tracking_cross_hold = 1U;
        break;
    case 3:
        T_cross = 3;
        cross_state = 0U;
        cross_cycle = 0U;
        cross_flag = 0U;
        g_tracking_cross_hold = 0U;
        motor_speed_set(0.0f, 0.0f, 0.0f, 0.0f);
        if (AI_mode == 1)
        {
            AI_mode = 255;
        }
        break;
    default:
        break;
    }
}

/*
 * 函数名称: AI_xunji_moshi_pro
 * 功能描述: 增强循迹模式（支持锐角和直角转弯）
 *           相比普通模式增加了复杂路况处理能力
 *           通过trackState状态机实现: 0=正常/1=左转中/2=右转中
 * 关键变量:
 *   trackState: 0=正常直走, 1=持续左转, 2=持续右转
 *   flag_F: S弯防抖标志，防止X4探头频繁切换
 *   forbid_F: 限制标志
 *   forbid_turn: 地图适配标志，遇到特定T字路口时不转弯
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 由app_sensor_run()在AI_mode=1且xunji_mode=1时调用
 *           可通过调整电机速度参数优化循迹效果
 *           避免在此函数中使用delay_ms，会导致程序异常
 */
static void AI_xunji_moshi_pro(void)
{
    app_sensor_tracking_follow_run(1U);
}


/*
 * 函数名称: AI_ziyou_bizhang
 * 功能描述: 自由避障模式，根据超声波距离检测自动避开障碍物
 *           每100ms检测一次距离:
 *             距离 > 35cm (bz_num=3): 前进0.1m/s
 *             距离 15~35cm (bz_num=2): 左转避障
 *             距离 < 15cm (bz_num=1): 后退
 *           仅距离档次变化时才切换运动状态
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 由app_sensor_run()在AI_mode=2时调用
 */
void AI_ziyou_bizhang(void)
{
    static u32 systick_ms_bak = 0;
    static int bz_num = 0, bz_num_bak = 0;
    int adc_csb;
    if (millis() - systick_ms_bak > 100)
    {
        systick_ms_bak = millis();

        adc_csb = (int)ultrasonic_read_distance(); // 获取超声波值，计算出距离
        ultrasonic_start_measuring();
        // printf("adc_csb=%d\r\n", adc_csb);
        if ((adc_csb < 35) && (adc_csb > 15))
        {

            bz_num = 2;
        }
        else if (adc_csb < 15 && adc_csb > 0)
        {

            bz_num = 1;
        }
        else if (adc_csb >= 35)
        {

            bz_num = 3;
        }

        if (bz_num != bz_num_bak)
        {
            switch (bz_num)
            {
            case 1:
                motor_speed_set(-0.1, -0.1, -0.1, -0.1);
                break;
            case 2:
                motor_speed_set(-0.2, 0.2, -0.2, 0.2);
                break;
            case 3:
                motor_speed_set(0.1, 0.1, 0.1, 0.1);

                break;
            default:
                zx_uart_send_str("error\n");
                break;
            }
            bz_num_bak = bz_num;
        }
    }
}

/*
 * 函数名称: AI_gensui_moshi
 * 功能描述: 定距跟随模式，根据超声波测距结果控制小车保持跟随距离
 *           每100ms检测一次距离:
 *             距离 > 25cm: 前进0.1m/s
 *             距离 20~25cm: 停止（保持距离）
 *             距离 < 20cm: 后退0.1m/s
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 由app_sensor_run()在AI_mode=3时调用
 *           需要先初始化超声波传感器
 */
void AI_gensui_moshi(void)
{
    static u32 systick_ms_bak = 0;
    int adc_csb;
    if (millis() - systick_ms_bak > 100)
    {
        systick_ms_bak = millis();
        adc_csb = (int)ultrasonic_read_distance(); // 获取a0的ad值，计算出距离
        ultrasonic_start_measuring();

        if ((adc_csb > 25 ))
        {
            // 距离30~50cm前进
            motor_speed_set(0.1, 0.1, 0.1, 0.1);
        }
        else if ((adc_csb < 20) && (adc_csb > 0))
        {
            // 距离低于20cm就后退
            motor_speed_set(-0.1, -0.1, -0.1, -0.1
					
					);
        }
        else if ((adc_csb < 25
					) && (adc_csb > 20))
        {
            // 其他情况停止
            motor_speed_set(0, 0, 0, 0);
        }
    }
}
