#include "lcd_driver.h"
#include "usbd_def.h"
#include "usbd_msc_bot.h"

// --- 新增必要的头文件 ---
#include <stdio.h>      // 用于 sprintf
#include "fatfs.h"      // 用于 f_stat, FILINFO, FR_OK
#include "usb_device.h" // 用于 hUsbDeviceFS
#include "usbd_msc.h"   // 用于 MSC 类定义

// 引用外部变量
extern USBD_HandleTypeDef hUsbDeviceFS;

// ==========================================
//               底层SPI函数
// ==========================================

// 写 8位 数据
static void LCD_Write_Byte(uint8_t data)
{
    HAL_SPI_Transmit(LCD_SPI_HANDLE, &data, 1, 100);
}

// 写命令
static void LCD_Write_Cmd(uint8_t cmd)
{
    LCD_DC_CMD();
    LCD_Write_Byte(cmd);
}

// 写数据 (8位)
static void LCD_Write_Data(uint8_t data)
{
    LCD_DC_DATA();
    LCD_Write_Byte(data);
}

// 写 16位 数据 (颜色, MSB First)
static void LCD_Write_Data16(uint16_t data)
{
    LCD_DC_DATA();
    uint8_t buf[2];
    buf[0] = data >> 8;   
    buf[1] = data & 0xFF; 
    HAL_SPI_Transmit(LCD_SPI_HANDLE, buf, 2, 10);
}

// 设置绘图区域 (ST7789 1.14寸 横屏偏移修正)
void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint16_t x_offset = 40; 
    uint16_t y_offset = 53; // 这里的偏移量请根据实际显示效果微调

    LCD_Write_Cmd(0x2a); // Column Address Set
    LCD_Write_Data((x1 + x_offset) >> 8);
    LCD_Write_Data(x1 + x_offset);
    LCD_Write_Data((x2 + x_offset) >> 8);
    LCD_Write_Data(x2 + x_offset);

    LCD_Write_Cmd(0x2b); // Row Address Set
    LCD_Write_Data((y1 + y_offset) >> 8);
    LCD_Write_Data(y1 + y_offset);
    LCD_Write_Data((y2 + y_offset) >> 8);
    LCD_Write_Data(y2 + y_offset);

    LCD_Write_Cmd(0x2c); // Memory Write
}

// ==========================================
//               功能函数
// ==========================================

/**
 * @brief  初始化LCD屏幕 (ST7789)
 * @note   包含硬件复位和寄存器配置序列，配置为横屏模式(0x70)
 * @example LCD_Init();
 */
void LCD_Init(void)
{
    LCD_CS_LOW();
    LCD_BLK_ON(); 

    // 硬件复位
    LCD_RST_HIGH();
    HAL_Delay(50);
    LCD_RST_LOW();
    HAL_Delay(100);
    LCD_RST_HIGH();
    HAL_Delay(120);

    // --- ST7789 初始化 ---
    LCD_Write_Cmd(0x11); // Sleep Out
    HAL_Delay(120);

    LCD_Write_Cmd(0x3A); 
    LCD_Write_Data(0x05); // 16-bit/pixel (RGB565)

    LCD_Write_Cmd(0x36); 
    LCD_Write_Data(0x70); // 横屏模式 (Y, X交换, Y镜像)

    LCD_Write_Cmd(0x21); // Inversion ON (反色开启，解决颜色不对的问题)

    LCD_Write_Cmd(0xB2);
    LCD_Write_Data(0x0C); LCD_Write_Data(0x0C); LCD_Write_Data(0x00); LCD_Write_Data(0x33); LCD_Write_Data(0x33); 

    LCD_Write_Cmd(0xB7); LCD_Write_Data(0x35); 
    LCD_Write_Cmd(0xBB); LCD_Write_Data(0x19);
    LCD_Write_Cmd(0xC0); LCD_Write_Data(0x2C);
    LCD_Write_Cmd(0xC2); LCD_Write_Data(0x01);
    LCD_Write_Cmd(0xC3); LCD_Write_Data(0x12);
    LCD_Write_Cmd(0xC4); LCD_Write_Data(0x20);
    LCD_Write_Cmd(0xC6); LCD_Write_Data(0x0F);
    LCD_Write_Cmd(0xD0); LCD_Write_Data(0xA4); LCD_Write_Data(0xA1);

    LCD_Write_Cmd(0xE0);
    LCD_Write_Data(0xD0); LCD_Write_Data(0x04); LCD_Write_Data(0x0D); LCD_Write_Data(0x11);
    LCD_Write_Data(0x13); LCD_Write_Data(0x2B); LCD_Write_Data(0x3F); LCD_Write_Data(0x54);
    LCD_Write_Data(0x4C); LCD_Write_Data(0x18); LCD_Write_Data(0x0D); LCD_Write_Data(0x0B);
    LCD_Write_Data(0x1F); LCD_Write_Data(0x23);

    LCD_Write_Cmd(0xE1);
    LCD_Write_Data(0xD0); LCD_Write_Data(0x04); LCD_Write_Data(0x0C); LCD_Write_Data(0x11);
    LCD_Write_Data(0x13); LCD_Write_Data(0x2C); LCD_Write_Data(0x3F); LCD_Write_Data(0x44);
    LCD_Write_Data(0x51); LCD_Write_Data(0x2F); LCD_Write_Data(0x1F); LCD_Write_Data(0x1F);
    LCD_Write_Data(0x20); LCD_Write_Data(0x23);

    LCD_Write_Cmd(0x29); // Display On
    HAL_Delay(50);
    
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK); 
}

