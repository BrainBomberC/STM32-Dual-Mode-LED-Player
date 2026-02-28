#ifndef __LED_DRIVER2_H__
#define __LED_DRIVER2_H__

#include "main.h"

// ==========================================
//         内置ic（24位rgb）芯片参数配置
// ==========================================

/** 
 * @brief 最大灯珠数量 
 * @note  请根据实际灯带长度修改此值
 */
#define LED_NUM_CHIPS      200        

/** 
 * @brief 每颗灯珠的数据位数
 */
#define LED_BITS_PER_CHIP  24     

/** 
 * @brief Reset 复位信号长度 (单位：PWM周期数)
 * @note  WS2812B 需要 >50us 的低电平复位。
 *        1个周期 = 1.25us，400个周期 = 500us，足以保证稳定复位。
 */
#define LED_RESET_BITS     400    

/** 
 * @brief 总数据长度 (所有灯珠数据 + 复位信号)
 */
#define LED_TOTAL_BITS (LED_NUM_CHIPS * LED_BITS_PER_CHIP + LED_RESET_BITS)


// ==========================================
//           PWM 占空比定义
// ==========================================
// 基于 STM32F412 主频 96MHz, TIM5 Prescaler=0, ARR=119
// PWM 频率 = 96MHz / (119+1) = 800kHz (周期 1.25us)

/** 
 * @brief 逻辑 "1" 的 CCR 值 (占空比 ~64%)
 *        目标高电平时间 ~0.8us -> 0.8us / 1.25us * 120 ≈ 76 (实际常用 60-64 兼容性更好)
 *        此处使用 61 (约 0.64us)
 */
#define DATA_ONE   61     

/** 
 * @brief 逻辑 "0" 的 CCR 值 (占空比 ~32%)
 *        目标高电平时间 ~0.4us -> 0.4us / 1.25us * 120 ≈ 38
 *        此处使用 31 (约 0.32us)
 */
#define DATA_ZERO  31     

/** 
 * @brief 复位信号 (占空比 0%，持续低电平)
 */
#define DATA_RESET 0      


// ==========================================
//              变量与缓冲区
// ==========================================

// PWM 数据缓冲区 (适配 TIM5 32位 CCR 寄存器，使用 uint32_t)
extern uint32_t led_pwm_buffer_ch1[LED_TOTAL_BITS];
extern uint32_t led_pwm_buffer_ch2[LED_TOTAL_BITS];
extern uint32_t led_pwm_buffer_ch3[LED_TOTAL_BITS];
extern uint32_t led_pwm_buffer_ch4[LED_TOTAL_BITS];

// DMA 传输完成标志位 (在 main.c 或中断回调中维护)
extern volatile uint8_t dma_finished_1;
extern volatile uint8_t dma_finished_2;
extern volatile uint8_t dma_finished_3;
extern volatile uint8_t dma_finished_4;


// ==========================================
//              函数声明
// ==========================================

/**
 * @brief 初始化 LED 驱动，清空缓冲区
 */
void LED2_Init(void);

/**
 * @brief 将 RGB 颜色转换为 PWM 数据填入缓冲区
 * @param buffer      目标缓冲区指针
 * @param chip_index  灯珠索引 (0 ~ LED_NUM_CHIPS-1)
 * @param g           绿色分量 (0-255)
 * @param r           红色分量 (0-255)
 * @param b           蓝色分量 (0-255)
 */
void LED2_PackChip(uint32_t *buffer, uint16_t chip_index, uint8_t g, uint8_t r, uint8_t b);

/**
 * @brief 在数据尾部附加 Reset 信号
 * @param buffer      目标缓冲区指针
 * @param start_index Reset 信号的起始位置
 */
void LED2_AppendReset(uint32_t *buffer, uint16_t start_index);

/**
 * @brief 四通道同时发送 LED 数据 (DMA 方式)
 * @param buf1 通道1 数据
 * @param buf2 通道2 数据
 * @param buf3 通道3 数据
 * @param buf4 通道4 数据
 * @param cnt1 通道1 实际灯珠数量
 * @param cnt2 通道2 实际灯珠数量
 * @param cnt3 通道3 实际灯珠数量
 * @param cnt4 通道4 实际灯珠数量
 */
void LED2_SendFrame_All(uint32_t *buf1, uint32_t *buf2, uint32_t *buf3, uint32_t *buf4, 
                        uint16_t cnt1, uint16_t cnt2, uint16_t cnt3, uint16_t cnt4);


#endif