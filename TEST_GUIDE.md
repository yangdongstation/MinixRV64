# MinixRV64 测试指南

## 快速测试

### 1. 编译和快速运行测试
```bash
# 清理并编译
make clean && make

# 运行快速测试（5秒）
./test_features.sh
```

### 2. 交互式测试
```bash
# 启动QEMU进入交互式shell
make qemu

# 按 Ctrl+A 然后按 X 退出QEMU
```

## 可用的Shell命令

### 基本命令
```bash
help        # 显示所有可用命令
echo <text> # 回显文本
clear       # 清屏
uname       # 显示系统信息
pwd         # 显示当前目录
```

### 进程管理
```bash
ps          # 列出进程
kill <pid>  # 终止进程（未实现）
```

### 文件系统命令（部分实现）
```bash
ls          # 列出目录内容
cat <file>  # 显示文件内容
mkdir <dir> # 创建目录
rm <file>   # 删除文件
cd <dir>    # 改变目录
```

### 系统命令
```bash
reboot      # 重启系统
```

## 测试场景

### 场景1: 系统启动测试
**目标**: 验证系统能正常启动并显示所有模块初始化信息

**步骤**:
```bash
make qemu
```

**预期输出**:
```
X1234
✓ MMU ready
✓ Scheduler
✓ Block device ready
✓ VFS ready

Minix RV64 ready
✓ Shell
minix#
```

**验证**:
- [✓] 看到 "MMU ready"
- [✓] 看到 "Scheduler"
- [✓] 看到 "VFS ready"
- [✓] 看到 "Shell"
- [✓] 出现 "minix#" 提示符

### 场景2: UART功能测试
**目标**: 验证UART驱动的基本输入输出功能

**步骤**:
```bash
# 在minix# 提示符下输入
help
echo Hello World
echo Test UART driver
uname
```

**预期输出**:
```
minix# help
Available commands:
  help - Show available commands
  clear - Clear screen
  echo - Echo arguments
  ...

minix# echo Hello World
Hello World

minix# uname
Minix RV64 for RISC-V 64-bit
Board: QEMU virt
```

**验证**:
- [✓] help命令显示所有命令
- [✓] echo正确回显
- [✓] uname显示系统信息
- [✓] 输入的字符正确显示
- [✓] 退格键正常工作

### 场景3: 进程信息测试
**目标**: 验证进程管理框架

**步骤**:
```bash
ps
```

**预期输出**:
```
minix# ps
PID  CMD
  1  kernel
```

**验证**:
- [✓] 显示内核进程

### 场景4: 清屏测试
**目标**: 验证终端控制

**步骤**:
```bash
help
echo line1
echo line2
clear
```

**预期**: 屏幕被清空

### 场景5: 符号表验证
**目标**: 验证新增功能是否正确编译进二进制

**步骤**:
```bash
# 检查UART符号
riscv64-unknown-elf-nm minix-rv64.elf | grep uart_

# 检查VFS符号
riscv64-unknown-elf-nm minix-rv64.elf | grep vfs_

# 检查文件系统符号
riscv64-unknown-elf-nm minix-rv64.elf | grep -E 'devfs|ramfs'
```

**预期**:
- 看到 uart_configure, uart_write, uart_read 等函数
- 看到 vfs_init, vfs_mount, vfs_open 等函数
- 看到 devfs_init, ramfs_init 等函数

## 性能测试

### UART吞吐量测试
```bash
# 在shell中快速输入大量字符测试缓冲区
echo 1234567890123456789012345678901234567890
```

**验证**:
- [✓] 所有字符正确显示
- [✓] 无字符丢失

### 连续命令测试
```bash
help
ps
uname
echo test1
echo test2
echo test3
```

**验证**:
- [✓] 所有命令正确执行
- [✓] 无系统崩溃

## 当前已知限制

### 功能限制
- ❌ 文件系统命令未完全实现（ls, cat, mkdir等显示"not yet mounted"）
- ❌ 文件系统未自动挂载
- ❌ kill命令未实现
- ❌ reboot执行后进入WFI循环，不会真正重启

### 需要手动测试的功能
由于文件系统未自动挂载，以下功能需要代码级测试：

1. **VFS路径解析**
   - 需要在代码中调用 vfs_lookup_path()
   - 测试 ".", "..", 多级路径

2. **devfs设备文件系统**
   - 需要挂载: vfs_mount(NULL, "/dev", "devfs")
   - 需要注册设备: devfs_register_device()

3. **ramfs内存文件系统**
   - 需要挂载: vfs_mount(NULL, "/tmp", "ramfs")
   - 需要创建文件进行读写测试

4. **UART高级功能**
   - 需要代码调用 uart_configure()
   - 需要代码调用 uart_get_stats()
   - 中断模式需要配置中断控制器

## 调试测试

### GDB调试测试
```bash
# 终端1: 启动QEMU并等待GDB连接
make qemu-gdb

# 终端2: 连接GDB
make gdb
```

**GDB命令**:
```gdb
# 设置断点
break kernel_main
break console_init
break vfs_init

# 继续执行
continue

# 查看变量
print uart_state

# 查看调用栈
backtrace

# 单步执行
step
next

# 查看寄存器
info registers
```

## 自动化测试脚本

### test_features.sh
快速功能测试，5秒超时
```bash
./test_features.sh
```

### run_interactive_test.sh
交互式测试，发送预定义命令
```bash
./run_interactive_test.sh
```

## 测试输出日志

所有测试输出保存在:
- `/tmp/minix_test.log` - 快速测试输出
- `/tmp/minix_interactive.log` - 交互测试输出

## 查看日志
```bash
cat /tmp/minix_test.log
cat /tmp/minix_interactive.log
```

## 常见问题

### Q1: QEMU无输出
**检查**:
- 是否正确编译: `make clean && make`
- 是否使用正确的QEMU版本: `qemu-system-riscv64 --version`

### Q2: 输入无响应
**检查**:
- UART是否正确初始化
- 检查启动日志中是否有错误

### Q3: 字符显示乱码
**检查**:
- 终端编码设置
- UART配置是否正确（应为8N1）

## 下一步测试计划

### 短期（需要添加代码）
1. [ ] 在kernel_main中挂载devfs和ramfs
2. [ ] 添加文件系统测试命令
3. [ ] 实现ls/cat/mkdir命令
4. [ ] 添加UART统计显示命令

### 中期
1. [ ] 添加单元测试框架
2. [ ] 添加自动化测试套件
3. [ ] 实现进程创建测试
4. [ ] 添加内存管理测试

### 长期
1. [ ] 完整的EXT2文件系统测试
2. [ ] 网络协议栈测试
3. [ ] 多进程并发测试
4. [ ] 压力测试和性能基准

## 测试报告模板

```
测试日期: YYYY-MM-DD
测试者:
版本: 0.0001

| 测试场景 | 结果 | 备注 |
|---------|------|------|
| 系统启动 | ✓/✗ |      |
| UART输入输出 | ✓/✗ |      |
| help命令 | ✓/✗ |      |
| echo命令 | ✓/✗ |      |
| ps命令 | ✓/✗ |      |
| uname命令 | ✓/✗ |      |

发现的问题:
1.
2.

建议:
1.
2.
```

## 贡献测试用例

欢迎贡献新的测试用例！请按照以下格式：

```
### 场景X: <测试名称>
**目标**: <测试目的>

**步骤**:
1. ...
2. ...

**预期结果**:
...

**实际结果**:
...

**状态**: ✓ 通过 / ✗ 失败 / ⚠ 部分通过
```
