/*
 * ================================================================================
 * @文件名称: y_global.c
 * @功能描述: 全局功能模块，包含命令解析、动作组管理、运动学控制等核心功能
 *           是整个系统的命令处理和动作执行中心
 * @所属模块: Components/y_global
 * @依赖: y_global.h, stm32f10x_conf.h, y_flash.h, y_servo.h, y_kinematics.h
 * @命令列表:
 *   $DST!     - 所有舵机/电机停止
 *   $DST:x!   - 第x个舵机停止
 *   $RST!     - 单片机重启
 *   $DGS:x!   - 执行第x个动作组
 *   $DGT:x-y,z! - 执行x到y组动作z次
 *   $DJR!     - 所有舵机复位到中位(1500)
 *   $Car:a,b,c,d! - 小车四轮速度设置
 *   $ZNXJ!    - 智能循迹模式
 *   $ZYBZ!    - 自由避障模式
 *   $DJGS!    - 定距跟随模式
 *   $QJ/$HT/$ZZ/$YZ/$ZPY/$YPY/$TZ - 前进/后退/左转/右转/左平移/右平移/停止
 *   $KMS:x,y,z,time! - 机械臂运动学控制
 *   $MVx!     - OLED显示模式切换
 *   $GETA!    - 获取应答信号
 *   #XXXPxxxxTxxxx! - 舵机调试命令
 *   <Gxxxx#xxxPxxxxTxxxx!> - 动作组存储命令
 * ================================================================================
 */

#include "./y_global/y_global.h"
#include "stm32f10x_conf.h"

/** 命令解析临时缓冲区，用于存放处理后返回的字符串 */
char cmd_return[CMD_RETURN_SIZE];
/** 动作组执行完成标志: 0=正在执行, 1=执行完毕 */
u8 group_do_ok = 1;

/** AI模式: 0=空闲, 1=循迹, 2=避障, 3=跟随, 255=停止 */
u8 AI_mode = 255;
/** 循迹模式地图适配标志 */
u8 forbid_turn = 0;
extern uint8_t xunji_mode;

/** 控制板EEPROM存储信息结构体 */
eeprom_info_t eeprom_info;

/** 动作组执行时间（ms），由getMaxTime计算 */
u32 action_time = 0;
/** 动作组批量执行起始索引 */
int do_start_index;
/** 动作组批量执行剩余次数 */
int do_group_cnt;
/** 动作组起始编号 */
int group_num_start;
/** 动作组终止编号 */
int group_num_end;
/** 动作组执行次数 */
int group_num_cnt;
/** OLED显示模式: 0=速度显示, 1=功能显示 */
int oled_mode = 0;
/** OLED功能显示子模式 */
int mv_mode = 0;

/*
 * 函数名称: soft_reset
 * 功能描述: 单片机软件复位
 *           关闭所有中断后调用NVIC_SystemReset()复位
 * 参数说明: 无
 * 返回值:   无
 * 使用说明: 通过$RST!命令触发，用于远程重启系统
 */
void soft_reset(void)
{
    printf("stm32 reset\r\n");
    __set_FAULTMASK(1);
    NVIC_SystemReset();
}

static void debug_stop_all_motion(void)
{
    AI_mode = 255;
    app_motor_set_closed_loop(0);
    motor_speed_set(0.0f, 0.0f, 0.0f, 0.0f);
    app_motor_run();
}

static void debug_set_manual_speed(float a, float b, float c, float d, uint8_t enable_closed_loop)
{
    AI_mode = 255;
    motor_speed_set(a, b, c, d);
    if (enable_closed_loop)
    {
        app_motor_set_closed_loop(1);
    }
}

static void debug_enter_ai_mode(uint8_t mode, uint8_t tracking_mode)
{
    AI_mode = mode;
    if (mode == 1U)
    {
        xunji_mode = tracking_mode;
    }
    app_motor_set_closed_loop(1);
}

static void uart_reply_ok(const char *message)
{
    if ((message != 0) && (message[0] != '\0'))
    {
        printf("@OK %s\r\n", message);
        return;
    }

    printf("@OK\r\n");
}

static void uart_reply_err(const char *message)
{
    if ((message != 0) && (message[0] != '\0'))
    {
        printf("@ERR %s\r\n", message);
        return;
    }

    printf("@ERR\r\n");
}

