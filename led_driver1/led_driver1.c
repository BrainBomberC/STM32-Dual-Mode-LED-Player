    #include "led_driver1.h"
    #include "tim.h"  // 包含 TIM3 定义
    #include "dma.h"
    #include <math.h>

    // *** 关键区别 ***：使用 uint32_t 适配 TIM3
    uint32_t led_pwm_buffer_ch5[SM_TOTAL_BITS];
    uint32_t led_pwm_buffer_ch6[SM_TOTAL_BITS];

    volatile uint8_t dma_finished_5 = 1;
    volatile uint8_t dma_finished_6 = 1;

    /**
    * @brief  初始化LED PWM缓冲区
    * @note   所有PWM缓冲区初始化为SM_DATA_RESET
    * @example LED1_Init();
    */
    void LED1_Init(void)
    {
        for (uint32_t i = 0; i < SM_TOTAL_BITS; i++) 
        {
            led_pwm_buffer_ch5[i] = SM_DATA_RESET;
            led_pwm_buffer_ch6[i] = SM_DATA_RESET;
        }
    }

    /**
    * @brief 核心打包函数
    *        SM15155E 顺序: R -> G -> B -> W -> Y (高位在前)
    *        每个颜色 16 bit
    */
    /**
    * @brief  打包单颗LED芯片的RGBWY数据到PWM缓冲区
    * @param  buffer: PWM数据缓冲区指针
    * @param  chip_index: 芯片序号（第几颗灯）
    * @param  r: 红色通道（必须为整数，0-65535）
    * @param  g: 绿色通道（必须为整数，0-65535）
    * @param  b: 蓝色通道（必须为整数，0-65535）
    * @param  w: 白色通道（必须为整数，0-65535）
    * @param  y: 黄色通道（必须为整数，0-65535）
    * @note   所有颜色参数必须为整数，不能为小数或浮点数
    * @example LED1_PackChip(buffer, 0, 65535, 0, 0, 0, 0);
    */
    void LED1_PackChip(uint32_t *buffer, uint16_t chip_index,
                    uint16_t r, uint16_t g, uint16_t b, uint16_t w, uint16_t y)
    {
        // 计算起始位置：每颗灯 80 bits
        uint32_t start = chip_index * SM_BITS_PER_CHIP;
        uint32_t pos = start;
        
        // 颜色数组，顺序必须严格遵守手册：OUTR, OUTG, OUTB, OUTW, OUTY
        uint16_t colors[5] = {(uint16_t)r, (uint16_t)g, (uint16_t)b, (uint16_t)w, (uint16_t)y};

        for (int c = 0; c < 5; c++) // 5个通道
        {
            uint16_t val = colors[c];
            for (int bit = 15; bit >= 0; bit--) // 16位精度，MSB first
            {
                if (val & (1 << bit))
                    buffer[pos++] = SM_DATA_ONE;
                else
                    buffer[pos++] = SM_DATA_ZERO;
            }
        }
    }

    /**
    * @brief 尾帧打包函数 (32 bits)
    *        结构: R_Gain(5) -> G_Gain(5) -> B_Gain(5) -> W_Gain(5) -> Y_Gain(5) -> Sleep(2) -> Reserved(5)
    *        共 32 位
    */
    /**
    * @brief  尾帧打包函数 (32 bits)
    * @param  buffer: PWM数据缓冲区指针
    * @param  actual_cnt: 实际灯珠数量
    * @param  r_gain: 红色增益（必须为整数，0-31，数值越大电流越大）
    * @param  g_gain: 绿色增益（必须为整数，0-31，数值越大电流越大）
    * @param  b_gain: 蓝色增益（必须为整数，0-31，数值越大电流越大）
    * @param  w_gain: 白色增益（必须为整数，0-31，数值越大电流越大）
    * @param  y_gain: 黄色增益（必须为整数，0-31，数值越大电流越大）
    * @note   所有增益参数必须为整数，不能为小数或浮点数。
    *         增益值越大，LED驱动电流越大，亮度越高，功耗也越大。建议根据实际需求调整，非必要不建议全31。
    *         结构: R_Gain(5) -> G_Gain(5) -> B_Gain(5) -> W_Gain(5) -> Y_Gain(5) -> Sleep(2) -> Reserved(5)
    * @example LED1_PackTail(buffer, 10, 20, 15, 5, 25);
    */
    void LED1_PackTail(uint32_t *buffer, uint16_t actual_cnt, uint8_t r_gain, uint8_t g_gain, uint8_t b_gain, uint8_t w_gain, uint8_t y_gain)
    {
        // 尾帧位于所有灯珠数据之后
        uint32_t start = actual_cnt  * SM_BITS_PER_CHIP;
        uint32_t pos = start;

        // 组装 32位 整数
        // 建议 Sleep = 00 (正常工作), Reserved = 11111 (全1)
        // 增益限制在 5 bit (0-31)
        uint32_t tail_data = 0;
        
        tail_data |= ((uint32_t)(r_gain & 0x1F) << 27); // Bit 31-27
        tail_data |= ((uint32_t)(g_gain & 0x1F) << 22); // Bit 26-22
        tail_data |= ((uint32_t)(b_gain & 0x1F) << 17); // Bit 21-17
        tail_data |= ((uint32_t)(w_gain & 0x1F) << 12); // Bit 16-12
        tail_data |= ((uint32_t)(y_gain & 0x1F) << 7);  // Bit 11-7
        
        tail_data |= (0x00 << 5); // Bit 6-5: Sleep Mode (10=Sleep, 00=Normal)
        tail_data |= (0x1F << 0); // Bit 4-0: Reserved (建议全1)

        // 将这 32 位转换成 PWM 波形
        for (int bit = 31; bit >= 0; bit--)
        {
            if (tail_data & (1U << bit))
                buffer[pos++] = SM_DATA_ONE;
            else
                buffer[pos++] = SM_DATA_ZERO;
        }

        // [新增] 尾帧后面补 Reset 信号 (确保后面没有脏数据)
        for (int i = 0; i < SM_RESET_BITS; i++) {
            buffer[pos++] = SM_DATA_RESET;
        }

        // 尾帧之后紧接着是 Reset 信号，Init时已经置0，无需再次填充
    }

    /**
    * @brief  通过DMA发送两路LED PWM数据帧（CH5/CH6）
    * @param  buf5: 通道5的PWM数据缓冲区指针
    * @param  buf6: 通道6的PWM数据缓冲区指针
    * @param  cnt5: 通道5的实际灯珠数量
    * @param  cnt6: 通道6的实际灯珠数量
    * @note   需确保缓冲区已正确打包，且所有颜色参数必须为整数
    * @note   会检测每一个通道的灯珠数量，如果灯珠数量是0，那么这个通道的DMA不会启动
    * @example LED1_SendFrame_All(led_pwm_buffer_ch5, led_pwm_buffer_ch6);
    */
    void LED1_SendFrame_All(uint32_t *buf5, uint32_t *buf6, uint16_t count5, uint16_t count6)
    {
        // 1. 等待上一轮完成
        while (!dma_finished_5 || !dma_finished_6);

        // 2. 逐个判断并启动通道

        // --- 通道 5 (TIM3_CH2) ---
        if (count5 > 0) {
            dma_finished_5 = 0; // 标记为忙
            // 计算长度：灯珠数据 + 尾帧 + 复位
            uint32_t len = (count5 * SM_BITS_PER_CHIP) + SM_TAIL_BITS + SM_RESET_BITS;
            HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_2, (uint32_t*)buf5, len);
        } else {
            // 没数据，不启动
            dma_finished_5 = 1;
        }

        // --- 通道 6 (TIM3_CH3) ---
        if (count6 > 0) {
            dma_finished_6 = 0; // 标记为忙
            uint32_t len = (count6 * SM_BITS_PER_CHIP) + SM_TAIL_BITS + SM_RESET_BITS;
            HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_3, (uint32_t*)buf6, len);
        } else {
            // 没数据，不启动
            dma_finished_6 = 1;
        }
    }

