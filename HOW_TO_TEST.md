# 如何测试 MinixRV64

## 🚀 快速开始

### 第一步：确认修复已应用
```bash
cd ~/MinixRV64

# 确认使用了正确的UART函数
grep "uart_getc()" kernel/shell.c
# 应该看到: int ch = uart_getc();

make clean
make
```

### 第二步：运行
```bash
make qemu
```

### 第三步：测试
等待系统启动，看到 `minix#` 提示符后，开始输入命令！

---

## 📝 测试流程

### 1. 启动系统

运行 `make qemu` 后，你会看到：

```
X1234
✓ MMU ready
✓ Scheduler
✓ Block device ready
✓ VFS ready

Minix RV64 ready
✓ Shell
minix# █
```

**重要**:
- ✅ 现在可以正常输入了！（已修复延迟问题）
- ⌨️ 直接开始打字即可
- ⏎ 按回车执行命令

### 2. 测试命令

#### 基础命令测试

```bash
minix# help          # 显示所有可用命令
minix# echo test     # 回显测试
minix# uname         # 显示系统信息
minix# ps            # 显示进程
minix# pwd           # 显示当前目录
```

#### 回显测试

```bash
minix# echo Hello World
Hello World

minix# echo Test 1 2 3
Test 1 2 3
```

#### 系统信息

```bash
minix# uname
Minix RV64 for RISC-V 64-bit
Board: QEMU virt

minix# ps
PID  CMD
  1  kernel
```

### 3. 退出QEMU

**方法1**: 按 `Ctrl+A` 然后按 `X`
**方法2**: 在另一个终端运行 `killall qemu-system-riscv64`

---

## 🎯 完整测试场景

### 场景1：基本功能测试（5分钟）

```bash
# 1. 启动系统
make qemu

# 2. 等待提示符

# 3. 测试help
help

# 4. 测试echo
echo Hello MinixRV64

# 5. 测试uname
uname

# 6. 测试ps
ps

# 7. 测试pwd
pwd

# 8. 退出
# 按 Ctrl+A 然后 X
```

**预期结果**: 所有命令正常执行，输出正确

### 场景2：输入测试（3分钟）

```bash
# 1. 测试基本输入
echo test

# 2. 测试退格
# 输入 "hellllo" 然后退格删除多余的 'l'
echo hello

# 3. 测试长输入
echo This is a very long command to test input buffer

# 4. 测试多参数
echo one two three four five
```

**预期结果**:
- ✅ 字符立即显示
- ✅ 退格正常工作
- ✅ 长命令正常显示

### 场景3：清屏测试（1分钟）

```bash
# 1. 输入几个命令
help
ps
uname

# 2. 清屏
clear

# 3. 验证屏幕被清空
```

**预期结果**: 屏幕清空，只显示 `minix#` 提示符

---

## 🔍 故障排查

### 问题1：无法输入

**症状**: 光标闪烁但打字没反应

**解决**:
```bash
# 确保已经重新编译
make clean
make

# 检查kernel/shell.c中的延迟是否为1000
grep -n "delay < 1000" kernel/shell.c

# 应该看到:
# 127:            for (delay = 0; delay < 1000; delay++)
```

### 问题2：输入有延迟

**症状**: 能输入但有明显延迟

**解决**: 可以进一步减小延迟
```c
// 在 kernel/shell.c 第127行
for (delay = 0; delay < 500; delay++)  // 从1000改为500
```

### 问题3：字符重复

**症状**: 一个字符显示两次

**解决**: 检查终端设置
```bash
# 确保没有本地回显
stty -echo
make qemu
```

### 问题4：QEMU无输出

**症状**: 运行后黑屏

**解决**:
```bash
# 检查二进制文件
ls -lh minix-rv64.elf

# 应该看到最新时间戳和约177KB大小

# 重新编译
make clean
make
```

---

## 📊 测试检查清单

运行系统后，验证以下功能：

### 启动检查
- [ ] 看到 "X1234"
- [ ] 看到 "✓ MMU ready"
- [ ] 看到 "✓ Scheduler"
- [ ] 看到 "✓ Block device ready"
- [ ] 看到 "✓ VFS ready"
- [ ] 看到 "✓ Shell"
- [ ] 看到 "minix#" 提示符

### 输入检查
- [ ] 可以输入字符
- [ ] 字符立即显示
- [ ] 退格键工作
- [ ] 回车键执行命令

### 命令检查
- [ ] help命令显示13个命令
- [ ] echo正确回显
- [ ] uname显示系统信息
- [ ] ps显示进程列表
- [ ] pwd显示 "/"
- [ ] clear清空屏幕

### 稳定性检查
- [ ] 连续执行10个命令无崩溃
- [ ] 长命令（>50字符）正常工作
- [ ] 特殊字符正常处理

---

## 🎬 演示脚本

想要快速演示？使用这个脚本：

```bash
# 运行演示
./quick_test.sh

# 然后按照屏幕提示操作
```

---

## 📸 截图指南

如果需要截图记录测试结果：

### 1. 启动截图
显示系统启动过程

### 2. help命令截图
显示所有可用命令

### 3. 功能测试截图
显示echo、uname、ps等命令执行

### 4. 长命令截图
显示长输入处理能力

---

## 💡 提示和技巧

### 快速测试序列
复制这些命令用于快速测试：
```
help
echo test
uname
ps
```

### 性能测试
测试输入响应速度：
```
echo 1234567890123456789012345678901234567890
```

### 压力测试
连续执行多个命令：
```bash
help
ps
uname
echo test1
echo test2
echo test3
pwd
clear
help
```

---

## 📚 相关文档

- [INPUT_FIX.md](INPUT_FIX.md) - 输入问题修复说明
- [TEST_COMMANDS.txt](TEST_COMMANDS.txt) - 测试命令列表
- [TEST_GUIDE.md](TEST_GUIDE.md) - 完整测试指南
- [TEST_REPORT.md](TEST_REPORT.md) - 测试报告

---

## ✅ 成功标准

测试通过的标准：

1. ✅ 系统启动无错误
2. ✅ 可以立即输入命令
3. ✅ 所有基本命令正常工作
4. ✅ 退格和回车正常
5. ✅ 无崩溃或死机

---

## 🎉 享受测试！

现在你的MinixRV64系统应该可以完美运行了。

有问题？查看 [INPUT_FIX.md](INPUT_FIX.md) 获取详细说明。

**快乐测试！** 🚀

---

*MinixRV64 v0.0001 - 测试指南*
*最后更新: 2025-12-10*
