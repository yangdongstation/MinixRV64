# MinixRV64 文件系统架构

## 概述

MinixRV64 实现了一个分层的文件系统架构，包含虚拟文件系统(VFS)层和多个具体的文件系统实现。

## 架构图

```
┌─────────────────────────────────────────────────────────┐
│                   应用程序接口                            │
│         open(), read(), write(), close()                │
└─────────────────────────────────────────────────────────┘
                         │
┌─────────────────────────────────────────────────────────┐
│                  VFS 虚拟文件系统层                        │
│  - 路径解析 (支持 . 和 ..)                                │
│  - 挂载点管理                                             │
│  - 文件系统注册                                           │
│  - Inode缓存                                             │
└─────────────────────────────────────────────────────────┘
         │              │              │              │
    ┌────┴────┐    ┌────┴────┐    ┌────┴────┐    ┌────┴────┐
    │  devfs  │    │  ramfs  │    │  ext2   │    │  fat32  │
    └─────────┘    └─────────┘    └─────────┘    └─────────┘
         │              │              │              │
    ┌────┴────┐         │         ┌────┴────────────┴────┐
    │  设备    │         │         │     块设备接口        │
    │  驱动    │         │         │  (SD卡/eMMC/虚拟磁盘) │
    └─────────┘         │         └─────────────────────┘
                    ┌───┴───┐
                    │  内存  │
                    └───────┘
```

## VFS层 (fs/vfs.c)

### 核心数据结构

#### inode_t - 索引节点
```c
typedef struct inode {
    u64 ino;                    // Inode编号
    u32 mode;                   // 文件类型和权限
    u32 nlink;                  // 硬链接数
    u32 uid;                    // 用户ID
    u32 gid;                    // 组ID
    u64 size;                   // 文件大小
    u64 atime;                  // 访问时间
    u64 mtime;                  // 修改时间
    u64 ctime;                  // 状态改变时间
    void *fs_private;           // 文件系统私有数据
    struct inode *parent;       // 父目录
} inode_t;
```

#### file_t - 文件描述符
```c
typedef struct file {
    inode_t *inode;             // 关联的inode
    u64 pos;                    // 当前读写位置
    u32 flags;                  // 打开标志
    void *private;              // 私有数据
} file_t;
```

#### fs_ops_t - 文件系统操作接口
```c
typedef struct fs_ops {
    int (*mount)(const char *device, const char *mount_point);
    int (*unmount)(const char *mount_point);
    inode_t *(*read_inode)(u64 ino);
    int (*write_inode)(inode_t *inode);
    ssize_t (*read)(file_t *file, void *buf, size_t count);
    ssize_t (*write)(file_t *file, const void *buf, size_t count);
    int (*mkdir)(inode_t *parent, const char *name, u32 mode);
    inode_t *(*lookup)(inode_t *dir, const char *name);
    // ... 更多操作
} fs_ops_t;
```

### 主要功能

1. **路径解析** (`vfs_lookup_path`)
   - 支持绝对路径和相对路径
   - 处理 `.` (当前目录) 和 `..` (父目录)
   - 逐级解析路径组件

2. **挂载管理** (`vfs_mount`, `vfs_unmount`)
   - 支持多个挂载点
   - 自动选择最长匹配的挂载点

3. **文件系统注册** (`vfs_register_fs`)
   - 动态注册文件系统类型
   - 支持多种文件系统共存

## devfs - 设备文件系统 (fs/devfs.c)

### 特点
- 提供 `/dev` 下的设备节点
- 支持字符设备和块设备
- 动态设备注册/注销

### 使用示例
```c
// 注册字符设备
devfs_register_device("console", DEVFS_TYPE_CHAR, 1, 0, 0666);
devfs_register_device("tty0", DEVFS_TYPE_CHAR, 4, 0, 0660);

// 注册块设备
devfs_register_device("sda", DEVFS_TYPE_BLOCK, 8, 0, 0660);
devfs_register_device("sda1", DEVFS_TYPE_BLOCK, 8, 1, 0660);

// 挂载devfs
vfs_mount(NULL, "/dev", "devfs");

// 打开设备
file_t *f = vfs_open("/dev/console", O_RDWR);
```

### 配置
- 最大设备节点数: 64 (`MAX_DEV_NODES`)

## ramfs - 内存文件系统 (fs/ramfs.c)

### 特点
- 完全在内存中运行
- 支持文件和目录
- 文件内容动态增长
- 系统重启后数据丢失

