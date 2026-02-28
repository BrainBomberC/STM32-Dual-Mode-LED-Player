#ifndef __LCD_DRIVER_H
#define __LCD_DRIVER_H

#include "main.h"
#include "tft_font.h"


// ==========================================
//               屏幕参数配置
// ==========================================

// ST7789 1.14寸 分辨率定义
// 横屏模式下
#define LCD_W 240    
#define LCD_H 135       

// 颜色定义 (RGB565 格式)
#define WHITE         0xFFFF
#define BLACK         0x0000
#define BLUE          0x001F
#define BRED          0XF81F
#define GRED          0XFFE0
#define GBLUE         0X07FF
#define RED           0xF800
#define MAGENTA       0xF81F
#define GREEN         0x07E0
#define CYAN          0x7FFF
#define YELLOW        0xFFE0
#define BROWN         0XBC40 //棕色
#define BRRED         0XFC07 //棕红色
#define GRAY          0X8430 //灰色

// ==========================================
//               硬件接口宏
// ==========================================
extern SPI_HandleTypeDef hspi3;
#define LCD_SPI_HANDLE &hspi3

// 片选 CS (PA15)
#define LCD_CS_LOW()      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define LCD_CS_HIGH()     HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)

// 数据/命令 DC (PB4)
#define LCD_DC_CMD()      HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define LCD_DC_DATA()     HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)

// 复位 RST (PB6)
#define LCD_RST_LOW()     HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define LCD_RST_HIGH()    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)

// 背光 BLK (PB7, PMOS控制: 低电平亮)
#define LCD_BLK_ON()      HAL_GPIO_WritePin(LCD_BLK_GPIO_Port, LCD_BLK_Pin, GPIO_PIN_RESET) 
#define LCD_BLK_OFF()     HAL_GPIO_WritePin(LCD_BLK_GPIO_Port, LCD_BLK_Pin, GPIO_PIN_SET)

// ==========================================
//               函数声明
// ==========================================

/**
 * @brief  初始化LCD屏幕 (SPI3, ST7789)
 * @example LCD_Init();
 */
void LCD_Init(void);

/**
 * @brief  填充矩形区域颜色
 * @param  xsta: 起始X坐标
 * @param  ysta: 起始Y坐标
 * @param  xend: 结束X坐标
 * @param  yend: 结束Y坐标
 * @param  color: 填充颜色 (RGB565)
 * @example LCD_Fill(0, 0, 240, 135, BLACK); // 全屏刷黑
 */
void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);

// --- 6x12 字体显示 ---
void LCD_ShowChar_1206(uint16_t x, uint16_t y, uint8_t num, uint16_t color, uint16_t bk_color);
void LCD_ShowString_1206(uint16_t x, uint16_t y, const char *p, uint16_t color, uint16_t bk_color);

// --- 8x16 字体显示 ---
void LCD_ShowChar_1608(uint16_t x, uint16_t y, uint8_t num, uint16_t color, uint16_t bk_color);
void LCD_ShowString_1608(uint16_t x, uint16_t y, const char *p, uint16_t color, uint16_t bk_color);

// --- 12x24 字体显示 ---
void LCD_ShowChar_2412(uint16_t x, uint16_t y, uint8_t num, uint16_t color, uint16_t bk_color);
void LCD_ShowString_2412(uint16_t x, uint16_t y, const char *p, uint16_t color, uint16_t bk_color);

// --- 16x32 字体显示 ---
void LCD_ShowChar_3216(uint16_t x, uint16_t y, uint8_t num, uint16_t color, uint16_t bk_color);
void LCD_ShowString_3216(uint16_t x, uint16_t y, const char *p, uint16_t color, uint16_t bk_color);


void LCD_Display_PlayerFiles(void);
void LCD_Display_Status(SystemMode mode);

#endif