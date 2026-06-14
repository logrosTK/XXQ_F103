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

static uint32_t g_ultrasonic_distance_x100 = 0U;
static uint8_t g_ultrasonic_distance_valid = 0U;

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

static void ultrasonic_periodic_debug_run(void)
{
    static u32 last_debug_ms = 0;
    char message[96];
    float distance_cm = 0.0f;
    uint32_t distance_x100 = 0;
    u32 now = millis();

    if ((now - last_debug_ms) < ULTRASONIC_DEBUG_PERIOD_MS)
    {
        return;
    }

    last_debug_ms = now;

    if (ultrasonic_measure_once(&distance_cm))
    {
        distance_x100 = ultrasonic_distance_to_x100(distance_cm);
        g_ultrasonic_distance_x100 = distance_x100;
        g_ultrasonic_distance_valid = 1U;
        snprintf(message, sizeof(message), "ultrasonic: %lu.%02lu cm\r\n",
                 (unsigned long)(distance_x100 / 100U),
                 (unsigned long)(distance_x100 % 100U));
    }
    else
    {
        g_ultrasonic_distance_x100 = 0U;
        g_ultrasonic_distance_valid = 0U;
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
    TRACK_IR4_Init();
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
    uint8_t IR_X3, IR_X2, IR_X4, IR_X1;
    static uint8_t cross_flag = 0, state = 0, cross = 0;

    IR_X3 = TRTACK_IR4_X3_READ();
    IR_X2 = TRTACK_IR4_X2_READ();
    IR_X1 = TRTACK_IR4_X1_READ();
    IR_X4 = TRTACK_IR4_X4_READ();
    // printf("%d  %d  %d  %d \r\n",IR_X1,IR_X2,IR_X3,IR_X4);

    /**********其中三个参数，state包含循迹状态与十字路口的三种状态
     * 0：普通循迹模式
     * 1：第一次遇到十字路口
     * 2：第二次遇到十字路口
     * 3：第三次遇到十字路口
     * cross是state在case 0中遇到十字路口第几次的标志位
     * cross_flag则是十字路口执行完成后的标志位
     **************************************/
    if (cross_flag == 1)
    {
        // 前进
        motor_speed_set(0.2, 0.2, 0.2, 0.2);
        if (IR_X1 == 1 || IR_X4 == 1)
            cross_flag = 0;
    }

    switch (state)
    {
    case 0:
        if (IR_X3 == 0 && IR_X2 == 0 && IR_X1 == 1 && IR_X4 == 1)
        {
            // 前进
            motor_speed_set(0.1, 0.1, 0.1, 0.1);
        }
        else if ((IR_X3 == 0 && IR_X2 == 1) || (IR_X2 == 1 && IR_X4 == 0 && IR_X1 == 1))
        {
            /* 右边出去，左转 */
            motor_speed_set(0, 0.1, 0, 0.1);
        }
        else if ((IR_X3 == 1 && IR_X2 == 0) || (IR_X3 == 1 && IR_X4 == 1 && IR_X1 == 0))
        {
            // 右转
            motor_speed_set(0.1, 0, 0.1, 0);
        }
        else if (IR_X3 == 0 && IR_X2 == 0 && IR_X1 == 0 && IR_X4 == 0 && (cross_flag == 0))
        {

            motor_speed_set(0, 0, 0, 0);
            if (cross == 0)
                state = 1;
            else if (cross == 1)
                state = 2;
            else if (cross == 2)
                state = 3;
        }
        break;
    case 1:
        if (AI_mode == 1)
        {
            // sprintf(cmd_return, "$DGT:%d-%d,%d!", 9, 17, 1); // 打印字符串到数组中
            // parse_cmd(cmd_return);   // 解析动作组
        }
        state = 0;
        cross = 1;
        cross_flag = 1;
        break;
    case 2:
        if (AI_mode == 1)
        {
            // sprintf(cmd_return, "$DGT:%d-%d,%d!", 9, 17, 1); // 打印字符串到数组中
            // parse_cmd(cmd_return);   // 解析动作组
        }
        state = 0;
        cross = 2;
        cross_flag = 1;
        break;
    case 3:
        state = 0;
        cross = 0;
        cross_flag = 0;
        if (AI_mode == 1)
        {
            AI_mode = 255;
        }
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
    IR_X3 = TRTACK_IR4_X3_READ();
    IR_X2 = TRTACK_IR4_X2_READ();
    IR_X1 = TRTACK_IR4_X1_READ();
    IR_X4 = TRTACK_IR4_X4_READ();

    //	 printf("%d  %d  %d  %d",IR_X1,IR_X2,IR_X3,IR_X4);
    //	delay_ms(2000);

    /**********其中三个参数，trackState表示直角或锐角转弯时的三种状态
     * 0：普通循迹模式
     * 1：左转情况，当小车遇到角度较小等复杂路况，则一直左转直至电平变为直走电平（1001）时才恢复正常
     * 2：右转情况，当小车遇到角度较小等复杂路况，则一直右转直至电平变为直走电平（1001）时才恢复正常
     * flag_F是为了限制在S弯行进时X4探头频繁切换状态
     * forbid_turn用于适配地图，当遇到第1和第3个T字路口时都不会转弯而是直走
     * 用户在使用过程中，可通过调整IX\IY\IW三个值而达到好的循迹效果
     * 尽量避免使用delay_ms函数，会造成程序异常
     **************************************/
    switch (trackState)
    {
    case 0:
        if ((IR_X1 == 1 && IR_X2 == 1 && IR_X3 == 1 && IR_X4 == 1 && flag_F != 1))
        {
            motor_speed_set(0.4, 0, 0.4, 0);
        }
        if ((IR_X1 == 1 && IR_X2 == 0 && IR_X3 == 0 && IR_X4 == 1))
        {
            // 四路传感器都检测到黑线，前进
            motor_speed_set(0.2, 0.2, 0.2, 0.2);
            flag_F = 0;
            forbid_F = 0;

            if (forbid_turn == 1 || forbid_turn == 4)
            {
                forbid_turn = 2;
            }
        }
        else if (IR_X1 == 1 && IR_X2 == 0 && IR_X3 == 0 && IR_X4 == 0)
        {
            // 左大弯
            motor_speed_set(0.05, 0.4, 0.05, 0.4);
            trackState = 1;
        }
        else if (IR_X1 == 0 && IR_X2 == 0 && IR_X3 == 0 && IR_X4 == 1)
        {
            if (forbid_turn == 0)
            {
                motor_speed_set(0.2, 0.2, 0.2, 0.2);
                forbid_turn = 1;
            }
            else if (forbid_turn == 3)
            {

                motor_speed_set(0.2, 0.2, 0.2, 0.2);
                forbid_turn = 4;
            }
            else if (forbid_turn != 0 && forbid_turn != 1 && forbid_turn != 4)
            {
                // 右大弯
                motor_speed_set(0.4, 0.05, 0.4, 0.05);
                trackState = 2;
            }
        }
        else if (IR_X4 == 0 && forbid_F != 1)
        {
            // 左最外侧检测
            motor_speed_set(0.1, 1, 0.1, 1);
            flag_F = 1;
        }
        else if (IR_X1 == 0)
        {
            // 右最外侧检测
            motor_speed_set(1, 0.1, 1, 0.1);
        }
        else if (IR_X1 == 1 && IR_X2 == 1 && IR_X3 == 0 && IR_X4 == 1)
        {
            // 中间黑线上的传感器微调车左转
            motor_speed_set(0.2, 0.4, 0.2, 0.4);
        }
        else if (IR_X1 == 1 && IR_X2 == 0 && IR_X3 == 1 && IR_X4 == 1)
        {
            // 中间黑线上的传感器微调车右转
            motor_speed_set(0.4, 0.2, 0.4, 0.2);
        }
        break;
    case 1:
        motor_speed_set(0.08, 0.58, 0.08, 0.58);
        if (IR_X1 == 1 && IR_X2 == 0 && IR_X3 == 0 && IR_X4 == 1)
        {
            trackState = 0;
            forbid_F = 1;
            delay_ms(200);
        }
        break;
    case 2:
        motor_speed_set(0.58, 0.08, 0.58, 0.08);
        if (IR_X1 == 1 && IR_X2 == 0 && IR_X3 == 0 && IR_X4 == 1)
        {
            delay_ms(200);
            trackState = 0;
            forbid_turn = 3;
        }
    }
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
