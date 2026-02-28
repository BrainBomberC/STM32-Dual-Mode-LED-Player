#ifndef __TF_DRIVER_H
#define __TF_DRIVER_H

#include "main.h"
#include "fatfs.h"

// 假设在 fatfs.c 中 SD_Driver 是第一个链接的，所以是 0:/
#define TF_PATH "0:/"

// 导出全局文件系统对象
extern FATFS fs_tf;


// 外部控制标志位 (用于打断播放)
extern volatile uint8_t stop_play_flag;




// API 声明
uint8_t TF_Init(void);
uint8_t TF_Test_Write(void);
uint8_t TF_Test_Read(void);

uint8_t TF_SaveFrameCSV(const char* filename, float* frameData, int len);





// 同步读取 c1.csv, c2.csv, c3.csv, c4.csv 并分别映射到通道1~4
// 如果某通道对应的文件不存在或已读完，则该通道输出复位（不发数据）
void TF_PlayCSV_Multi(uint16_t delay_ms);

// static int get_csv_int(char *line, int column_index);

#endif
