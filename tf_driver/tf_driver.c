    #include "tf_driver.h"
    #include <stdio.h>
    #include <string.h>
    #include "fatfs.h"
    #include "led_driver2.h"
    #include "led_driver1.h"
    #include <stdlib.h> // for atoi


    FATFS fs_tf;  // TF卡专用的文件系统对象
    FIL tf_file;  // TF卡专用的文件对象


    volatile uint8_t stop_play_flag = 0; // 播放中止标志位，初始为0



    // 初始化并挂载 TF 卡
    uint8_t TF_Init(void)
    {
        FRESULT res;
        
        // 挂载
        res = f_mount(&fs_tf, TF_PATH, 1);
        
        if (res == FR_OK)
        {
            //printf("[TF-SDIO] Mount Success on %s\r\n", TF_PATH);
            return 0;
        }
        else
        {
        // printf("[TF-SDIO] Mount Failed! Error: %d\r\n", res);
            return 1;
        }
    }

    // 测试写入
    uint8_t TF_Test_Write(void)
    {
        FRESULT res;
        UINT bw;
        char path[32];
        const char *data = "STM32 F412 SDIO Test Data\r\n";

        // 建议规范路径写法，避免 //
        sprintf(path, "%stest111.txt", TF_PATH); 

        // 1. 打开文件
        res = f_open(&tf_file, path, FA_WRITE | FA_CREATE_ALWAYS);
        if (res != FR_OK) {
            printf("[TF Error] f_open failed: res=%d\r\n", res); // 打印错误码
            return 1;
        }

        // 2. 写入数据
        res = f_write(&tf_file, data, strlen(data), &bw);
        if (res != FR_OK) {
            printf("[TF Error] f_write failed: res=%d\r\n", res); // 打印错误码
            f_close(&tf_file);
            return 2;
        }

        // 3. 关闭文件 (至关重要，只有关闭了数据才会真正刷入)
        res = f_close(&tf_file);
        if (res != FR_OK) {
            printf("[TF Error] f_close failed: res=%d\r\n", res); // 打印错误码
            return 3;
        }

        // 4. 校验写入长度
        if (bw == strlen(data))
        {
            printf("[TF-SDIO] Write OK. Bytes written: %d\r\n", bw);
            return 0;
        }
        else
        {
            printf("[TF Error] Incomplete write: bw=%d (expected %d)\r\n", bw, strlen(data));
            return 4;
        }
    }

    // 测试读取
    uint8_t TF_Test_Read(void)
    {
        FRESULT res;
        UINT br;
        char path[32];
        char buffer[64] = {0};

        sprintf(path, "%stest.txt", TF_PATH);

        res = f_open(&tf_file, path, FA_READ);
        if (res != FR_OK) return 1;

        res = f_read(&tf_file, buffer, sizeof(buffer)-1, &br);
        f_close(&tf_file);

        if (res == FR_OK)
        {
            printf("[TF-SDIO] Read Content: %s\r\n", buffer);
            return 0;
        }
        return 2;
    }

    // 将温度数组保存为 CSV 文件 (SDIO TF卡)
    /** uint8_t TF_SaveFrameCSV(const char* filename, float* frameData, int len)
    * @brief 针对MLX90640型号的红外相机数据，将其温度数据以csv的形式写入到tf卡中
    * @param filename       文件名称
    * @param frameData      具体温度数据
    * @param len            温度数据长度
    * @return 0或1          0表示正常，1表示异常
    */
    uint8_t TF_SaveFrameCSV(const char* filename, float* frameData, int len)
    {
        FIL fil;
        FRESULT res;
        char path[64];
        char line_buf[16];

        sprintf(path, "0:/%s", filename);

        res = f_open(&fil, path, FA_WRITE | FA_OPEN_APPEND);
        if (res != FR_OK) return 1;

        for (int i = 0; i < len; i++)                 //循环遍历数组 frameData，长度是 len
        {
            sprintf(line_buf, "%.2f,", frameData[i]);   //把 frameData[i]（一个浮点数温度值）格式化成字符串，保留两位小数:例如 25.6789 → "25.68,"
                                                    //存到 line_buf里,sprintf 就是“把数字转成字符串”的工具
            f_puts(line_buf, &fil);                   //把刚刚生成的字符串写入到 SD 卡文件 fil 中
            if ((i + 1) % 32 == 0) f_putc('\n', &fil);//每写满 32 个数据，就插入一个换行符 \n;这样文件里的数据会按 32 个一行 排版;因为 MLX90640 是 32 列 × 24 行，所以这里正好对应一行像素
        }
        f_putc('\n', &fil);                           //循环结束后，再写一个换行符;保证最后一行结束时也有换行，不会和后续数据粘在一起

        f_close(&fil);
        return 0;
    }