static uint8_t parse_decimal_float(const char *text, const char **end_ptr, float *value)
{
    const char *cursor = text;
    float integer_part = 0.0f;
    float fraction_part = 0.0f;
    float fraction_scale = 1.0f;
    float sign = 1.0f;
    uint8_t has_digit = 0U;

    if ((cursor == 0) || (value == 0) || (end_ptr == 0))
    {
        return 0U;
    }

    if (*cursor == '+')
    {
        cursor++;
    }
    else if (*cursor == '-')
    {
        sign = -1.0f;
        cursor++;
    }

    while ((*cursor >= '0') && (*cursor <= '9'))
    {
        integer_part = integer_part * 10.0f + (float)(*cursor - '0');
        cursor++;
        has_digit = 1U;
    }

    if (*cursor == '.')
    {
        cursor++;
        while ((*cursor >= '0') && (*cursor <= '9'))
        {
            fraction_part = fraction_part * 10.0f + (float)(*cursor - '0');
            fraction_scale *= 10.0f;
            cursor++;
            has_digit = 1U;
        }
    }

    if (!has_digit)
    {
        return 0U;
    }

    *value = sign * (integer_part + fraction_part / fraction_scale);
    *end_ptr = cursor;
    return 1U;
}

static uint8_t parse_float_list4_command(const char *cmd, const char *prefix,
                                         float *value0, float *value1, float *value2, float *value3)
{
    const char *cursor = cmd;

    if ((cmd == 0) || (prefix == 0) || (value0 == 0) || (value1 == 0) || (value2 == 0) || (value3 == 0))
    {
        return 0U;
    }

    while (*prefix != '\0')
    {
        if (*cursor != *prefix)
        {
            return 0U;
        }
        cursor++;
        prefix++;
    }

    if (!parse_decimal_float(cursor, &cursor, value0) || (*cursor != ','))
    {
        return 0U;
    }
    cursor++;

    if (!parse_decimal_float(cursor, &cursor, value1) || (*cursor != ','))
    {
        return 0U;
    }
    cursor++;

    if (!parse_decimal_float(cursor, &cursor, value2) || (*cursor != ','))
    {
        return 0U;
    }
    cursor++;

    if (!parse_decimal_float(cursor, &cursor, value3) || (*cursor != '!') || (cursor[1] != '\0'))
    {
        return 0U;
    }

    return 1U;
}

static uint8_t parse_float_list3_command(const char *cmd, const char *prefix,
                                         float *value0, float *value1, float *value2)
{
    const char *cursor = cmd;

    if ((cmd == 0) || (prefix == 0) || (value0 == 0) || (value1 == 0) || (value2 == 0))
    {
        return 0U;
    }

    while (*prefix != '\0')
    {
        if (*cursor != *prefix)
        {
            return 0U;
        }
        cursor++;
        prefix++;
    }

    if (!parse_decimal_float(cursor, &cursor, value0) || (*cursor != ','))
    {
        return 0U;
    }
    cursor++;

    if (!parse_decimal_float(cursor, &cursor, value1) || (*cursor != ','))
    {
        return 0U;
    }
    cursor++;

    if (!parse_decimal_float(cursor, &cursor, value2) || (*cursor != '!') || (cursor[1] != '\0'))
    {
        return 0U;
    }

    return 1U;
}

static uint8_t parse_kms_command(const char *cmd, float *x, float *y, float *z, int *time_ms)
{
    const char *cursor = cmd;

    if ((cmd == 0) || (x == 0) || (y == 0) || (z == 0) || (time_ms == 0))
    {
        return 0U;
    }

    if (strncmp(cursor, "$KMS:", 5) != 0)
    {
        return 0U;
    }
    cursor += 5;

    if (!parse_decimal_float(cursor, &cursor, x) || (*cursor != ','))
    {
        return 0U;
    }
    cursor++;

    if (!parse_decimal_float(cursor, &cursor, y) || (*cursor != ','))
    {
        return 0U;
    }
    cursor++;

    if (!parse_decimal_float(cursor, &cursor, z) || (*cursor != ','))
    {
        return 0U;
    }
    cursor++;

    if (sscanf(cursor, "%d!", time_ms) != 1)
    {
        return 0U;
    }

    return 1U;
}

/*
 * 函数名称: zx_uart_send_str
 * 功能描述: 总线串口发送字符串（同时发送到UART5和UART1）
 * 参数说明: str - 要发送的字符串指针
 * 返回值:   无
 * 使用说明: UART5为半双工总线，UART1为调试输出
 */
void zx_uart_send_str(char *str)
{
    uart5_send_str(str);
    uart1_send_str(str);
}

/*
 * 函数名称: all_uart_send_str
 * 功能描述: 向所有串口发送字符串（UART1/2/3/5）
 * 参数说明: str - 要发送的字符串指针
 * 返回值:   无
 * 使用说明: 用于向所有连接的设备广播数据
 */
void all_uart_send_str(char *str)
{
    uart5_send_str(str);
    uart1_send_str(str);
    uart2_send_str(str);
    uart3_send_str(str);
}

/*
 * 函数名称: str_contain_str
 * 功能描述: 检测字符串str中是否包含子串str2
 * 参数说明: str - 源字符串指针
 *          str2 - 要查找的子串指针
 * 返回值:   0 - 未找到
 *           非0 - str2在str中的位置（从1开始计数）
 * 使用说明: 在命令解析中用于匹配命令关键字
 *           示例: if(str_contain_str(cmd, "$DST!")) { ... }
 */