/**
 * @brief  填充矩形区域颜色
 * @param  xsta, ysta: 起始坐标
 * @param  xend, yend: 结束坐标 (不包含)
 * @param  color: 填充颜色
 * @example LCD_Fill(0, 0, 240, 135, RED);
 */
void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color)
{
    uint16_t i, j;
    LCD_Address_Set(xsta, ysta, xend - 1, yend - 1);
    for(i = ysta; i < yend; i++)
    {
        for(j = xsta; j < xend; j++)
        {
            LCD_Write_Data16(color);
        }
    }
}

// ==========================================
//          字体显示函数 (多种尺寸)
// ==========================================

// 通用字符显示逻辑宏 (为了减少代码重复)
// W: 字符宽, H: 字符高, FONT_ARR: 字模数组
#define SHOW_CHAR_IMPL(W, H, FONT_ARR) \
    uint8_t temp; \
    uint8_t i, t; \
    uint8_t bytes_per_line = (W + 7) / 8; /* 计算每行占多少字节 */ \
    if(x > LCD_W - W || y > LCD_H - H) return; \
    LCD_Address_Set(x, y, x + W - 1, y + H - 1); \
    num = num - ' '; \
    LCD_DC_DATA(); \
    for(i = 0; i < H; i++) { \
        for(uint8_t k = 0; k < bytes_per_line; k++) { \
            temp = FONT_ARR[num][i * bytes_per_line + k]; \
            for(t = 0; t < 8; t++) { \
                if((k * 8 + t) < W) { /* 防止越界 */ \
                    if(temp & 0x80) LCD_Write_Data16(color); \
                    else LCD_Write_Data16(bk_color); \
                    temp <<= 1; \
                } \
            } \
        } \
    }

// 通用字符串显示逻辑宏
#define SHOW_STRING_IMPL(W, H, CHAR_FUNC) \
    while(*p != '\0') { \
        if(x > LCD_W - W) { x = 0; y += H; } \
        if(y > LCD_H - H) { y = x = 0; LCD_Fill(0, 0, LCD_W, LCD_H, bk_color); } \
        CHAR_FUNC(x, y, *p, color, bk_color); \
        x += W; \
        p++; \
    }

// --- 6x12 字体 ---
/**
 * @brief  显示 6x12 字符
 * @example LCD_ShowChar_1206(10, 10, 'A', RED, BLACK);
 */
void LCD_ShowChar_1206(uint16_t x, uint16_t y, uint8_t num, uint16_t color, uint16_t bk_color)
{
    // asc2_1206 是 12行，每行1字节(只用前6位)
    SHOW_CHAR_IMPL(6, 12, asc2_1206);
}

void LCD_ShowString_1206(uint16_t x, uint16_t y, const char *p, uint16_t color, uint16_t bk_color)
{
    SHOW_STRING_IMPL(6, 12, LCD_ShowChar_1206);
}

// --- 8x16 字体 ---
/**
 * @brief  显示 8x16 字符
 * @example LCD_ShowChar_1608(10, 10, 'A', WHITE, BLUE);
 */
void LCD_ShowChar_1608(uint16_t x, uint16_t y, uint8_t num, uint16_t color, uint16_t bk_color)
{
    SHOW_CHAR_IMPL(8, 16, asc2_1608);
}

void LCD_ShowString_1608(uint16_t x, uint16_t y, const char *p, uint16_t color, uint16_t bk_color)
{
    SHOW_STRING_IMPL(8, 16, LCD_ShowChar_1608);
}

