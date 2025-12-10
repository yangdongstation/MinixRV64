# MinixRV64 UART驱动文档

## 概述

MinixRV64 的 UART 驱动实现了一个功能完整的16550兼容串口驱动，支持缓冲I/O、中断驱动和多种配置选项。

## 架构

```
┌─────────────────────────────────────────────────────────┐
│              应用层/内核服务                              │
│       printk(), uart_puts(), uart_read()                │
└─────────────────────────────────────────────────────────┘
                         │
┌─────────────────────────────────────────────────────────┐
│                  UART驱动API层                           │
│  - uart_putc()     - uart_getc()                        │
│  - uart_write()    - uart_read()                        │
│  - uart_configure() - uart_flush_rx/tx()                │
└─────────────────────────────────────────────────────────┘
                         │
┌─────────────────────────────────────────────────────────┐
│               缓冲区管理层                                │
│  RX Buffer (256B)    TX Buffer (256B)                   │
│  循环缓冲区，支持溢出检测                                 │
└─────────────────────────────────────────────────────────┘
                         │
┌─────────────────────────────────────────────────────────┐
│              中断处理层                                   │
│  - RX中断: 接收数据到缓冲区                               │
│  - TX中断: 从缓冲区发送数据                               │
└─────────────────────────────────────────────────────────┘
                         │
┌─────────────────────────────────────────────────────────┐
│            硬件抽象层 (16550寄存器)                        │
│  RBR/THR, IER, IIR/FCR, LCR, MCR, LSR, MSR, DLL/DLM     │
└─────────────────────────────────────────────────────────┘
                         │
┌─────────────────────────────────────────────────────────┐
│                 物理硬件                                  │
│  QEMU virt: 0x10000000                                  │
│  MilkV Duo: 0x04140000                                  │
└─────────────────────────────────────────────────────────┘
```

## 特性

### ✓ 已实现
- 16550兼容寄存器访问
- 发送/接收循环缓冲区 (各256字节)
- 中断驱动模式
- 轮询模式
- 可配置波特率 (9600-115200)
- 可配置数据位 (5-8)
- 可配置校验位 (无/奇/偶)
- 可配置停止位 (1/2)
- 缓冲区管理 (flush)
- 统计信息收集

### ⚠ 部分实现
- 流控制 (RTS/CTS)
- DMA支持

### ☐ 待实现
- 多UART端口支持
- 自动波特率检测
- RS-485模式

## 数据结构

### 缓冲区结构
```c
typedef struct {
    char data[UART_BUFFER_SIZE];  // 256字节缓冲区
    volatile u32 head;             // 写入位置
    volatile u32 tail;             // 读取位置
    volatile u32 count;            // 当前数据量
} uart_buffer_t;
```

### 配置结构
```c
typedef struct {
    u32 baud_rate;    // 波特率: 9600, 19200, 38400, 57600, 115200
    u8 data_bits;     // 数据位: 5, 6, 7, 8
    u8 parity;        // 校验: NONE, ODD, EVEN
    u8 stop_bits;     // 停止位: 1, 2
} uart_config_t;
```

### 统计结构
```c
typedef struct {
    u64 tx_bytes;     // 发送字节数
    u64 rx_bytes;     // 接收字节数
    u32 tx_errors;    // 发送错误
    u32 rx_errors;    // 接收错误
    u32 overruns;     // 缓冲区溢出次数
} uart_stats_t;
```

## API参考

### 初始化
```c
void console_init(void);
```
初始化UART控制器和缓冲区。默认配置：115200波特率，8N1。

### 基本I/O

#### 字符输出
```c
void uart_putc(char c);
```
发送单个字符。自动处理换行符（\n -> \r\n）。

#### 字符输入
```c
char uart_getchar(void);
int uart_getc(void);
int uart_haschar(void);
```
- `uart_getchar()`: 阻塞读取，无数据时返回'\0'
- `uart_getc()`: 非阻塞读取，无数据时返回-1
- `uart_haschar()`: 检查是否有数据可读

#### 字符串输出
```c
void uart_puts(const char *s);
```
输出字符串。

### 块I/O

#### 写入数据
```c
int uart_write(const void *buf, size_t len);
```
写入数据块，返回实际写入的字节数。

#### 读取数据
```c
int uart_read(void *buf, size_t len);
```
读取数据块，返回实际读取的字节数。

### 配置

#### 配置UART参数
```c
int uart_configure(const uart_config_t *config);
```

示例：
```c
uart_config_t config = {
    .baud_rate = UART_BAUD_115200,
    .data_bits = UART_DATA_8,
    .parity = UART_PARITY_NONE,
    .stop_bits = UART_STOP_1
};
uart_configure(&config);
```

### 缓冲区管理

#### 清空缓冲区
```c
void uart_flush_rx(void);  // 清空接收缓冲区
void uart_flush_tx(void);  // 清空发送缓冲区
```

### 统计信息

#### 获取统计
```c
int uart_get_stats(uart_stats_t *stats);
```

示例：
```c
uart_stats_t stats;
uart_get_stats(&stats);
printk("TX: %llu bytes, RX: %llu bytes\n", stats.tx_bytes, stats.rx_bytes);
printk("Overruns: %u\n", stats.overruns);
```

