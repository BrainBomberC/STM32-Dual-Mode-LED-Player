# STM32F412 六通道灯效播放器与 USB 读卡平台

## 目录

- [1. 项目定位](#1-项目定位)
- [2. 第一性原理理解这个项目](#2-第一性原理理解这个项目)
- [3. 使用场景](#3-使用场景)
- [4. 平台资源配置](#4-平台资源配置)
  - [4.1 MCU 与内存](#41-mcu-与内存)
  - [4.2 系统时钟](#42-系统时钟)
  - [4.3 外设资源](#43-外设资源)
  - [4.4 DMA 分配](#44-dma-分配)
- [5. 硬件接口与引脚映射](#5-硬件接口与引脚映射)
  - [5.1 灯带输出](#51-灯带输出)
  - [5.2 USB](#52-usb)
  - [5.3 TF 卡](#53-tf-卡)
  - [5.4 串口](#54-串口)
  - [5.5 LCD](#55-lcd)
  - [5.6 按键与指示](#56-按键与指示)
- [6. 软件分层与目录结构](#6-软件分层与目录结构)
- [7. 系统两种工作模式](#7-系统两种工作模式)
  - [7.1 USB Mode](#71-usb-mode)
  - [7.2 Player Mode](#72-player-mode)
  - [7.3 设计两种模式的原因](#73-设计两种模式的原因)
- [8. 主函数执行逻辑分析](#8-主函数执行逻辑分析)
  - [8.1 基础初始化](#81-基础初始化)
  - [8.2 启动串口与 TF 卡底层](#82-启动串口与-tf-卡底层)
  - [8.3 默认进入 USB Mode](#83-默认进入-usb-mode)
  - [8.4 主循环轮询](#84-主循环轮询)
  - [8.5 中断与回调](#85-中断与回调)
- [9. 核心数据链路](#9-核心数据链路)
  - [9.1 USB 模式数据链路](#91-usb-模式数据链路)
  - [9.2 播放模式数据链路](#92-播放模式数据链路)
  - [9.3 为什么能稳定输出](#93-为什么能稳定输出)
- [10. 重要驱动文件说明](#10-重要驱动文件说明)
  - [10.1 `Core/Src/main.c`](#101-coresrcmainc)
  - [10.2 `tf_driver/tf_driver.c`](#102-tf_drivertf_driverc)
  - [10.3 `led_driver2/led_driver2.c/.h`](#103-led_driver2led_driver2ch)
  - [10.4 `led_driver1/led_driver1.c/.h`](#104-led_driver1led_driver1ch)
  - [10.5 `USB_DEVICE/App/usb_device.c`](#105-usb_deviceappusb_devicec)
  - [10.6 `USB_DEVICE/App/usbd_desc.c`](#106-usb_deviceappusbd_descc)
  - [10.7 `USB_DEVICE/App/usbd_storage_if.c`](#107-usb_deviceappusbd_storage_ifc)
  - [10.8 `USB_DEVICE/Target/usbd_conf.c`](#108-usb_devicetargetusbd_confc)
  - [10.9 `FATFS/Target/bsp_driver_sd.c`](#109-fatfstargetbsp_driver_sdc)
  - [10.10 `lcd_driver/lcd_driver.c`](#1010-lcd_driverlcd_driverc)
  - [10.11 `Core/Src/tim.c`](#1011-coresrctimc)
  - [10.12 `Core/Src/sdio.c`](#1012-coresrcsdioc)
- [11. 灯珠数量限制与数据通道维护](#11-灯珠数量限制与数据通道维护)
  - [11.1 通道类型](#111-通道类型)
  - [11.2 当前代码上限](#112-当前代码上限)
  - [11.3 为什么不是无限增加](#113-为什么不是无限增加)
- [12. CSV 数据组织方式](#12-csv-数据组织方式)
  - [12.1 文件命名](#121-文件命名)
  - [12.2 RGB 通道](#122-rgb-通道)
  - [12.3 RGBWY 通道](#123-rgbwy-通道)
  - [12.4 播放方式](#124-播放方式)
- [13. 具体使用方式](#13-具体使用方式)
  - [13.1 硬件准备](#131-硬件准备)
  - [13.2 导入灯效文件](#132-导入灯效文件)
  - [13.3 切换到播放模式](#133-切换到播放模式)
  - [13.4 切回 USB 模式](#134-切回-usb-模式)
- [14. 注意事项与已知限制](#14-注意事项与已知限制)
  - [14.1 必须先安全弹出](#141-必须先安全弹出)
  - [14.2 卡检测与异常保护还不够强](#142-卡检测与异常保护还不够强)
  - [14.3 多通道不是严格全局同步](#143-多通道不是严格全局同步)
  - [14.4 帧间隔不是完全硬实时](#144-帧间隔不是完全硬实时)
- [15. 维护建议](#15-维护建议)
  - [15.1 结构优化建议](#151-结构优化建议)
  - [15.2 稳定性建议](#152-稳定性建议)
  - [15.3 性能建议](#153-性能建议)

## 1. 项目定位

这是一个基于 `STM32F412RET6` 的混合灯效测试与播放平台。

它同时承担两件事：

1. 作为 `USB MSC` 设备，把 `TF 卡` 暴露给电脑，表现成一个可读写的 U 盘。
2. 作为本地灯效播放器，从 `TF 卡 CSV 文件` 中读取像素数据，实时驱动 6 路灯带输出。

项目不是纯粹的“读卡器”，也不是纯粹的“灯控器”，而是一个围绕 `TF 卡` 做资源切换的双模式平台。

## 2. 第一性原理理解这个项目

从第一性原理看，这个系统本质上是在解决一个核心问题：

`如何让同一份灯效数据，既能方便地由电脑写入，又能被单片机稳定地转换成严格时序的灯珠控制波形。`

系统拆开后只有四层：

1. `存储层`
   - TF 卡保存灯效 CSV 文件。
2. `访问层`
   - 电脑通过 USB 访问 TF 卡。
   - 单片机通过 SDIO 访问 TF 卡。
3. `解释层`
   - `tf_driver` 负责把 CSV 文本解析为每颗灯珠的颜色值。
4. `执行层`
   - `TIM + DMA` 把颜色值转换成严格时序的 PWM 波形，输出到灯带。

设计 `USB Mode` 和 `Player Mode` 两种模式的原因：

- 因为同一时刻不能既让 `电脑` 持有这张卡，又让 `单片机本地文件系统` 同时持有这张卡。
- 否则会出现文件系统冲突、缓存不一致、甚至卡损坏。

所以这个平台的本质不是“功能切换”，而是：`对 TF 卡访问权的仲裁。`

## 3. 使用场景

适合以下场景：

- 灯带协议验证平台
- 多通道灯效样机演示
- 工厂或研发现场快速导入灯效文件
- 同时验证 `RGB` 与 `RGBWY` 两类灯珠
- 作为 USB 读卡器与本地播放器的一体化测试板

不适合以下场景：

- 对多路输出要求严格硬同步的工业控制
- 需要复杂动态脚本引擎或网络控制的产品化系统
- 完整量产前的最终商用品固件

## 4. 平台资源配置

### 4.1 MCU 与内存

- MCU: `STM32F412RET6`
- Flash: `512 KB`
- RAM: `256 KB`

### 4.2 系统时钟

- `SYSCLK = 96 MHz`
- `USB CLK = 48 MHz`
- `APB1 = 48 MHz`
- `APB2 = 96 MHz`

### 4.3 外设资源

- `USB_OTG_FS`
  - 设备模式
  - 设备类为 `MSC`
- `SDIO`
  - TF 卡存储访问
- `TIM5`
  - 4 路 RGB 灯输出
- `TIM3`
  - 2 路 RGBWY 灯输出
- `DMA`
  - 共 6 路 DMA 用于定时器 PWM 数据搬运
- `USART1`
  - 串口日志输出与简单命令接收
- `SPI3`
  - 驱动 `ST7789` LCD
- `GPIO EXTI`
  - `PC13` 按键切换模式

### 4.4 DMA 分配

- `TIM5_CH1 -> DMA1_Stream2`
- `TIM5_CH2 -> DMA1_Stream4`
- `TIM5_CH3 -> DMA1_Stream0`
- `TIM5_CH4 -> DMA1_Stream1`
- `TIM3_CH2 -> DMA1_Stream5`
- `TIM3_CH3 -> DMA1_Stream7`

## 5. 硬件接口与引脚映射

### 5.1 灯带输出

- `PA0 -> TIM5_CH1 -> RGB 通道 1`
- `PA1 -> TIM5_CH2 -> RGB 通道 2`
- `PA2 -> TIM5_CH3 -> RGB 通道 3`
- `PA3 -> TIM5_CH4 -> RGB 通道 4`
- `PA7 -> TIM3_CH2 -> RGBWY 通道 5`
- `PB0 -> TIM3_CH3 -> RGBWY 通道 6`

### 5.2 USB

- `PA11 -> USB_OTG_FS_DM`
- `PA12 -> USB_OTG_FS_DP`

### 5.3 TF 卡

- `PC8 -> SDIO_D0`
- `PC9 -> SDIO_D1`
- `PC10 -> SDIO_D2`
- `PC11 -> SDIO_D3`
- `PC12 -> SDIO_CK`
- `PD2 -> SDIO_CMD`
- `PA8 -> Detect_SDIO 输入`

### 5.4 串口

- `PA9 -> USART1_TX`
- `PA10 -> USART1_RX`

### 5.5 LCD

- `PB3 -> SPI3_SCK`
- `PB5 -> SPI3_MOSI`
- `PA15 -> LCD_CS`
- `PB4 -> LCD_DC`
- `PB6 -> LCD_RST`
- `PB7 -> LCD_BLK`

### 5.6 按键与指示

- `PC13 -> 模式切换按键`
- `PC0 / PC1 / PC2 -> 板载状态指示输出`

## 6. 软件分层与目录结构

```text
Core/
  Src/main.c                 主程序、状态机、回调入口
  Src/tim.c                  TIM3/TIM5 初始化
  Src/sdio.c                 SDIO 初始化
  Src/usart.c                USART1 初始化
  Src/spi.c                  SPI3 初始化
  Src/gpio.c                 GPIO/按键/指示灯初始化

tf_driver/
  tf_driver.c                TF 卡挂载、CSV 解析、循环播放

led_driver2/
  led_driver2.c/.h           4 路 RGB 灯 PWM 打包与发送

led_driver1/
  led_driver1.c/.h           2 路 RGBWY 灯 PWM 打包与发送

USB_DEVICE/
  App/usb_device.c           USB 设备栈初始化入口
  App/usbd_desc.c            USB 描述符
  App/usbd_storage_if.c      USB 存储类与 TF 卡读写桥接
  Target/usbd_conf.c         USB Device Core 与 USB_OTG_FS 硬件桥接层

FATFS/
  Target/bsp_driver_sd.c     SD 卡底层 BSP
  Target/sd_diskio.c         FatFs 与 SDIO 的块设备接口

lcd_driver/
  lcd_driver.c/.h            ST7789 状态显示
```

## 7. 系统两种工作模式

## 7.1 USB Mode

目的：

- 让电脑把设备识别成 `U 盘`
- 直接向 TF 卡根目录写入或修改 `c1.csv ~ c6.csv`

逻辑：

1. `USB_OTG_FS` 接收来自电脑的 USB 数据包。
2. `USB Device Core + MSC` 解析成块读写请求。
3. `usbd_storage_if.c` 把请求转换成 `HAL_SD_ReadBlocks / HAL_SD_WriteBlocks`。
4. `SDIO` 访问 TF 卡。
5. 返回数据给电脑。

对外表现：

- 电脑识别成 `STM32 Card Reader`
- 用户像操作普通 U 盘一样拷贝灯效文件

## 7.2 Player Mode

目的：

- 单片机脱离电脑，自主从 TF 卡读取灯效并输出到 6 路灯带

逻辑：

1. `tf_driver` 打开 `c1.csv ~ c6.csv`
2. 解析每一帧的颜色数据
3. 打包为 PWM 占空比数组
4. `TIM + DMA` 输出严格时序波形
5. 灯珠锁存并显示

对外表现：

- LCD 显示 `PLAYER`
- 灯带按照 CSV 内容循环播放

## 7.3 设计两种模式的原因

因为 `USB Mode` 和 `Player Mode` 访问的是同一张 TF 卡。

- USB 模式下，卡的“主人”是电脑
- 播放模式下，卡的“主人”是单片机本地文件系统

如果两边同时访问，文件系统会失去一致性。

所以模式切换的本质是：

`切换 TF 卡所有权。`

## 8. 主函数执行逻辑分析

主程序入口在 `Core/Src/main.c`。

可以把 `main()` 理解成五个阶段。

### 8.1 基础初始化

依次完成：

- `HAL_Init()`
- `SystemClock_Config()`
- GPIO
- DMA
- SDIO
- USART1
- FATFS
- TIM5
- USB_DEVICE
- SPI3
- TIM3

这一步的目标不是立刻播放，而是把所有“能力模块”先准备出来。

### 8.2 启动串口与 TF 卡底层

主程序随后会：

- 开启 `USART1` 中断接收
- 调用 `TF_Init()` 检查并挂载 TF 卡
- 对 SD 卡做一次预初始化

这样做的目的，是保证后续不论进入 USB 模式还是播放模式，底层存储都已经处于基本可用状态。

### 8.3 默认进入 USB Mode

上电后默认：

- `app_mode = MODE_USB_MSC`
- 启动 USB 设备栈
- LCD 显示当前模式

设计意图很明确：

`先让用户能方便地导入文件，再决定是否播放。`

### 8.4 主循环轮询

`while(1)` 里主要做三件事：

1. 监听按键切换模式
2. 在 `MODE_PLAYER` 下调用 `TF_PlayCSV_Multi()`
3. 在 `MODE_USB_MSC` 下轮询 USB 与 MSC 状态，并刷新 LCD

也就是说，主循环本身不是复杂任务调度器，而是一个比较直接的有限状态机。

### 8.5 中断与回调

项目里几个关键回调：

- `HAL_UART_RxCpltCallback()`
  - 处理串口接收
- `HAL_GPIO_EXTI_Callback()`
  - 处理 `PC13` 按键
- `HAL_TIM_PWM_PulseFinishedCallback()`
  - 处理 6 路 PWM DMA 发送完成事件

主程序的思路是：

- 中断里只做事件通知或完成标记
- 复杂流程在主循环中处理

这是一种典型而合理的嵌入式分工方式。

## 9. 核心数据链路

### 9.1 USB 模式数据链路

```text
电脑
-> USB 线
-> USB_OTG_FS 硬件
-> usbd_conf.c / HAL_PCD
-> USB Device Core
-> MSC 类
-> usbd_storage_if.c
-> HAL_SD_ReadBlocks / HAL_SD_WriteBlocks
-> SDIO
-> TF 卡
```

### 9.2 播放模式数据链路

```text
TF 卡
-> FatFs
-> tf_driver.c
-> CSV 解析
-> led_driver1 / led_driver2
-> PWM Buffer
-> DMA
-> TIM3 / TIM5
-> 灯带
```

### 9.3 为什么能稳定输出

因为时序最敏感的部分不是由 CPU 一位一位手搓，而是由硬件定时器和 DMA 完成。

CPU 的职责只是：

- 读文件
- 解析 CSV
- 准备下一帧缓冲区

这大幅降低了位级时序抖动。

## 10. 重要驱动文件说明

### 10.1 `Core/Src/main.c`

职责：

- 应用入口
- 状态机控制
- USB 模式与播放模式切换
- LCD 状态刷新
- 关键中断回调

它决定“系统现在在做什么”。

### 10.2 `tf_driver/tf_driver.c`

职责：

- TF 卡初始化与挂载
- 逐个打开 `c1.csv ~ c6.csv`
- 解析灯珠数量、延时、增益和像素数据
- 逐帧打包 PWM 数据
- 启动 6 路并行发送

它决定“TF 卡里的数据如何变成一帧灯效”。

### 10.3 `led_driver2/led_driver2.c/.h`

职责：

- 驱动前 4 路 `RGB` 灯带
- 协议模型为 `24 bit / 颗`
- 使用 `TIM5 + 4 路 DMA`
- 为 WS2812 类灯珠打包 `GRB` 数据

它决定“RGB 灯珠如何被编码与发送”。

### 10.4 `led_driver1/led_driver1.c/.h`

职责：

- 驱动后 2 路 `RGBWY` 灯带
- 芯片为 `SM15155E`
- 每颗 `80 bit`
- 使用 `TIM3 + 2 路 DMA`
- 每帧尾部追加增益与尾帧配置

它决定“RGBWY 灯珠如何被编码与发送”。

### 10.5 `USB_DEVICE/App/usb_device.c`

职责：

- 初始化 USB 设备栈
- 注册 `MSC` 类
- 绑定存储接口
- 启动 USB Device

它决定“设备要以什么 USB 身份出现”。

### 10.6 `USB_DEVICE/App/usbd_desc.c`

职责：

- 向电脑提供 VID/PID、产品名、接口字符串等描述符

它决定“电脑识别到的设备是谁”。

### 10.7 `USB_DEVICE/App/usbd_storage_if.c`

职责：

- 实现 MSC 后端存储接口
- 把 USB 块读写请求转成 TF 卡块访问

它决定“电脑对 U 盘的访问如何落到 TF 卡上”。

### 10.8 `USB_DEVICE/Target/usbd_conf.c`

职责：

- 连接 `USB Device Core` 与 `USB_OTG_FS` 硬件
- 完成端点、FIFO、回调和 PCD 初始化

它不是业务层，而是桥接层。

### 10.9 `FATFS/Target/bsp_driver_sd.c`

职责：

- SD 卡 BSP 初始化
- 总线宽度与底层访问支持

它决定“SD 卡硬件层是否工作正常”。

### 10.10 `lcd_driver/lcd_driver.c`

职责：

- 驱动 `ST7789`
- 显示当前模式
- 显示 USB 连接状态和播放文件状态

它决定“用户能看到什么状态反馈”。

### 10.11 `Core/Src/tim.c`

职责：

- 初始化 `TIM3` 与 `TIM5`
- 为 LED 时序输出提供基础节拍

它决定“PWM 波形时基是否正确”。

### 10.12 `Core/Src/sdio.c`

职责：

- 初始化 SDIO 时钟、引脚和中断

它决定“TF 卡访问通道是否建立起来”。

## 11. 灯珠数量限制与数据通道维护

### 11.1 通道类型

- `CH1 ~ CH4`: `RGB`
- `CH5 ~ CH6`: `RGBWY`

### 11.2 当前代码上限

- RGB 每路最多 `200` 颗
- RGBWY 每路最多 `200` 颗

总计：

- `4 x 200 = 800` 颗 RGB
- `2 x 200 = 400` 颗 RGBWY

### 11.3 为什么不是无限增加

因为每路都需要预留较大的 PWM 缓冲区，DMA 发送依赖这些内存。

这个项目当前是：

- 功能上限受宏定义限制
- 实际上限还受 `256 KB RAM` 约束

所以如果继续增加灯珠数量，需要同步评估：

- SRAM 余量
- DMA 发送时长
- 帧刷新周期
- 电源供给能力

## 12. CSV 数据组织方式

### 12.1 文件命名

- `c1.csv` 对应通道 1
- `c2.csv` 对应通道 2
- `c3.csv` 对应通道 3
- `c4.csv` 对应通道 4
- `c5.csv` 对应通道 5
- `c6.csv` 对应通道 6

### 12.2 RGB 通道

适用于 `c1 ~ c4`。

每行表示一颗灯珠：

```csv
R,G,B
255,0,0
0,255,0
0,0,255
```

### 12.3 RGBWY 通道

适用于 `c5 ~ c6`。

每行表示一颗灯珠：

```csv
R,G,B,W,Y
65535,0,0,0,0
0,65535,0,0,0
0,0,65535,0,0
```

### 12.4 播放方式

- 文件按通道分别循环读取
- 读到结尾会回到数据区起点
- `TF_PlayCSV_Multi()` 采用的是“按帧读取并打包，再 DMA 输出”的方式

## 13. 具体使用方式

### 13.1 硬件准备

1. 接入 6 路灯带
2. 插入 TF 卡
3. 接好 LCD、USB 和供电
4. 确认灯带供电与地线共地

### 13.2 导入灯效文件

1. 上电后设备默认进入 `USB Mode`
2. 用 USB 连接电脑
3. 电脑识别出 U 盘后，将 `c1.csv ~ c6.csv` 复制到 TF 卡根目录
4. 在电脑侧执行安全弹出

### 13.3 切换到播放模式

1. 按下 `PC13` 按键
2. 系统停止 USB 存储服务
3. 本地重新挂载 TF 卡
4. 进入 `Player Mode`
5. 开始循环播放灯效

### 13.4 切回 USB 模式

1. 再次按下 `PC13`
2. 停止播放
3. 卸载本地文件系统
4. 重新启动 USB MSC

## 14. 注意事项与已知限制

### 14.1 必须先安全弹出

这是当前平台最重要的使用要求。

如果电脑还在占用 TF 卡，而你直接按键切到 `Player Mode`，有文件系统损坏风险。

### 14.2 卡检测与异常保护还不够强

当前工程更偏研发测试平台，不是完全收敛后的量产固件。

遇到以下情况时，需要提高警惕：

- 无卡启动
- 坏卡启动
- 电脑拷贝大文件时强行切模式

### 14.3 多通道不是严格全局同步

当前 6 路输出是并行启动，但不是“全局硬同步锁存”。

对大多数肉眼可见灯效问题不大，但如果场景要求严格同步，需要额外设计。

### 14.4 帧间隔不是完全硬实时

位级输出时序由 `TIM + DMA` 保证，但帧与帧之间还受以下因素影响：

- TF 卡读文件速度
- CSV 解析耗时
- `HAL_Delay()`

所以它更准确地说是：

`位级硬实时，帧级软实时。`

## 15. 维护建议

### 15.1 结构优化建议

- 把 `main.c` 中的模式切换流程拆成独立函数
- 把 SDIO 调速逻辑下沉到 SD/TF 驱动层
- 把 USB 状态读取封装成接口，减少对中间件内部结构的直接访问

### 15.2 稳定性建议

- 增加真实的 TF 卡在位检测
- 增加 SD 读写超时与错误恢复
- 在切换到 `Player Mode` 前增加“主机已安全弹出”判定
- 清理重复的 USB 初始化路径

### 15.3 性能建议

- 如果后续需要更稳定的连续播放，可考虑双缓冲
- 如果要扩大灯珠数量，需要重新评估 SRAM 占用
- 如果要提升多路同步性，需要增加统一触发与锁存设计