// --- 12x24 字体 ---
/**
 * @brief  显示 12x24 字符
 * @note   需要字模 asc2_2412 (48字节/字, 逐行式)
 * @example LCD_ShowString_2412(0, 0, "Hello", GREEN, BLACK);
 */
void LCD_ShowChar_2412(uint16_t x, uint16_t y, uint8_t num, uint16_t color, uint16_t bk_color)
{
    // 宽12，每行需2字节(0-7, 8-11)
    SHOW_CHAR_IMPL(12, 24, asc2_2412);
}

void LCD_ShowString_2412(uint16_t x, uint16_t y, const char *p, uint16_t color, uint16_t bk_color)
{
    SHOW_STRING_IMPL(12, 24, LCD_ShowChar_2412);
}

// --- 16x32 字体 ---
/**
 * @brief  显示 16x32 字符
 * @note   需要字模 asc2_3216 (64字节/字, 逐行式)
 */
void LCD_ShowChar_3216(uint16_t x, uint16_t y, uint8_t num, uint16_t color, uint16_t bk_color)
{
    SHOW_CHAR_IMPL(16, 32, asc2_3216);
}

void LCD_ShowString_3216(uint16_t x, uint16_t y, const char *p, uint16_t color, uint16_t bk_color)
{
    SHOW_STRING_IMPL(16, 32, LCD_ShowChar_3216);
}


// ==========================================
//               UI 状态显示函数
// ==========================================

/**
 * @brief  在LCD上显示系统状态（USB/PLAYER）
 * @param  mode: 当前系统模式（如MODE_USB_MSC, MODE_PLAYER等）
 * @note   USB模式下会检测主机连接和MSC弹出状态，并显示相应提示
 * @example LCD_Display_Status(MODE_USB_MSC);
 */
void LCD_Display_Status(SystemMode mode)
{
  LCD_Fill(0, 0, 240, 135, WHITE); // 顶部清屏区域
  LCD_ShowString_3216(20, 50, "MODE:", BLACK, WHITE);
  
  if(mode == MODE_USB_MSC) {
    // 优先使用 MSC 的 scsi_medium_state 判断是否被主机弹出
    uint8_t hostConnected = 0;
    
    // 1. 判断 USB 设备层是否配置成功
    if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) {
      hostConnected = 1;
    }
    
    // 2. 深入检查 MSC 类状态 (是否被弹出)
    for (uint8_t i = 0; i < USBD_MAX_SUPPORTED_CLASS; i++) {
      if (hUsbDeviceFS.pClassDataCmsit[i] != NULL) {
        USBD_MSC_BOT_HandleTypeDef *hmsc = (USBD_MSC_BOT_HandleTypeDef *)hUsbDeviceFS.pClassDataCmsit[i];
        if (hmsc != NULL) {
          if (hmsc->scsi_medium_state == SCSI_MEDIUM_EJECTED) {
            hostConnected = 0; // 媒体被弹出
          } else {
            hostConnected = 1;
          }
          break;
        }
      }
    }

    LCD_ShowString_3216(100, 50, "USB", BLACK, WHITE);
    if (hostConnected) {
      LCD_ShowString_1608(160, 62, "Connected", BLACK, WHITE);
    } else {
      LCD_ShowString_1608(160, 62, "No Host", RED, WHITE);
    }
  } else {
    LCD_ShowString_3216(100, 50, "PLAYER", BLACK, WHITE);
  }
}

/**
 * @brief  在Player模式下显示已识别的CSV通道文件（c1-c6）
 * @note   检查SD卡根目录下c1.csv~c6.csv文件，存在则在LCD上依次显示
 * @example LCD_Display_PlayerFiles();
 */
void LCD_Display_PlayerFiles(void)
{
  // 检查 6 个文件
  const char *names[6] = {"0:/c1.csv", "0:/c2.csv", "0:/c3.csv", "0:/c4.csv", "0:/c5.csv", "0:/c6.csv"};
  FILINFO fno;
  int x = 20;
  const int y = 95; // 在大字（y≈50）下方显示小字
  char label[8];

  // 显示标题前缀
  LCD_ShowString_1608(20, 80, "Found:", BLACK, WHITE);

  for (uint8_t i = 0; i < 6; i++) {
    if (f_stat(names[i], &fno) == FR_OK) {
      sprintf(label, "c%u", (unsigned)(i + 1)); // 显示 c1, c2... 省略 .csv 以节省空间
      LCD_ShowString_1608(x, y, label, BLACK, WHITE);
      x += 35; // 间距调整为35，以便放下6个
    }
  }
}