uint16_t str_contain_str(char *str, char *str2)
{
    char *str_temp, *str_temp2;
    str_temp = str;
    str_temp2 = str2;
    while (*str_temp)
    {
        if (*str_temp == *str_temp2)
        {
            while (*str_temp2)
            {
                if (*str_temp++ != *str_temp2++)
                {
                    str_temp = str_temp - (str_temp2 - str2) + 1;
                    str_temp2 = str2;
                    break;
                }
            }
            if (!*str_temp2)
            {
                return (str_temp - str);
            }
        }
        else
        {
            str_temp++;
        }
    }
    return 0;
}

/*
 * 函数名称: replace_char
 * 功能描述: 字符串中的字符替换，将str中所有ch1替换为ch2
 * 参数说明: str - 字符串指针（原地修改）
 *          ch1 - 要被替换的字符
 *          ch2 - 替换后的字符
 * 返回值:   无
 * 使用说明: 用于存储动作组时将<>替换为{}存储到Flash
 */
void replace_char(char *str, char ch1, char ch2)
{
    while (*str)
    {
        if (*str == ch1)
        {
            *str = ch2;
        }
        str++;
    }
    return;
}

/*
 * 函数名称: parse_action
 * 功能描述: 解析#开头的舵机控制字符串
 *           支持格式:
 *           #xxxPyyyyTzzzz! - 设置xxx号舵机PWM为yyyy，时间zzzzms
 *           #xxxPSCK+bbb!    - 设置xxx号舵机偏置+bbb
 *           #xxxPSCK-bbb!    - 设置xxx号舵机偏置-bbb
 *           #xxxPDST!        - 停止xxx号舵机
 * 参数说明: str - 要解析的字符串指针
 * 返回值:   无
 * 使用说明: 无阻塞，立即设置舵机运动参数
 *           支持多个#xxxPyyyyTzzzz!连续解析
 */
void parse_action(char *str)
{
    u16 index = 0, time = 0, i = 0, j = 0;
    int len = 0, bias = 0;
    float pwm = 0;

    zx_uart_send_str(str);
    len = strlen((char *)str);

    if (len >= 12 && str[0] == '#' && str[4] == 'P' && str[5] == 'S' && str[6] == 'C' && str[7] == 'K' && str[12] == '!')
    {
        index = (str[1] - '0') * 100 + (str[2] - '0') * 10 + (str[3] - '0');
        bias = (str[9] - '0') * 100 + (str[10] - '0') * 10 + (str[11] - '0');
        if (bias <= 200 && index < SERVO_NUM)
        {
            if (str[8] == '+')
            {
                pwmServo_bias_set(index, bias);
                eeprom_info.dj_bias_pwm[index] = bias;
            }
            else if (str[8] == '-')
            {
                pwmServo_bias_set(index, -bias);
                eeprom_info.dj_bias_pwm[index] = -bias;
            }
            rewrite_eeprom();
        }
    }
    else if (len >= 8 && str[0] == '#' && str[4] == 'P' && str[5] == 'D' && str[6] == 'S' && str[7] == 'T' && str[8] == '!')
    {
        index = (str[1] - '0') * 100 + (str[2] - '0') * 10 + (str[3] - '0');
        pwmServo_stop_motion(index);
        return;
    }

    while ((len >= i) && str[i])
    {
        if (str[i] == '#')
        {
            index = 0;
            i++;
            for (j = 0; j < 3; j++)
            {
                if ((len >= i) && str[i] <= '9' && str[i] >= '0')
                {
                    index = index * 10 + str[i] - '0';
                    i++;
                }
                else
                {
                    goto err;
                }
            }
        }
        else if (str[i] == 'P')
        {
            pwm = 0;
            i++;
            for (j = 0; j < 4; j++)
            {
                if ((len >= i) && str[i] <= '9' && str[i] >= '0')
                {
                    pwm = pwm * 10 + str[i] - '0';
                    i++;
                }
                else
                {
                    goto err;
                }
            }
        }
        else if (str[i] == 'T')
        {
            time = 0;
            i++;
            for (j = 0; j < 4; j++)
            {
                if ((len >= i) && str[i] <= '9' && str[i] >= '0')
                {
                    time = time * 10 + str[i] - '0';
                    i++;
                }
                else
                {
                    goto err;
                }
            }

            pwmServo_angle_set(index, pwm, time);
        }
        else
        {
        err:
            i++;
        }
    }
}

