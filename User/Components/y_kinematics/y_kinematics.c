/*
 * ================================================================================
 * @文件名称: y_kinematics.c
 * @功能描述: 4自由度机械臂运动学逆解算法
 *           根据末端坐标(x,y,z)和爪子角度Alpha，计算4个舵机的角度和PWM值
 * @所属模块: Components/y_kinematics
 * @依赖: y_kinematics.h, math.h
 * @算法说明: 使用几何法求解4自由度串联机械臂逆运动学
 *   L0: 底座高度, L1: 大臂长度, L2: 小臂长度, L3: 爪子长度
 *   theta6: 底盘旋转角度（XY平面投影角）
 *   theta5: 大臂与水平面夹角
 *   theta4: 小臂与大臂夹角
 *   theta3: 爪子与水平面夹角
 * ================================================================================
 */

#include "y_kinematics/y_kinematics.h"
#include <math.h>

/** 圆周率 */
#define pi 3.1415926

/*
 * 全局变量: kinematics
 * 功能描述: 机械臂运动学参数结构体
 *   包含关节长度(L0~L3)、舵机角度(servo_angle[4])、舵机PWM(servo_pwm[4])
 */
kinematics_t kinematics;

/*
 * 函数名称: setup_kinematics
 * 功能描述: 设置机械臂四个关节的长度参数（内部放大10倍以提高精度）
 * 参数说明: L0 - 底座到肩关节的高度 (单位: mm)
 *          L1 - 大臂长度 (单位: mm)
 *          L2 - 小臂长度 (单位: mm)
 *          L3 - 爪子长度 (单位: mm)
 *          kinematics - 运动学参数结构体指针
 * 返回值:   无
 * 使用说明: 在机械臂初始化时调用一次
 *           示例: setup_kinematics(80, 120, 100, 50, &kinematics);
 */
void setup_kinematics(float L0, float L1, float L2, float L3, kinematics_t *kinematics) {
	//放大10倍
	kinematics->L0 = L0*10;
	kinematics->L1 = L1*10;
	kinematics->L2 = L2*10;
	kinematics->L3 = L3*10;
}

/*
 * 函数名称: kinematics_analysis
 * 功能描述: 机械臂运动学逆解，根据末端坐标计算4个关节角度和PWM值
 *           坐标原点在底座中心，X轴向右，Y轴向前，Z轴向上
 * 参数说明: x - 末端X坐标 (单位: mm)
 *          y - 末端Y坐标 (单位: mm)
 *          z - 末端Z坐标 (单位: mm) 
 *          Alpha - 爪子与水平面的夹角 (单位: 度，推荐范围 -25° ~ -65°)
 *          kinematics - 运动学参数结构体指针（输入关节长度，输出角度和PWM）
 * 返回值:   0 - 求解成功
 *           1 - z坐标超出范围（低于底座）
 *           2 - 距离超出机械臂最大伸展范围
 *           3 - acos参数超出范围
 *           4 - theta4超出范围
 *           5 - acos参数超出范围
 *           6 - theta5超出范围
 *           7 - theta3超出范围
 * 使用说明: 调用后，kinematics->servo_angle[0~3]存储4个舵机角度
 *           kinematics->servo_pwm[0~3]存储4个舵机PWM值(500~2500)
 *           角度到PWM的转换: 1500 ± 2000 * angle / 270 (中值1500对应0°)
 *           示例: kinematics_analysis(100, 150, 50, -45, &kinematics);
 */
int kinematics_analysis(float x, float y, float z, float Alpha, kinematics_t *kinematics) {
	float theta3, theta4, theta5, theta6;
	float l0, l1, l2, l3;
	float aaa, bbb, ccc, zf_flag;
	
	//放大10倍
	x = x*10;
	y = y*10;
	z = z*10;
	
	
	l0 = kinematics->L0;
	l1 = kinematics->L1;
	l2 = kinematics->L2;
	l3 = kinematics->L3;
	
	if(x == 0) {
		theta6 = 0.0;
	} else {
		theta6 = atan(x/y)*270.0/pi;
	}
	
	y = sqrt(x*x + y*y);    
    y = y-l3 * cos(Alpha*pi/180.0);  
    z = z-l0-l3*sin(Alpha*pi/180.0); 
    if(z < -l0) {
        return 1;
	}
    if(sqrt(y*y + z*z) > (l1+l2)) {
        return 2;
	}
	
	ccc = acos(y / sqrt(y * y + z * z));
    bbb = (y*y+z*z+l1*l1-l2*l2)/(2*l1*sqrt(y*y+z*z));
    if(bbb > 1 || bbb < -1) {
        return 5;
	}
    if (z < 0) {
        zf_flag = -1;
	} else {
        zf_flag = 1;
	}
    theta5 = ccc * zf_flag + acos(bbb);
    theta5 = theta5 * 180.0 / pi;
    if(theta5 > 180.0 || theta5 < 0.0) {
        return 6;
	}
	
    aaa = -(y*y+z*z-l1*l1-l2*l2)/(2*l1*l2);
    if (aaa > 1 || aaa < -1) {
        return 3;
	}
    theta4 = acos(aaa); 
    theta4 = 180.0 - theta4 * 180.0 / pi ;  
    if (theta4 > 135.0 || theta4 < -135.0) {
        return 4;
	}

    theta3 = Alpha - theta5 + theta4;
    if(theta3 > 90.0 || theta3 < -90.0) {
        return 7;
	}
	
	kinematics->servo_angle[0] = theta6;
	kinematics->servo_angle[1] = theta5-90;
	kinematics->servo_angle[2] = theta4;
	kinematics->servo_angle[3] = theta3;    
	
	kinematics->servo_pwm[0] = (int)(1500-2000.0 * kinematics->servo_angle[0] / 270.0);
	kinematics->servo_pwm[1] = (int)(1500+2000.0 * kinematics->servo_angle[1] / 270.0);
	kinematics->servo_pwm[2] = (int)(1500+2000.0 * kinematics->servo_angle[2] / 270.0);
	kinematics->servo_pwm[3] = (int)(1500+2000.0 * kinematics->servo_angle[3] / 270.0);

	return 0;
}