// 辅助函数：获取CSV行中第N个逗号后的整数值 (从0开始计数，第1列是0)
// 例如 "gain of R,2,,," -> get_csv_int(buf, 1) 返回 2
static int get_csv_int(char *line, int column_index)
{
    char *ptr = line;
    int current_col = 0;
    
    // 如果是要找第0列，直接解析
    if (column_index == 0) return atoi(ptr);

    // 寻找第N个逗号
    while (*ptr)
    {
        if (*ptr == ',')
        {
            current_col++;
            if (current_col == column_index)
            {
                return atoi(ptr + 1); // 返回逗号后的数字
            }
        }
        ptr++;
    }
    return 0; // 没找到
}

/** 
 * @brief  全能播放函数：支持 WS2812 (c1-c4) 和 SM15155E (c5-c6)
 */
void TF_PlayCSV_Multi(uint16_t default_delay_ms)
{
    // 定义文件对象：一共6个
    FIL file[6];                
    uint8_t file_open[6] = {0}; 
    char line_buffer[128];      
    char path[64];
    
    // 动态配置参数
    uint16_t current_delay = default_delay_ms;
    uint8_t delay_set = 0; // 【关键】标记延时是否已由 CSV 文件设定

    // 存储每个通道的灯珠数量 ,默认全是0
    // 只有当成功读取 CSV 且解析出数量后，才更新为实际值。
    // 如果没有文件，它保持 0，DMA 就几乎不占用时间。
    uint16_t channel_led_num[6] = {0,0,0,0,0,0};
    
    // SM15155E 专用：存储解析出来的增益值 [通道索引0-1][RGBWY]
    // 对应 c5(idx 0) 和 c6(idx 1)
    uint8_t sm_gains[2][5]; 
    // 默认增益初始化
    memset(sm_gains, 16, sizeof(sm_gains)); 

    const char* filenames[6] = {"c1.csv", "c2.csv", "c3.csv", "c4.csv", "c5.csv", "c6.csv"};
    
    // WS2812 的缓冲区指针数组 (Ch1-Ch4)
    uint32_t* buffers_ws[4] = {led_pwm_buffer_ch1, led_pwm_buffer_ch2, led_pwm_buffer_ch3, led_pwm_buffer_ch4};
    // SM15155 的缓冲区指针数组 (Ch5-Ch6)
    uint32_t* buffers_sm[2] = {led_pwm_buffer_ch5, led_pwm_buffer_ch6};

    // 数据起始偏移量
    DWORD data_start_offset[6] = {0};
    uint8_t any_file_open = 0;

    printf("[TF] Start Hybrid Playing (6 Channels)...\r\n");

    // ==========================================
    // 1. 打开文件 & 解析文件头
    // ==========================================
    for (int i = 0; i < 6; i++)
    {
        sprintf(path, "0:/%s", filenames[i]);
        if (f_open(&file[i], path, FA_READ) == FR_OK)
        {
            file_open[i] = 1;
            any_file_open = 1;
            printf("[TF] CH%d loaded: %s\r\n", i + 1, filenames[i]);

            // -------------------------------------------------
            // 策略 A: 处理 WS2812 (c1.csv - c4.csv)
            // 格式：4行头，3列数据
            // -------------------------------------------------
            if (i < 4) 
            {
                // Line 1: 描述 (跳过)
                f_gets(line_buffer, sizeof(line_buffer), &file[i]);
                
                // Line 2: Num of Leds (格式: "Num,10,") -> 取第1列
                if (f_gets(line_buffer, sizeof(line_buffer), &file[i])) {
                    int count = get_csv_int(line_buffer, 1); // 取逗号后的值
                    if (count == 0) count = atoi(line_buffer); // 容错：如果在第0列
                    
                    if (count > 0 && count <= LED_NUM_CHIPS) {
                        channel_led_num[i] = count;
                    }
                }

                // Line 3: Delay (只从 c1 读取)
                if (f_gets(line_buffer, sizeof(line_buffer), &file[i])) {
                    if (!delay_set) { // 【修正点】如果之前没设过延时，则读取
                        int d = get_csv_int(line_buffer, 1);
                        if (d == 0) d = atoi(line_buffer);
                        if (d > 0) {
                            current_delay = d;
                            delay_set = 1; // 标记已设置
                            printf("    -> Delay (from %s): %d ms\r\n", filenames[i], current_delay);
                        }
                    }
                }

                // Line 4: Hints (跳过)
                f_gets(line_buffer, sizeof(line_buffer), &file[i]);
            }
            // -------------------------------------------------
            // 策略 B: 处理 SM15155E (c5.csv - c6.csv)
            // 格式：8行头，5列数据，含增益
            // -------------------------------------------------
            else 
            {
                int sm_idx = i - 4; // 0 或 1

                // Line 1: Description (跳过)
                f_gets(line_buffer, sizeof(line_buffer), &file[i]);

                // Line 2: Num of ICs (格式: "Num of ICs...,3,,,") -> 取第1列
                if (f_gets(line_buffer, sizeof(line_buffer), &file[i])) {
                    int count = get_csv_int(line_buffer, 1);
                    if (count > 0 && count <= SM_NUM_CHIPS) {
                        channel_led_num[i] = count;
                        printf("    -> SM ICs: %d\r\n", count);
                    }
                }


                 // 【新增/修改】为了兼容延时，建议 c5/c6 的第 3 行也设为 Delay
                // 如果你的 c5.csv 格式已经固定且没有 Delay 行，此处需根据实际调整
                if (f_gets(line_buffer, sizeof(line_buffer), &file[i])) {
                    if (!delay_set) {
                        int d = get_csv_int(line_buffer, 1);
                        if (d > 0) { // 简单判断是否是数字
                            current_delay = d;
                            delay_set = 1;
                            printf("    -> Delay (from %s): %d ms\r\n", filenames[i], current_delay);
                        }
                    }
                }

                // Line 4-8: Gains (R, G, B, W, Y)
                // 格式: "gain of R,2,,," -> 取第1列
                for (int color = 0; color < 5; color++) {
                    if (f_gets(line_buffer, sizeof(line_buffer), &file[i])) {
                        int g_val = get_csv_int(line_buffer, 1);
                        // 限制增益范围 0-31
                        if (g_val > 31) g_val = 31;
                        if (g_val < 0) g_val = 0;
                        sm_gains[sm_idx][color] = (uint8_t)g_val;
                    }
                }
                // printf("    -> Gains: R%d G%d B%d W%d Y%d\r\n", 
                //        sm_gains[sm_idx][0], sm_gains[sm_idx][1], sm_gains[sm_idx][2], 
                //        sm_gains[sm_idx][3], sm_gains[sm_idx][4]);

                // Line 9: Hints (R,G,B,W,Y) (跳过)
                f_gets(line_buffer, sizeof(line_buffer), &file[i]);
            }

            // 记录数据区起始位置
            data_start_offset[i] = f_tell(&file[i]);
        }
        else
        {
            file_open[i] = 0;
            // 如果文件没打开，清空对应缓冲区
            if (i < 4) {
                 memset(buffers_ws[i], 0, LED_TOTAL_BITS * sizeof(uint32_t));
            } else {
                 memset(buffers_sm[i-4], 0, SM_TOTAL_BITS * sizeof(uint16_t));
            }
        }
    }

    if (!any_file_open) {
        printf("[TF] No files found. Abort.\r\n");
        return;
    }

    // ==========================================
    // 2. 循环播放数据
    // ==========================================
    int r, g, b, w, y; // 解析变量
    
    while (1)
    {
        if (stop_play_flag) {
            printf("[TF] Play Stopped.\r\n");
            break;
        }

        // --- 遍历所有通道读取一帧 ---
        for (int ch = 0; ch < 6; ch++) 
        {
            if (file_open[ch])
            {
                uint16_t leds_to_read = channel_led_num[ch];
                
                for (int led = 0; led < leds_to_read; led++)
                {
                    // 读取一行
                    if (!f_gets(line_buffer, sizeof(line_buffer), &file[ch]))
                    {
                        // 读到文件尾，回绕到数据起始位置
                        f_lseek(&file[ch], data_start_offset[ch]);
                        f_gets(line_buffer, sizeof(line_buffer), &file[ch]);
                    }

                    // --- 解析数据 ---
                    if (ch < 4) 
                    {
                        // WS2812 (c1-c4): R,G,B (0-255)
                        if (sscanf(line_buffer, "%d,%d,%d", &r, &g, &b) == 3) {
                            LED2_PackChip(buffers_ws[ch], led, g, r, b); // GRB
                        }
                    }
                    else 
                    {
                        // SM15155 (c5-c6): R,G,B,W,Y (0-65535)
                        int sm_idx = ch - 4;
                        // 注意 CSV 里是 int，这里读出来可能是 65520，直接传入 uint16_t 即可
                        if (sscanf(line_buffer, "%d,%d,%d,%d,%d", &r, &g, &b, &w, &y) == 5) {
                            LED1_PackChip(buffers_sm[sm_idx], led, r, g, b, w, y);
                        }
                    }
                }
            }
        }

        // --- 数据打包收尾 ---
        
        // 1. WS2812 (TIM5) 添加 Reset 信号
         // 通道 1
        LED2_AppendReset(led_pwm_buffer_ch1, channel_led_num[0] * LED_BITS_PER_CHIP);
        // 通道 2
        LED2_AppendReset(led_pwm_buffer_ch2, channel_led_num[1] * LED_BITS_PER_CHIP);
        // 通道 3
        LED2_AppendReset(led_pwm_buffer_ch3, channel_led_num[2] * LED_BITS_PER_CHIP);
        // 通道 4
        LED2_AppendReset(led_pwm_buffer_ch4, channel_led_num[3] * LED_BITS_PER_CHIP);

        // 2. SM15155 (TIM3) 添加尾帧配置 (Current Gain)
        // 必须每帧都发，否则芯片不会锁存数据
        // [关键修改] SM15155 部分：传入实际灯珠数 channel_led_num
        // 这样尾帧就会准确地写在第1颗灯数据后面，而不是第200颗后面
        LED1_PackTail(led_pwm_buffer_ch5, channel_led_num[4],sm_gains[0][0], sm_gains[0][1], sm_gains[0][2], sm_gains[0][3], sm_gains[0][4]);
                      
        LED1_PackTail(led_pwm_buffer_ch6, channel_led_num[5],sm_gains[1][0], sm_gains[1][1], sm_gains[1][2], sm_gains[1][3], sm_gains[1][4]);

        // --- 启动发送 (并发) ---
        // 两个定时器是独立的 DMA，可以同时启动
        LED2_SendFrame_All(led_pwm_buffer_ch1, led_pwm_buffer_ch2, led_pwm_buffer_ch3, led_pwm_buffer_ch4, channel_led_num[0], channel_led_num[1], channel_led_num[2], channel_led_num[3]);
        LED1_SendFrame_All(led_pwm_buffer_ch5, led_pwm_buffer_ch6, channel_led_num[4], channel_led_num[5]);

        // 延时
        HAL_Delay(current_delay);
    }

    // 关闭所有文件
    for (int i = 0; i < 6; i++) {
        if (file_open[i]) f_close(&file[i]);
    }
}