/*
 * 函数名称: save_action
 * 功能描述: 动作组保存函数（仅处理用<>包含的字符串）
 *           支持三种格式:
 *           <$!          - 删除开机动作
 *           <$xxx!       - 设置开机动作
 *           <Gxxxx#xxxPxxxxTxxxx!> - 存储第xxxx号动作组
 * 参数说明: str - 要解析的字符串指针
 * 返回值:   无
 * 使用说明: 通过串口接收的<>格式命令调用
 *           动作组存储到W25Q64 Flash中
 *           存储成功回复"A"，失败回复"E"
 */
void save_action(char *str)
{
    int32_t action_index = -1;

    if (str[1] == '$' && str[2] == '!')
    {
        eeprom_info.pre_cmd[PRE_CMD_SIZE] = 0;
        rewrite_eeprom();
        uart1_send_str("@CLEAR PRE_CMD OK!");
        return;
    }
    else if (str[1] == '$')
    {
        memset(eeprom_info.pre_cmd, 0, sizeof(eeprom_info.pre_cmd));
        strcpy(eeprom_info.pre_cmd, str + 1);
        eeprom_info.pre_cmd[strlen(str) - 2] = '\0';
        eeprom_info.pre_cmd[PRE_CMD_SIZE] = FLAG_VERIFY;
        rewrite_eeprom();
        all_uart_send_str("@SET PRE_CMD OK!");
        all_uart_send_str(eeprom_info.pre_cmd);
        return;
    }

    action_index = (str[2] - '0') * 1000 + (str[3] - '0') * 100 + (str[4] - '0') * 10 + (str[5] - '0');

    if ((action_index < 0) || str[6] != '#')
    {
        all_uart_send_str("E");
        return;
    }

    if ((action_index * ACTION_SIZE % 4096) == 0)
    {
        w25x_erase_sector(action_index * ACTION_SIZE / 4096);
    }
    replace_char(str, '<', '{');
    replace_char(str, '>', '}');

    w25x_write((u8 *)str, action_index * ACTION_SIZE, strlen(str) + 1);

    all_uart_send_str("A");

    return;
}

/*
 * 支持的串口命令列表:
 *   $DRS!            - 测试应答 "hello word!"
 *   $DST!            - 全部停止
 *   $DST:x!          - 停止第x号舵机
 *   $RST!            - 软件复位
 *   $DGS:x!          - 执行第x号动作组1次
 *   $DGT:x-y,z!      - 执行x到y号动作组z次
 *   $DJR!            - 所有舵机复位到中位
 *   $Car:a,b,c,d!    - 小车四轮速度
 *   $ZNXJ!           - 智能循迹
 *   $ZYBZ!           - 自由避障
 *   $DJGS!           - 定距跟随
 *   $QJ!             - 前进
 *   $HT!             - 后退
 *   $ZZ!             - 左转
 *   $YZ!             - 右转
 *   $ZPY!            - 左平移
 *   $YPY!            - 右平移
 *   $TZ!             - 停止
 *   $GETA!           - 应答"AAA"
 *   $SMART_STOP!     - 智能停止
 *   $KMS:x,y,z,time! - 机械臂运动学定位
 *   $MVx!            - OLED显示切换
 *   $RunStop!        - 停止OLED功能显示
 */

/*
 * 函数名称: parse_cmd
 * 功能描述: 命令解析函数，解析$开头的所有系统命令
 *           根据命令关键字分发到不同的处理逻辑
 * 参数说明: cmd - 命令字符串指针
 * 返回值:   无
 * 使用说明: 由串口接收处理函数调用
 *           命令格式: $XXX:参数!
 *           支持多种命令，详见上方命令列表
 */
