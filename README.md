# Linux 救星工具箱 (Linux Savior Toolbox)

> 一个让 Linux 新手也能轻松维护系统的图形化工具箱。不用记命令，点几下鼠标就能搞定系统更新、清理、修复等常见问题。

## 🎯 这个软件能帮你做什么？

如果你刚从 Windows 转到 Linux，或者不想记那些复杂的命令行，这个工具就是为你准备的。它把常用的系统维护操作做成了按钮，点击即可执行，还会告诉你每个操作是干什么的、有没有风险。

### 主要功能

| 功能 | 说明 |
|------|------|
| 🖥️ **首页仪表盘** | 一眼看到 CPU、内存、磁盘使用情况，还有系统推荐操作 |
| 🚨 **紧急修复** | 桌面卡住了？回收站删不掉？包管理器锁死了？一键修复 |
| 📦 **系统更新** | 自动识别你的系统（Ubuntu/Fedora/Arch等），一键更新所有软件 |
| 🧹 **系统清理** | 清理缓存、日志、无用依赖，释放磁盘空间 |
| 🔄 **桌面修复** | 重启桌面环境、文件管理器、输入法，解决卡顿和异常 |
| 💻 **终端模拟器** | 内置终端，支持多标签、主题切换、快捷命令 |
| 🎨 **GNOME 扩展** | 浏览、安装、管理 GNOME 桌面扩展 |
| 💾 **备份恢复** | 备份桌面配置，创建系统快照，出问题时一键还原 |
| 🔧 **双系统工具** | 修复 Windows/Linux 双系统时间错乱、管理 GRUB 启动项、挂载 Windows 分区 |
| 📊 **系统诊断** | 查看硬件信息、生成诊断报告 |
| ⚙️ **服务/进程管理** | 管理开机启动项、系统服务、运行中的进程 |
| 🌐 **网络诊断** | 测试网络连通性、查看端口、重启网络服务 |

## 🖥️ 支持的系统

### Linux 发行版
- ✅ Ubuntu / Linux Mint / Debian（apt）
- ✅ Fedora / RHEL（dnf）
- ✅ openSUSE（zypper）
- ✅ Arch Linux / Manjaro（pacman）

> 软件会自动检测你的系统，使用对应的包管理器命令。

### 桌面环境
- ✅ GNOME
- ✅ KDE Plasma
- ✅ XFCE

## 📦 安装方法

### 方法一：下载安装包（推荐新手）

从 [Releases](https://github.com/ASHM2011/linux-savior-toolbox/releases) 页面下载对应你系统的安装包：

**Debian / Ubuntu / Mint（.deb 文件）：**
```bash
sudo dpkg -i linux-savior-toolbox_5.2.0_amd64.deb
sudo apt install -f   # 如果提示缺少依赖，运行这个修复
```

**Fedora / RHEL（.rpm 文件）：**
```bash
sudo dnf install linux-savior-toolbox_5.2.0_amd64.rpm
```

**所有发行版（AppImage，免安装）：**
```bash
chmod +x LinuxSaviorToolbox-5.2.0-x86_64.AppImage
./LinuxSaviorToolbox-5.2.0-x86_64.AppImage
```

### 方法二：从源码编译

适合想自己修改代码的用户。需要先安装 Qt 6 开发库。

```bash
# 安装依赖（以 Ubuntu 为例）
sudo apt install qt6-base-dev qt6-tools-dev libqt6core5compat6-dev

# 编译
mkdir build && cd build
cmake ..
make -j$(nproc)

# 运行
./LinuxSaviorToolbox
```

## ⚠️ 安全说明

- 所有需要管理员权限的操作都会通过系统授权弹窗（pkexec）请求密码，**不会记住你的密码**
- 危险操作（如删除文件、恢复系统快照）会有二次确认提示
- 软件不会在后台偷偷上传你的数据，所有操作都在本地执行

## 🔧 可选依赖

部分功能需要额外安装软件才能使用：

| 功能 | 需要的软件 | 安装方法（Ubuntu） |
|------|-----------|-------------------|
| 终端模拟器 | qtermwidget6 | `sudo apt install qtermwidget6` |
| 系统快照 | timeshift | `sudo apt install timeshift` |
| NTFS 读写 | ntfs-3g | `sudo apt install ntfs-3g` |

## 📄 许可证

GPL-3.0
