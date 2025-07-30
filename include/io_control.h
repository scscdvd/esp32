#ifndef _IO_CONTROL_H
#define _IO_CONTROL_H
#include "main.h"


#define GPIO_OUTPUT_PIN  ((1ULL<<GPIO_NUM_14) | (1ULL<<GPIO_NUM_13)) // 定义GPIO输出引脚
#define GPIO_INPUT_PIN   ((1ULL<<GPIO_NUM_12) | (1ULL<<GPIO_NUM_15)) // 定义GPIO输入引脚

void io_control_init(void); // 初始化IO控制


#endif // _IO_CONTROL_H