void parse_cmd(char *cmd)
{
    int pos = 0, index = 0, int1 = 0, int4 = 0;
    uint8_t handled = 0U;
    float kinematics_x = 0, kinematics_y = 0, kinematics_z = 0;
    int mode_num;
    const unsigned char *display_chars;
    int num_chars;

    if (pos = str_contain_str(cmd, "$DRS!"), pos)
    {
        handled = 1U;
        uart1_send_str("hello word!");
        uart_reply_ok("DRS");
    }
    else if (pos = str_contain_str(cmd, "$HELP!"), pos)
    {
        handled = 1U;
        app_uart_print_help();
        uart_reply_ok("HELP");
    }
    else if (pos = str_contain_str(cmd, "$STATUS!"), pos)
    {
        handled = 1U;
        (void)app_sensor_refresh_ultrasonic_distance();
        app_uart_print_status();
        uart_reply_ok("STATUS");
    }
    else if (pos = str_contain_str(cmd, "$PID:GET!"), pos)
    {
        handled = 1U;
        app_uart_print_pid();
        uart_reply_ok("PID GET");
    }
    else if (pos = str_contain_str(cmd, "$PID:RST!"), pos)
    {
        handled = 1U;
        SPEED_PidReset();
        printf("PID reset ok\r\n");
        app_uart_print_pid();
        uart_reply_ok("PID RST");
    }
    else if (pos = str_contain_str(cmd, "$PID:"), pos)
    {
        float kp = 0.0f, ki = 0.0f, kd = 0.0f;
        handled = 1U;
        if (parse_float_list3_command(cmd, "$PID:", &kp, &ki, &kd))
        {
            SPEED_PidSetParam(kp, ki, kd);
            printf("PID set ok\r\n");
            app_uart_print_pid();
            uart_reply_ok("PID SET");
        }
        else
        {
            uart_reply_err("PID format");
        }
    }
    else if (pos = str_contain_str(cmd, "$ULTRA:GET!"), pos)
    {
        handled = 1U;
        (void)app_sensor_refresh_ultrasonic_distance();
        app_uart_print_ultrasonic_status();
        uart_reply_ok("ULTRA GET");
    }
    else if (pos = str_contain_str(cmd, "$TRACK:GET!"), pos)
    {
        handled = 1U;
        app_uart_print_tracking_status();
        uart_reply_ok("TRACK GET");
    }
    else if (pos = str_contain_str(cmd, "$LOG:GET!"), pos)
    {
        handled = 1U;
        app_uart_print_log_config();
        uart_reply_ok("LOG GET");
    }
    else if (pos = str_contain_str(cmd, "$LOG:MOTOR:"), pos)
    {
        handled = 1U;
        if (sscanf((char *)cmd, "$LOG:MOTOR:%d,%d!", &int1, &int4) == 2)
        {
            app_uart_set_motor_log((uint8_t)int1, (uint32_t)int4);
            app_uart_print_log_config();
            uart_reply_ok("LOG MOTOR");
        }
        else if (sscanf((char *)cmd, "$LOG:MOTOR:%d!", &int1) == 1)
        {
            app_uart_set_motor_log((uint8_t)int1, app_uart_get_motor_log_period_ms());
            app_uart_print_log_config();
            uart_reply_ok("LOG MOTOR");
        }
        else
        {
            uart_reply_err("LOG MOTOR format");
        }
    }
    else if (pos = str_contain_str(cmd, "$LOG:ULTRA:"), pos)
    {
        handled = 1U;
        if (sscanf((char *)cmd, "$LOG:ULTRA:%d,%d!", &int1, &int4) == 2)
        {
            app_uart_set_ultrasonic_log((uint8_t)int1, (uint32_t)int4);
            app_uart_print_log_config();
            uart_reply_ok("LOG ULTRA");
        }
        else if (sscanf((char *)cmd, "$LOG:ULTRA:%d!", &int1) == 1)
        {
            app_uart_set_ultrasonic_log((uint8_t)int1, app_uart_get_ultrasonic_log_period_ms());
            app_uart_print_log_config();
            uart_reply_ok("LOG ULTRA");
        }
        else
        {
            uart_reply_err("LOG ULTRA format");
        }
    }
    else if (pos = str_contain_str(cmd, "$MOTOR:STOP!"), pos)
    {
        handled = 1U;
        debug_stop_all_motion();
        printf("MOTOR stop ok\r\n");
        app_uart_print_status();
        uart_reply_ok("MOTOR STOP");
    }
    else if (pos = str_contain_str(cmd, "$MOTOR:CL:"), pos)
    {
        handled = 1U;
        if (sscanf((char *)cmd, "$MOTOR:CL:%d!", &int1) == 1)
        {
            app_motor_set_closed_loop((uint8_t)int1);
            if (int1 == 0)
            {
                app_motor_run();
            }
            app_uart_print_status();
            uart_reply_ok("MOTOR CL");
        }
        else
        {
            uart_reply_err("MOTOR CL format");
        }
    }
    else if (pos = str_contain_str(cmd, "$MOTOR:SET:"), pos)
    {
        float a = 0.0f, b = 0.0f, c = 0.0f, d = 0.0f;
        handled = 1U;
        if (parse_float_list4_command(cmd, "$MOTOR:SET:", &a, &b, &c, &d))
        {
            debug_set_manual_speed(a, b, c, d, 0U);
            printf("MOTOR target set ok\r\n");
            app_uart_print_status();
            uart_reply_ok("MOTOR SET");
        }
        else
        {
            uart_reply_err("MOTOR SET format");
        }
    }
    else if (pos = str_contain_str(cmd, "$MOTOR:RUN:"), pos)
    {
        float a = 0.0f, b = 0.0f, c = 0.0f, d = 0.0f;
        handled = 1U;
        if (parse_float_list4_command(cmd, "$MOTOR:RUN:", &a, &b, &c, &d))
        {
            debug_set_manual_speed(a, b, c, d, 1U);
            printf("MOTOR run ok\r\n");
            app_uart_print_status();
            uart_reply_ok("MOTOR RUN");
        }
        else
        {
            uart_reply_err("MOTOR RUN format");
        }
    }
    else if (pos = str_contain_str(cmd, "$AI:STOP!"), pos)
    {
        handled = 1U;
        debug_stop_all_motion();
        printf("AI stop ok\r\n");
        app_uart_print_status();
        uart_reply_ok("AI STOP");
    }
    else if (pos = str_contain_str(cmd, "$AI:TRACKPRO!"), pos)
    {
        handled = 1U;
        debug_enter_ai_mode(1U, 1U);
        printf("AI mode=TRACKPRO\r\n");
        app_uart_print_status();
        uart_reply_ok("AI TRACKPRO");
    }
    else if (pos = str_contain_str(cmd, "$AI:TRACK!"), pos)
    {
        handled = 1U;
        debug_enter_ai_mode(1U, 0U);
        printf("AI mode=TRACK\r\n");
        app_uart_print_status();
        uart_reply_ok("AI TRACK");
    }
    else if (pos = str_contain_str(cmd, "$AI:AVOID!"), pos)
    {
        handled = 1U;
        debug_enter_ai_mode(2U, 0U);
        motor_speed_set(0.3f, 0.3f, 0.3f, 0.3f);
        printf("AI mode=AVOID\r\n");
        app_uart_print_status();
        uart_reply_ok("AI AVOID");
    }
    else if (pos = str_contain_str(cmd, "$AI:FOLLOW!"), pos)
    {
        handled = 1U;
        debug_enter_ai_mode(3U, 0U);
        printf("AI mode=FOLLOW\r\n");
        app_uart_print_status();
        uart_reply_ok("AI FOLLOW");
    }
    else if (pos = str_contain_str(cmd, "$DST!"), pos)
    {
        handled = 1U;
        group_do_ok = 1;
        pwmServo_stop_motion(255);
        all_uart_send_str("#255PDST!");
        debug_stop_all_motion();
        uart_reply_ok("DST");
    }
    else if (pos = str_contain_str(cmd, "$DST:"), pos)
    {
        handled = 1U;
        if (sscanf((char *)cmd, "$DST:%d!", &index))
        {
            pwmServo_stop_motion(index);
            sprintf((char *)cmd_return, "#%03dPDST!\r\n", (int)index);
            all_uart_send_str(cmd_return);
            memset(cmd_return, 0, sizeof(cmd_return));
            uart_reply_ok("DST servo");
        }
        else
        {
            uart_reply_err("DST format");
        }
    }
    else if (pos = str_contain_str(cmd, "$RST!"), pos)
    {
        handled = 1U;
        uart_reply_ok("RST");
        soft_reset();
    }
    else if (pos = str_contain_str(cmd, "$DGS:"), pos)
    {
        handled = 1U;
        if (sscanf((char *)cmd, "$DGS:%d!", &int1))
        {
            group_do_ok = 1;
            do_group_once(int1);
            uart_reply_ok("DGS");
        }
        else
        {
            uart_reply_err("DGS format");
        }
    }
    else if (pos = str_contain_str(cmd, "$DGT:"), pos)
    {
        handled = 1U;
        if (sscanf((char *)cmd, "$DGT:%d-%d,%d!", &group_num_start, &group_num_end, &group_num_cnt))
        {
            group_do_ok = 1;
            if (group_num_start != group_num_end)
            {
                do_start_index = group_num_start;
                do_group_cnt = group_num_cnt;
                group_do_ok = 0;
            }
            else
            {
                do_group_once(group_num_start);
            }
            uart_reply_ok("DGT");
        }
        else
        {
            uart_reply_err("DGT format");
        }
    }
    else if (pos = str_contain_str(cmd, "$DJR!"), pos)
    {
        handled = 1U;
        all_uart_send_str("#255P1500T2000!");
        debug_stop_all_motion();
        pwmServo_angle_set(255, 1500, 2000);
        uart_reply_ok("DJR");
    }
    else if (pos = str_contain_str(cmd, "$Car:"), pos)
    {
				uart2_send_str("cmdOk");
        /* 小车运动参数 */
        float a = 0, b = 0, c = 0, d = 0;
        handled = 1U;
                if (parse_float_list4_command(cmd, "$Car:", &a, &b, &c, &d))
        {
            debug_set_manual_speed(a, b, c, d, 1U);
            uart_reply_ok("CAR");
        }
        else
        {
            uart_reply_err("CAR format");
        }
    }
    else if (pos = str_contain_str(cmd, "$ZNXJ!"), pos)
    {
        handled = 1U;
        /* 智能循迹模式 */
        debug_enter_ai_mode(1U, 0U);
        forbid_turn = 0;
        uart_reply_ok("ZNXJ");
    }
    else if (pos = str_contain_str(cmd, "$ZYBZ!"), pos)
    {
        handled = 1U;
        // 自由避障
        debug_enter_ai_mode(2U, 0U);
        // 前进
        motor_speed_set(0.3, 0.3, 0.3, 0.3);
        uart_reply_ok("ZYBZ");
    }
    else if (pos = str_contain_str(cmd, "$DJGS!"), pos)
    {
        handled = 1U;
        // 定距跟随
        debug_enter_ai_mode(3U, 0U);
        uart_reply_ok("DJGS");
    }
    else if (pos = str_contain_str(cmd, "$QJ!"), pos)
    {
        handled = 1U;
        // 前进
        debug_set_manual_speed(0.3f, 0.3f, 0.3f, 0.3f, 1U);
        uart_reply_ok("QJ");
    }
    else if (pos = str_contain_str(cmd, "$HT!"), pos)
    {
        handled = 1U;
        // 后退
        debug_set_manual_speed(-0.3f, -0.3f, -0.3f, -0.3f, 1U);
        uart_reply_ok("HT");
    }
    else if (pos = str_contain_str(cmd, "$ZZ!"), pos)
    {
        handled = 1U;
        // 左转
        debug_set_manual_speed(-0.3f, 0.3f, -0.3f, 0.3f, 1U);
        uart_reply_ok("ZZ");
    }
    else if (pos = str_contain_str(cmd, "$YZ!"), pos)
    {
        handled = 1U;
        // 右转
        debug_set_manual_speed(0.3f, -0.3f, 0.3f, -0.3f, 1U);
        uart_reply_ok("YZ");
    }
    else if (pos = str_contain_str(cmd, "$ZPY!"), pos)
    {
        handled = 1U;
        // 左平移
        debug_set_manual_speed(-0.3f, 0.3f, 0.3f, -0.3f, 1U);
        uart_reply_ok("ZPY");
    }
    else if (pos = str_contain_str(cmd, "$YPY!"), pos)
    {
        handled = 1U;
        // 右平移
        debug_set_manual_speed(0.3f, -0.3f, -0.3f, 0.3f, 1U);
        uart_reply_ok("YPY");
    }
    else if (pos = str_contain_str(cmd, "$TZ!"), pos)
    {
        handled = 1U;
        // 停止
        debug_stop_all_motion();
        uart_reply_ok("TZ");
    }
    else if (pos = str_contain_str(cmd, "$GETA!"), pos)
    {
        handled = 1U;
        uart1_send_str("AAA");
        uart_reply_ok("GETA");
    }
    else if (pos = str_contain_str(cmd, "$SMART_STOP!"), pos)
    {
        handled = 1U;
        mdelay(10);
        parse_action("#255PDST!");
        mdelay(10);
        uart1_send_str("#006P1500T0000!#007P1500T0000!");
        mdelay(10);
        uart1_send_str("@OK!");
        mdelay(10);
        uart_reply_ok("SMART STOP");
    }
    else if (pos = str_contain_str(cmd, "$KMS:"), pos)
    {
        handled = 1U;
        if (parse_kms_command(cmd, &kinematics_x, &kinematics_y, &kinematics_z, &int4))
        {
            // uart1_send_str("Try to find best pos:\r\n");
            if (kinematics_move(kinematics_x, kinematics_y, kinematics_z, int4))
            {
                uart_reply_ok("KMS");
            }
            else
            {
                uart1_send_str("Can't find best pos!!!");
                uart_reply_err("KMS no solution");
            }
        }
        else
        {
            uart_reply_err("KMS format");
        }
    }
    else if (pos = str_contain_str(cmd, "$CarLineWalk!"), pos)
    {
        handled = 1U;
        uart_reply_ok("CarLineWalk");
    }
    else if (pos = str_contain_str(cmd, "$MV"), pos)
    {
        handled = 1U;
        if (sscanf((char *)cmd, "$MV%d!", &mode_num))
        {
            oled_mode = 1;
            OLED_CLS();
            mv_mode = mode_num;

            switch (mv_mode)
            {
            case 1:
                display_chars = CAR_COLOR_TRACE;
                num_chars = 6;
                break;
            case 2:
                display_chars = MAP3;
                num_chars = 5;
                break;
            case 3:
                display_chars = ARM_COLOR_TRACE;
                num_chars = 7;
                break;
            case 4:
                display_chars = AprilTag_Sort;
                num_chars = 7;
                break;
            case 5:
                display_chars = AprilTag_Stack;
                num_chars = 7;
                break;
            case 6:
                display_chars = COLOR_TRACE_GRASP;
                num_chars = 6;
                break;
            case 7:
                display_chars = MAP1;
                num_chars = 5;
                break;
            case 8:
                display_chars = MAP2;
                num_chars = 5;
                break;
            case 9:
                display_chars = FACE_TRACE;
                num_chars = 4;
                break;
            case 10:
                display_chars = PTZ_COLOR_TRACE;
                num_chars = 6;
                break;
            default:
                oled_mode = 0;
                display_chars = NULL;
                num_chars = 0;
                break;
            }

            if (display_chars != NULL)
            {
                for (int i = 0, x = 8; i < num_chars; i++, x += 16)
                {
                    OLED_P16x16Ch(x, 3, i, display_chars);
                }
            }
            uart_reply_ok("MV");
        }
        else
        {
            uart_reply_err("MV format");
        }
    }
    else if (pos = str_contain_str(cmd, "$RunStop!"), pos)
    {
        handled = 1U;
        oled_mode = 0;
        uart_reply_ok("RunStop");
    }

    if (!handled)
    {
        uart_reply_err("unknown command");
    }
}