### 使用示例
```c
// 挂载ramfs
vfs_mount(NULL, "/tmp", "ramfs");

// 创建目录
vfs_mkdir("/tmp/test", 0755);

// 创建和写入文件
file_t *f = vfs_open("/tmp/hello.txt", O_CREAT | O_RDWR);
vfs_write(f, "Hello, World!", 13);
vfs_close(f);

// 读取文件
f = vfs_open("/tmp/hello.txt", O_RDONLY);
char buf[64];
vfs_read(f, buf, sizeof(buf));
vfs_close(f);
```

### 限制
- 最大文件数: 256 (`RAMFS_MAX_FILES`)
- 单个文件最大大小: 1MB (`RAMFS_MAX_FILE_SIZE`)

## EXT2 文件系统 (fs/ext2.c)

### 当前状态
- ✓ 超级块读取和验证
- ✓ 挂载支持
- ✓ Inode结构定义
- ⚠ 文件读写 (部分实现)
- ⚠ 目录操作 (部分实现)

### 数据结构
- 超级块 (1024字节偏移)
- 块组描述符
- Inode表
- 数据块

### TODO
- [ ] 完整的inode读取
- [ ] 块映射和数据读取
- [ ] 目录遍历
- [ ] 文件创建和删除

## FAT32 文件系统 (fs/fat32.c)

### 当前状态
- ✓ FAT表结构定义
- ✓ 目录项定义
- ⚠ 挂载支持 (部分实现)
- ⚠ 文件操作 (部分实现)

### TODO
- [ ] FAT表解析
- [ ] 簇链遍历
- [ ] 长文件名支持
- [ ] 文件读写

## 文件类型和权限

### 文件类型 (mode & S_IFMT)
- `S_IFREG` (0x8000) - 普通文件
- `S_IFDIR` (0x4000) - 目录
- `S_IFCHR` (0x2000) - 字符设备
- `S_IFBLK` (0x6000) - 块设备
- `S_IFIFO` (0x1000) - FIFO
- `S_IFLNK` (0xA000) - 符号链接

### 权限位
- `S_IRUSR` (0x100) - 用户读
- `S_IWUSR` (0x080) - 用户写
- `S_IXUSR` (0x040) - 用户执行
- `S_IRGRP` (0x020) - 组读
- `S_IWGRP` (0x010) - 组写
- `S_IXGRP` (0x008) - 组执行
- `S_IROTH` (0x004) - 其他读
- `S_IWOTH` (0x002) - 其他写
- `S_IXOTH` (0x001) - 其他执行

## 打开标志

- `O_RDONLY` (0x0) - 只读
- `O_WRONLY` (0x1) - 只写
- `O_RDWR` (0x2) - 读写
- `O_APPEND` (0x8) - 追加模式
- `O_CREAT` (0x100) - 不存在则创建
- `O_TRUNC` (0x200) - 截断文件

## 使用流程

### 初始化
```c
// 1. 初始化VFS
vfs_init();

// 2. 注册文件系统
devfs_init();
ramfs_init();
ext2_init();

// 3. 挂载文件系统
vfs_mount(NULL, "/dev", "devfs");
vfs_mount(NULL, "/tmp", "ramfs");
vfs_mount("/dev/sda1", "/", "ext2");

// 4. 注册设备
devfs_register_device("console", DEVFS_TYPE_CHAR, 1, 0, 0666);
```

### 文件操作
```c
// 打开文件
file_t *file = vfs_open("/tmp/test.txt", O_CREAT | O_RDWR);
if (file == NULL) {
    // 错误处理
}

// 写入数据
char data[] = "Hello, World!";
ssize_t written = vfs_write(file, data, sizeof(data));

// 读取数据
char buffer[128];
file->pos = 0;  // 重置位置
ssize_t read_bytes = vfs_read(file, buffer, sizeof(buffer));

// 关闭文件
vfs_close(file);
```

### 目录操作
```c
// 创建目录
vfs_mkdir("/tmp/newdir", 0755);

// 读取目录内容
file_t *dir = vfs_open("/tmp", O_RDONLY);
dirent_t entries[32];
int count = vfs_readdir(dir->inode, entries, 32);

for (int i = 0; i < count; i++) {
    printk("File: %s (inode=%llu)\n", entries[i].name, entries[i].ino);
}

vfs_close(dir);
```

## 未来改进

### 短期
- [ ] 完善EXT2文件系统实现
- [ ] 添加文件缓存机制
- [ ] 实现inode缓存
- [ ] 添加错误处理和恢复

### 中期
- [ ] 支持符号链接
- [ ] 实现文件锁
- [ ] 添加文件系统统计
- [ ] procfs文件系统

### 长期
- [ ] 日志文件系统支持
- [ ] 文件系统检查工具
- [ ] 网络文件系统 (NFS)
- [ ] FUSE用户态文件系统支持
