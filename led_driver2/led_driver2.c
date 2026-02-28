#include "led_driver2.h"
#include "tim.h"  // 确保这里包含的是 TIM5 的定义
#include "dma.h"
#include "math.h"

// 定义 32 位缓冲区，适配 STM32F4 TIM5 的 32 位 CCR 寄存器
uint32_t led_pwm_buffer_ch1[LED_TOTAL_BITS];
uint32_t led_pwm_buffer_ch2[LED_TOTAL_BITS];
uint32_t led_pwm_buffer_ch3[LED_TOTAL_BITS];
uint32_t led_pwm_buffer_ch4[LED_TOTAL_BITS];

// DMA 完成标志位 (需要在 main.c 的中断回调 HAL_TIM_PWM_PulseFinishedCallback 中维护)
volatile uint8_t dma_finished_1 = 1;
volatile uint8_t dma_finished_2 = 1;
volatile uint8_t dma_finished_3 = 1;
volatile uint8_t dma_finished_4 = 1;

/**
 * @brief 初始化LED PWM缓冲区，将所有数据设置为复位状态(0占空比)
 *        防止上电时引脚输出杂波导致灯珠误亮
 * @param None
 * @return None
 */
void LED2_Init(void)
{
    for (uint16_t i = 0; i < LED_TOTAL_BITS; i++) 
    {
        led_pwm_buffer_ch1[i] = DATA_RESET; // 将通道1缓冲区填充为0
        led_pwm_buffer_ch2[i] = DATA_RESET; // 将通道2缓冲区填充为0
        led_pwm_buffer_ch3[i] = DATA_RESET; // 将通道3缓冲区填充为0
        led_pwm_buffer_ch4[i] = DATA_RESET; // 将通道4缓冲区填充为0
    }
}

/**
 * @brief 将单个灯珠的RGB颜色数据转换为WS2812B协议的PWM占空比数据并填入缓冲区
 * @param buffer      目标通道的PWM缓冲区指针 (uint32_t*)
 * @param chip_index  灯珠在灯带中的索引（第几颗灯，从0开始）
 * @param g           绿色分量 (0-255)
 * @param r           红色分量 (0-255)
 * @param b           蓝色分量 (0-255)
 * @return None
 * @note WS2812B的数据发送顺序是 G -> R -> B，且高位在前 (MSB first)
 */
void LED2_PackChip(uint32_t *buffer, uint16_t chip_index,
                   uint8_t g, uint8_t r, uint8_t b)
{
    uint16_t start = chip_index * LED_BITS_PER_CHIP; // 计算该灯珠数据在缓冲区中的起始位置
    uint8_t values[3] = { g, r, b };                 // WS2812B 协议要求的颜色顺序是 GRB
    uint16_t pos = start;                            // 当前填充的缓冲区下标

    for (int c = 0; c < 3; c++)                      // 遍历 3 个颜色通道 (G, R, B)
    {
        uint8_t val = values[c];                     // 取出当前颜色的 8 位数值
        for (int bit = 7; bit >= 0; bit--)           // 从高位到低位遍历每一位 (MSB first)
        {
            if (val & (1 << bit))                    // 判断当前位是 1 还是 0
                buffer[pos++] = DATA_ONE;            // 如果是 1，填入长高电平的 CCR 值 (例如 61)
            else
                buffer[pos++] = DATA_ZERO;           // 如果是 0，填入短高电平的 CCR 值 (例如 31)
        }
    }
}

/**
 * @brief 在有效颜色数据后附加 Reset 复位信号
 *        WS2812B 需要一段长时间的低电平(>50us或>280us)来锁存数据
 * @param buffer      目标通道的PWM缓冲区指针
 * @param start_index 复位信号在缓冲区中的起始下标 (通常是 灯珠数*24)
 * @return None
 */
void LED2_AppendReset(uint32_t *buffer, uint16_t start_index)
{
    for (int i = 0; i < LED_RESET_BITS; i++)         // 循环填充复位位的数量
        buffer[start_index + i] = DATA_RESET;        // 填入 0 占空比，保持数据线为低电平
}

/**
 * @brief 通过 DMA 同时启动 TIM5 的 4 个通道发送 LED 数据
 *        该函数是阻塞式的（会等待上一轮 DMA 传输完成）
 * @param buf1 通道1的数据缓冲区
 * @param buf2 通道2的数据缓冲区
 * @param buf3 通道3的数据缓冲区
 * @param buf4 通道4的数据缓冲区
 * @param cnt1 通道1的实际灯珠数量
 * @param cnt2 通道2的实际灯珠数量
 * @param cnt3 通道3的实际灯珠数量
 * @param cnt4 通道4的实际灯珠数量
 * @note 会检测识别的每一个通道灯珠的数量，如果灯珠数量是0，那么这个通道的DMA

 */
void LED2_SendFrame_All(uint32_t *buf1, uint32_t *buf2, uint32_t *buf3, uint32_t *buf4, uint16_t cnt1, uint16_t cnt2, uint16_t cnt3, uint16_t cnt4)
{
    // 1. 等待上一轮所有“已启动”的通道完成
    // 如果某通道上一轮没启动，它的标志位一直是 1，不会卡住这里
    while (!dma_finished_1 || !dma_finished_2 || !dma_finished_3 || !dma_finished_4);
    
    // 2. 逐个判断并启动通道

    // --- 通道 1 ---
    if (cnt1 > 0) {
        dma_finished_1 = 0; // 标记为正在忙
        uint32_t len = (cnt1 * LED_BITS_PER_CHIP) + LED_RESET_BITS;
        HAL_TIM_PWM_Start_DMA(&htim5, TIM_CHANNEL_1, buf1, len);
    } else {
        // 如果没有灯，确保标志位是 1 (完成状态)，且不启动 DMA
        // (全局变量初始化是1，正常跑完中断也是1，这里是为了双重保险)
        dma_finished_1 = 1; 
    }

    // --- 通道 2 ---
    if (cnt2 > 0) {
        dma_finished_2 = 0;
        uint32_t len = (cnt2 * LED_BITS_PER_CHIP) + LED_RESET_BITS;
        HAL_TIM_PWM_Start_DMA(&htim5, TIM_CHANNEL_2, buf2, len);
    } else {
        dma_finished_2 = 1;
    }

    // --- 通道 3 ---
    if (cnt3 > 0) {
        dma_finished_3 = 0;
        uint32_t len = (cnt3 * LED_BITS_PER_CHIP) + LED_RESET_BITS;
        HAL_TIM_PWM_Start_DMA(&htim5, TIM_CHANNEL_3, buf3, len);
    } else {
        dma_finished_3 = 1;
    }

    // --- 通道 4 ---
    if (cnt4 > 0) {
        dma_finished_4 = 0;
        uint32_t len = (cnt4 * LED_BITS_PER_CHIP) + LED_RESET_BITS;
        HAL_TIM_PWM_Start_DMA(&htim5, TIM_CHANNEL_4, buf4, len);
    } else {
        dma_finished_4 = 1;
    }

}