/*
 * 函数名称: getMaxTime
 * 功能描述: 获取动作组字符串中所有T命令的最大时间值
 *           遍历字符串找到所有Txxxx，取最大值
 * 参数说明: str - 动作组字符串指针
 * 返回值:   最大时间值(ms)，用于计算动作组总执行时间
 * 使用说明: 在执行动作组前调用，用于计算等待时间
 */
int getMaxTime(char *str)
{
    int i = 0, max_time = 0, tmp_time = 0;
    while (str[i])
    {
        if (str[i] == 'T')
        {
            tmp_time = (str[i + 1] - '0') * 1000 + (str[i + 2] - '0') * 100 + (str[i + 3] - '0') * 10 + (str[i + 4] - '0');
            if (tmp_time > max_time)
                max_time = tmp_time;
            i = i + 4;
            continue;
        }
        i++;
    }
    return max_time;
}

// 执行动作组1次
// 参数是动作组序号
void do_group_once(int group_num)
{
    delay_ms(10);
    // 将uart_receive_buf清零
    memset(cmd_return, 0, sizeof(cmd_return));
    // 从存储芯片中读取第group_num个动作组
    w25x_read((u8 *)cmd_return, group_num * ACTION_SIZE, ACTION_SIZE);
    // 获取最大的组时间
    action_time = getMaxTime(cmd_return);

    // 把读取出来的动作组传递到parse_action执行
    parse_action(cmd_return);
}

