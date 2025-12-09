# Minix RV64 QEMU 测试���南

## 测试步骤

由于当前环境没有 RISC-V 交叉编译工具链，以下是完整的测试步骤说明：

### 1. 安装工具链

```bash
# 选项1：使用我们提供的脚本
./tools/setup_toolchain.sh

# 选项2：Ubuntu/Debian 系统
sudo apt-get install gcc-riscv64-linux-gnu

# 选项3：从源码编译
git clone https://github.com/riscv/riscv-gnu-toolchain
cd riscv-gnu-toolchain
./configure --prefix=/opt/riscv
make
```

### 2. 编译内核

```bash
# 使用 build.sh 脚本
./build.sh

# 或使用 make
make BOARD=qemu-virt CROSS_COMPILE=riscv64-linux-gnu- all
```

### 3. 运行 QEMU 测试

```bash
# 方法1：使用 make 目标
make qemu

# 方法2：直接使用 QEMU 命令
qemu-system-riscv64 \
    -machine virt \
    -cpu rv64 \
    -nographic \
    -bios none \
    -kernel minix-rv64.elf \
    -smp 1 \
    -m 128M
```

## 预期输出

成功运行后，你应该看到类似以下输出：

```
=== Minix RV64 Booting ===
Board: QEMU RISC-V Virt
Kernel loaded at: 0x80000000
BSS cleared, size: 12345 bytes
Initializing kernel subsystems...
✓ Trap handling initialized
✓ Memory management initialized
✓ Scheduler initialized
✓ Drivers initialized
✓ Interrupts enabled
Entering kernel main loop...
```

## 调试模式

如果需要调试，可以：

```bash
# 启动 QEMU 并等待 GDB 连接
make qemu-gdb

# 在另一个终端连接 GDB
riscv64-linux-gnu-gdb minix-rv64.elf
(gdb) target remote localhost:1234
(gdb) break kinit
(gdb) continue
```

## 项目验证

### 1. 检查项目结构
```bash
./tests/build_test.sh
```

### 2. 查看生成的文件
```bash
ls -la minix-rv64.*
```

### 3. 检查 ELF 信息
```bash
riscv64-linux-gnu-readelf -h minix-rv64.elf
```

## 故障排除

### 常见问题

1. **工具链未找到**
   - 确保 `riscv64-unknown-elf-gcc` 或 `riscv64-linux-gnu-gcc` 在 PATH 中
   - 运行 `which riscv64-*-gcc` 确认

2. **编译错误**
   - 检查所有源文件是否存在
   - 确认头文件路径正确

3. **QEMU 无法启动**
   - 确保 QEMU 版本支持 RISC-V
   - 使用 `qemu-system-riscv64 -version` 检查版本

4. **没有输出**
   - 检查 UART 初始化代码
   - 确认 QEMU 参数正确

## 项目状态

当前项目框架已完成，包括：
- ✅ 完整的源代码结构
- ✅ 构建系统
- ✅ 多平台支持
- ✅ 基础驱动框架
- ✅ 中断处理
- ✅ 调试支持

下一步需要完成：
- [ ] 实现完整的内存管理
- [ ] 完善进程调度
- [ ] 添加更多驱动
- [ ] 实现文件系统

## 联系方式

如有问题或需要帮助，请查看：
- ARCHITECTURE.md - 架构设计文档
- DEVELOPMENT.md - 开发指南
- README_QEMU.md - QEMU 测试详细文档