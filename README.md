# Linux Savior Toolbox

一款专为 Linux 桌面用户设计的全能系统维护工具箱。

## 功能特性

### 🖥️ 终端模拟器
- 基于 QTermWidget 的高性能终端
- 多标签页管理
- 12种内置主题（Dracula、Monokai、Solarized 等）
- 自定义字体和透明度
- 快捷命令面板

### 💾 系统备份与恢复
- 桌面配置一键备份/恢复
- Timeshift 系统快照深度集成
- 自动备份计划管理
- 快照浏览与恢复

### 🔧 系统清理
- 缓存清理
- 日志清理
- 孤立包清理
- 磁盘空间分析

### 🎨 GNOME 扩展管理
- 扩展列表管理
- 一键启用/禁用
- 扩展配置备份

### 🔄 Windows 双系统工具箱
- 启动项管理（GRUB配置）
- 分区管理与挂载
- NTFS 读写支持
- 时间同步修复
- 智能双系统检测

### 📊 系统诊断
- 系统信息展示
- 硬件检测
- 性能监控
- 诊断报告生成

## 系统要求

- Qt 6.5+
- Linux 桌面环境（GNOME、KDE、XFCE 等）
- 可选依赖：
  - qtermwidget6（终端模拟器）
  - timeshift（系统快照）
  - ntfs-3g（NTFS读写支持）

## 安装

### Debian/Ubuntu 系列
```bash
sudo dpkg -i linux-savior-toolbox_3.55_amd64.deb
```

### Fedora/RHEL 系列
```bash
sudo rpm -i linux-savior-toolbox-3.55-1.x86_64.rpm
```

## 从源码编译

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 许可证

GPL-3.0
