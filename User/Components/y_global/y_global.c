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
    float kinematics_x = 0, kinematics_y = 0, kinematics_z = 0;
    int mode_num;
    const unsigned char *display_chars;
    int num_chars;

    uart1_send_str(cmd);
    if (pos = str_contain_str(cmd, "$DRS!"), pos)
    {
        uart1_send_str("hello word!");
    }
    else if (pos = str_contain_str(cmd, "$DST!"), pos)
    {
        group_do_ok = 1;
        pwmServo_stop_motion(255);
        all_uart_send_str("#255PDST!");
        motor_speed_set(0, 0, 0, 0);
        AI_mode = 255;
    }
    else if (pos = str_contain_str(cmd, "$DST:"), pos)
    {
        if (sscanf((char *)cmd, "$DST:%d!", &index))
        {
            pwmServo_stop_motion(index);
            sprintf((char *)cmd_return, "#%03dPDST!\r\n", (int)index);
            all_uart_send_str(cmd_return);
            memset(cmd_return, 0, sizeof(cmd_return));
        }
    }
    else if (pos = str_contain_str(cmd, "$RST!"), pos)
    {
        soft_reset();
    }
    else if (pos = str_contain_str(cmd, "$DGS:"), pos)
    {
        if (sscanf((char *)cmd, "$DGS:%d!", &int1))
        {
            group_do_ok = 1;
            do_group_once(int1);
        }
    }
    else if (pos = str_contain_str(cmd, "$DGT:"), pos)
    {
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
        }
    }
    else if (pos = str_contain_str(cmd, "$DJR!"), pos)
    {
        all_uart_send_str("#255P1500T2000!");
        motor_speed_set(0, 0, 0, 0);
        pwmServo_angle_set(255, 1500, 2000);
        AI_mode = 255;
    }
    else if (pos = str_contain_str(cmd, "$Car:"), pos)
    {
				uart2_send_str("cmdOk");
        /* 小车运动参数 */
        float a = 0, b = 0, c = 0, d = 0;
        if (sscanf((char *)cmd, "$Car:%f,%f,%f,%f!", &a, &b, &c, &d))
        {
            motor_speed_set(a, b, c, d);
        }
    }
    else if (pos = str_contain_str(cmd, "$ZNXJ!"), pos)
    {
        /* 智能循迹模式 */
        AI_mode = 1;
        forbid_turn = 0;
    }
    else if (pos = str_contain_str(cmd, "$ZYBZ!"), pos)
    {
        // 自由避障
        AI_mode = 2;
        // 前进
        motor_speed_set(0.3, 0.3, 0.3, 0.3);
    }
    else if (pos = str_contain_str(cmd, "$DJGS!"), pos)
    {
        // 定距跟随
        AI_mode = 3;
    }
    else if (pos = str_contain_str(cmd, "$QJ!"), pos)
    {
        // 前进
        motor_speed_set(0.3, 0.3, 0.3, 0.3);
    }
    else if (pos = str_contain_str(cmd, "$HT!"), pos)
    {
        // 后退
        motor_speed_set(-0.3, -0.3, -0.3, -0.3);
    }
    else if (pos = str_contain_str(cmd, "$ZZ!"), pos)
    {
        // 左转
        motor_speed_set(-0.3, 0.3, -0.3, 0.3);
    }
    else if (pos = str_contain_str(cmd, "$YZ!"), pos)
    {
        // 右转
        motor_speed_set(0.3, -0.3, 0.3, -0.3);
    }
    else if (pos = str_contain_str(cmd, "$ZPY!"), pos)
    {
        // 左平移
        motor_speed_set(-0.3, 0.3, 0.3, -0.3);
    }
    else if (pos = str_contain_str(cmd, "$YPY!"), pos)
    {
        // 右平移
        motor_speed_set(0.3, -0.3, -0.3, 0.3);
    }
    else if (pos = str_contain_str(cmd, "$TZ!"), pos)
    {
        // 停止
        motor_speed_set(0, 0, 0, 0);
        AI_mode = 255;
    }
    else if (pos = str_contain_str(cmd, "$GETA!"), pos)
    {
        uart1_send_str("AAA");
    }
    else if (pos = str_contain_str(cmd, "$SMART_STOP!"), pos)
    {
        mdelay(10);
        parse_action("#255PDST!");
        mdelay(10);
        uart1_send_str("#006P1500T0000!#007P1500T0000!");
        mdelay(10);
        uart1_send_str("@OK!");
        mdelay(10);
    }
    else if (pos = str_contain_str(cmd, "$KMS:"), pos)
    {
        if (sscanf((char *)cmd, "$KMS:%f,%f,%f,%d!", &kinematics_x, &kinematics_y, &kinematics_z, &int4))
        {
            // uart1_send_str("Try to find best pos:\r\n");
            if (kinematics_move(kinematics_x, kinematics_y, kinematics_z, int4))
            {
            }
            else
            {
                uart1_send_str("Can't find best pos!!!");
            }
        }
    }
    else if (pos = str_contain_str(cmd, "$CarLineWalk!"), pos)
    {
    }
    else if (pos = str_contain_str(cmd, "$MV"), pos)
    {
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
        }
    }
    else if (pos = str_contain_str(cmd, "$RunStop!"), pos)
    {
        oled_mode = 0;
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
