#ifndef __LED_DRIVER1_H__
#define __LED_DRIVER1_H__

#include "main.h"

// ==========================================
//               SM15155E 芯片配置
// ==========================================

// SM15155E 是 5通道 (R,G,B,W,Y) * 16位精度 = 80 bits/颗
#define SM_NUM_CHIPS       200       // 最大接入灯珠数量
#define SM_BITS_PER_CHIP   80        // 16 bits * 5 channels
#define SM_TAIL_BITS       32        // 级联尾部的电流增益配置位
#define SM_RESET_BITS      200       // >200us 复位信号

// 总缓冲长度
#define SM_TOTAL_BITS (SM_NUM_CHIPS * SM_BITS_PER_CHIP + SM_TAIL_BITS + SM_RESET_BITS)

// ==========================================
//           PWM 占空比定义 (TIM3)
// ==========================================
// 注意：TIM3 是 16位定时器，ARR=119
// 0码: 300ns -> CCR ≈ 29
// 1码: 900ns -> CCR ≈ 86
#define SM_DATA_ONE    86    
#define SM_DATA_ZERO   29    
#define SM_DATA_RESET  0     

// ==========================================
//              变量与缓冲区
// ==========================================

// *** 关键区别 ***：TIM3 是 32位定时器，必须使用 uint32_t
extern uint32_t led_pwm_buffer_ch5[SM_TOTAL_BITS]; // 对应 TIM3_CH2
extern uint32_t led_pwm_buffer_ch6[SM_TOTAL_BITS]; // 对应 TIM3_CH3

extern volatile uint8_t dma_finished_5;
extern volatile uint8_t dma_finished_6;

// ==========================================
//              函数声明
// ==========================================

void LED1_Init(void);

/**
 * @brief 填充单颗 SM15155E 灯珠数据 (16位灰度, 5通道)
 * @param buffer 目标缓冲区
 * @param chip_index 灯珠索引
 * @param r Red (0-65535)
 * @param g Green (0-65535)
 * @param b Blue (0-65535)
 * @param w White (0-65535)
 * @param y Yellow (0-65535)
 */
void LED1_PackChip(uint32_t *buffer, uint16_t chip_index, uint16_t r, uint16_t g, uint16_t b, uint16_t w, uint16_t y);

/**
 * @brief 填充帧尾的电流增益和配置数据 (所有灯珠数据之后发送一次)
 * @param buffer 目标缓冲区
 * @param actual_cnt 实际灯珠数量
 * @param r_gain 红色电流增益 (0-31)
 * @param g_gain 绿色电流增益 (0-31)
 * @param b_gain 蓝色电流增益 (0-31)
 * @param w_gain 白色电流增益 (0-31)
 * @param y_gain 黄色电流增益 (0-31)
 */
void LED1_PackTail(uint32_t *buffer, uint16_t actual_cnt, uint8_t r_gain, uint8_t g_gain, uint8_t b_gain, uint8_t w_gain, uint8_t y_gain);

/**
 * @brief 发送 TIM3 的两路数据
 */
void LED1_SendFrame_All(uint32_t *buf5, uint32_t *buf6, uint16_t count5, uint16_t count6);



#endif