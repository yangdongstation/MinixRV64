# Minix RV64 开发指南

## 开发环境设置

### 1. 安装交叉编译工具链

```bash
# 运行自动化安装脚本
./tools/setup_toolchain.sh

# 或手动安装预构建工具链
wget https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/2024.04.12/riscv64-unknown-elf-gcc-13.2.0-1-linux-x86_64.tar.gz
sudo tar -xzf riscv64-unknown-elf-gcc-13.2.0-1-linux-x86_64.tar.gz -C /opt/
export PATH=$PATH:/opt/riscv64/bin
```

### 2. 编译内核

```bash
# 清理构建目录
make clean

# 编译内核
make CROSS_COMPILE=riscv64-unknown-elf-

# 查看生成的文件
ls -la minix-rv64.*
```

### 3. 调试设置

#### GDB 调试

```bash
# 启动 GDB
riscv64-unknown-elf-gdb minix-rv64.elf

# 连接到 QEMU (如果使用模拟器)
target remote localhost:1234

# 常用调试命令
break kinit
continue
info registers
x/20i $pc
```

#### 串口调试

确保已启用 `EARLY_PRINTK` 配置项，调试信息将通过 UART0 输出。

## 代码组织

### 核心模块

1. **启动代码** (`arch/riscv64/boot/`)
   - `start.S`: 汇编启动入口
   - 切换到 S 模式
   - 设置堆栈
   - 跳转到 C 入口

2. **内存管理** (`arch/riscv64/mm/`)
   - `mmu.c`: 虚拟内存管理
   - SV39 页表格式
   - 伙伴分配器
   - slab 分配器

3. **进程调度** (`kernel/`)
   - `sched.c`: ���程调度器
   - 优先级调度
   - 时间片轮转
   - 上下文切换

4. **中断处理** (`arch/riscv64/kernel/`)
   - `trap.c`: 异常和中断处理
   - 系统调用入口
   - 设备中断分发

5. **设备驱动** (`drivers/`)
   - `char/uart.c`: 串口驱动
   - `block/`: 块设备驱动
   - `net/`: 网络设备驱动

## 开发流程

### 1. 添加新功能

```c
// 1. 在头文件中声明
// include/minix/syscall.h
int sys_new_feature(void);

// 2. 实现功能
// kernel/syscalls.c
int sys_new_feature(void) {
    // 实现逻辑
    return 0;
}

// 3. 添加到系统调用表
// arch/riscv/kernel/trap.c
case SYS_NEW_FEATURE:
    return sys_new_feature();
```

### 2. 添加设备驱动

```c
// 1. 定义设备寄存器
// drivers/char/device.c
#define DEVICE_BASE   0xXXXX0000
#define DEVICE_CTRL   0x00
#define DEVICE_DATA   0x04

// 2. 实现初始化
void device_init(void) {
    writel(DEVICE_INIT, DEVICE_BASE + DEVICE_CTRL);
}

// 3. 实现操作接口
int device_write(char *data, int len) {
    // 实现写入
    return len;
}

// 4. 注册驱动
// drivers_init() 中添加
device_init();
```

### 3. 调试技巧

#### 使用 printk

```c
#include <minix/print.h>

void debug_function(void) {
    printk(KERN_DEBUG "Entering debug_function\n");
    printk(KERN_INFO "Some info\n");
    printk(KERN_ERR "Error occurred\n");
}
```

#### 断点调试

```bash
# 在 GDB 中设置断点
(gdb) break trap.c:do_trap
(gdb) break main.c:kinit

# 查看调用栈
(gdb) backtrace

# 查看变量
(gdb) print tf->scause
(gdb) print current_proc
```

## 移植注意事项

### 1. RISC-V 特定

- 使用 CSR 寄存器控制 CPU
- 内存序需要显式屏障
- 中断处理需要保存完整上下文
- 必须处理对齐异常

### 2. CV1800B 特定

- 时钟配置: 确保外设时钟使能
- Pinmux: 配置 GPIO 引脚复用
- 内存映射: 注意 IO 寄存器地址
- 启动顺序: 遵循芯片启动流程

### 3. Minix 兼容性

- 保持微内核架构
- 维护消息传递机制
- 兼容 POSIX 接口
- 保持进程模型

## 测试

### 1. 单元测试

```bash
# 运行内存管理测试
make test-mm

# 运行调度器测试
make test-sched
```

### 2. 集成测试

```bash
# 编译测试程序
cd tests/
make

# 运行完整测试套件
make run-tests
```

## 常见问题

### 1. 编译错误

**问题**: `unknown register name 'x0'`
**解决**: 确保使用支持 RISC-V 的汇编器版本

**问题**: `undefined reference to '__stack_end'`
**解决**: 检查链接器脚本中的符号定义

### 2. 运行时错误

**问题**: 启动后没有输出
**解决**:
- 检查 UART 初始化
- 验证时钟配置
- 查看异常原因

**问题**: 页错误异常
**解决**:
- 检查页表映射
- 验证内存权限
- 确认物理地址有效

### 3. 性能优化

- 使用内联汇编优化关键路径
- 实现零拷贝的 IPC
- 优化页表切换
- 使用缓存友好的数据结构

## 贡献指南

1. **提交前检查**
   - 确保代码通过编译
   - 运行测试套件
   - 遵循代码风格

2. **提交格式**
   ```
   [PATCH] module: brief description

   Detailed description of changes...

   Signed-off-by: Your Name <your@email.com>
   ```

3. **代码审查**
   - 提交 PR 到主分支
   - 等待代码审查
   - 根据反馈修改

## 参考资源

- [RISC-V 规范](https://riscv.org/technical/specifications/)
- [CV1800B 数据手册](https://milkv.io/docs/duo/specifications)
- [Minix 设计原理](http://www.minix3.org/)
- [RISC-V Linux 移植指南](https://www.kernel.org/doc/html/latest/riscv/)