### 中断控制

```c
void uart_enable_rx_interrupt(void);   // 使能接收中断
void uart_disable_rx_interrupt(void);  // 禁用接收中断
void uart_enable_tx_interrupt(void);   // 使能发送中断
void uart_disable_tx_interrupt(void);  // 禁用发送中断
```

### 中断处理

```c
void uart_interrupt_handler(void);
```
UART中断处理函数，由trap handler调用。

## 寄存器定义

### 16550 UART寄存器 (偏移量)

| 偏移 | 寄存器 | 读/写 | 说明 |
|------|--------|------|------|
| 0x00 | RBR | R | 接收缓冲寄存器 |
| 0x00 | THR | W | 发送保持寄存器 |
| 0x00 | DLL | R/W | 除数锁存器低字节 (DLAB=1) |
| 0x04 | IER | R/W | 中断使能寄存器 |
| 0x04 | DLM | R/W | 除数锁存器高字节 (DLAB=1) |
| 0x08 | IIR | R | 中断识别寄存器 |
| 0x08 | FCR | W | FIFO控制寄存器 |
| 0x0C | LCR | R/W | 线路控制寄存器 |
| 0x10 | MCR | R/W | 调制解调器控制寄存器 |
| 0x14 | LSR | R | 线路状态寄存器 |
| 0x18 | MSR | R | 调制解调器状态寄存器 |
| 0x1C | SCR | R/W | 临时寄存器 |

### 线路状态寄存器 (LSR)

| 位 | 名称 | 说明 |
|----|------|------|
| 0 | DR | 数据就绪 |
| 1 | OE | 溢出错误 |
| 2 | PE | 校验错误 |
| 3 | FE | 帧错误 |
| 4 | BI | 中断指示 |
| 5 | THRE | 发送保持寄存器空 |
| 6 | TEMT | 发送器空 |
| 7 | ERR | FIFO错误 |

### 中断使能寄存器 (IER)

| 位 | 名称 | 说明 |
|----|------|------|
| 0 | ERBFI | 使能接收数据可用中断 |
| 1 | ETBEI | 使能发送保持寄存器空中断 |
| 2 | ELSI | 使能接收线路状态中断 |
| 3 | EDSSI | 使能调制解调器状态中断 |

## 使用示例

### 基本输出
```c
// 初始化
console_init();

// 输出字符
uart_putc('H');
uart_putc('\n');

// 输出字符串
uart_puts("Hello, World!\n");
```

### 配置和数据传输
```c
// 自定义配置
uart_config_t config = {
    .baud_rate = UART_BAUD_9600,
    .data_bits = UART_DATA_8,
    .parity = UART_PARITY_EVEN,
    .stop_bits = UART_STOP_1
};
uart_configure(&config);

// 发送数据
char tx_data[] = "Test data";
uart_write(tx_data, sizeof(tx_data));

// 接收数据
char rx_data[64];
int received = uart_read(rx_data, sizeof(rx_data));
```

### 中断模式
```c
// 使能接收中断
uart_enable_rx_interrupt();

// 在trap handler中调用
void trap_handler(void) {
    // ...
    if (interrupt_source == UART_IRQ) {
        uart_interrupt_handler();
    }
    // ...
}
```

### 错误处理和统计
```c
// 检查统计信息
uart_stats_t stats;
uart_get_stats(&stats);

if (stats.overruns > 0) {
    printk("Warning: %u buffer overruns detected\n", stats.overruns);
    uart_flush_rx();  // 清空缓冲区
}
```

## 平台支持

### QEMU virt
- 基地址: `0x10000000`
- 时钟: 自动配置
- 中断: 通过PLIC

### MilkV Duo (CV1800B)
- 基地址: `0x04140000`
- 时钟: 25MHz
- 波特率除数: `clock / (16 * baud_rate)`

## 性能考虑

### 缓冲区大小
- 默认256字节，可通过 `UART_BUFFER_SIZE` 调整
- 更大的缓冲区可以减少溢出，但增加内存使用

### 轮询 vs 中断
- **轮询模式**: 简单，适合低速通信
- **中断模式**: 高效，适合高速通信和多任务

### 优化建议
1. 对于高速数据传输，使用中断模式
2. 定期检查和清理缓冲区
3. 监控溢出统计，必要时增加缓冲区
4. 使用批量I/O (`uart_write/read`) 而非单字符I/O

## 调试

### 启用调试输出
```c
#define UART_DEBUG 1

// 在关键位置添加调试信息
uart_puts("[UART] Initialized\n");
```

### 检查硬件状态
```c
// 读取LSR状态
u32 lsr = uart_read_reg(BOARD_UART_BASE, UART_LSR);
printk("LSR: 0x%x\n", lsr);
```

### 常见问题

1. **无输出**
   - 检查波特率配置
   - 确认UART基地址正确
   - 验证时钟配置

2. **乱码**
   - 检查数据位、校验位、停止位配置
   - 确认波特率匹配

3. **数据丢失**
   - 检查缓冲区溢出统计
   - 增加缓冲区大小
   - 使用中断模式

## 未来改进

- [ ] 多UART端口支持
- [ ] DMA传输
- [ ] 硬件流控制
- [ ] 错误注入测试
- [ ] 性能基准测试
