#ifndef _Y_ADC_H_
#define _Y_ADC_H_
#include "user_main.h"

// ADC1转换的电压值通过MDA方式传到SRAM
extern __IO uint16_t ADC_ConvertedValue;

/*******ADC相关函数声明*******/
void ADC_init(void); // 初始化ADC信号灯

#endif