// 动作组批量执行
void app_action_run(void)
{
    // 通过判断舵机是否全部执行完毕 并且是执行动作组group_do_ok尚未结束的情况下进入处理
    static long long systick_ms_bak = 0;
    if (group_do_ok == 0)
    {
        if (millis() - systick_ms_bak > action_time)
        {
            systick_ms_bak = millis();
            if (group_num_cnt != 0 && do_group_cnt == 0)
            {
                group_do_ok = 1;
                uart1_send_str("@GroupDone!");
                return;
            }
            // 调用do_start_index个动作
            do_group_once(do_start_index);

            if (group_num_start < group_num_end)
            {
                if (do_start_index == group_num_end)
                {
                    do_start_index = group_num_start;
                    if (group_num_cnt != 0)
                    {
                        do_group_cnt--;
                    }
                    return;
                }
                do_start_index++;
            }
            else
            {
                if (do_start_index == group_num_end)
                {
                    do_start_index = group_num_start;
                    if (group_num_cnt != 0)
                    {
                        do_group_cnt--;
                    }
                    return;
                }
                do_start_index--;
            }
        }
    }
}

int kinematics_move(float x, float y, float z, int time)
{
    int i, min = 0, flag = 0;

    if (y < 0)
        return 0;

    // 寻找最佳角度
    flag = 0;
    for (i = 0; i >= -135; i--)
    {
        if (0 == kinematics_analysis(x, y, z, i, &kinematics))
        {

            if (i < min)
                min = i;
            flag = 1;
        }
    }

    // 用3号舵机与水平最大的夹角作为最佳值
    if (flag)
    {
        kinematics_analysis(x, y, z, min, &kinematics);
        sprintf((char *)cmd_return, "{#000P%04dT%04d!#001P%04dT%04d!#002P%04dT%04d!#003P%04dT%04d!}", kinematics.servo_pwm[0], time,
                kinematics.servo_pwm[1], time,
                kinematics.servo_pwm[2], time,
                kinematics.servo_pwm[3], time);
        parse_action(cmd_return);
        return 1;
    }

    return 0;
}

void set_servo(int index, int pwm, int time)
{
    sprintf((char *)cmd_return, "#%03dP%04dT%04d!", index, pwm, time);
    parse_action(cmd_return);
}

// 把eeprom_info写入到W25Q64_INFO_ADDR_SAVE_STR位置
void rewrite_eeprom(void)
{
    w25x_erase_sector(W25Q64_INFO_ADDR_SAVE_STR / 4096);
    w25x_writeS((u8 *)(&eeprom_info), W25Q64_INFO_ADDR_SAVE_STR, sizeof(eeprom_info_t));
}
