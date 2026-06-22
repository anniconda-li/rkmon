# rkmon

`rkmon` 是一个面向 RK3568 Linux 开发板的轻量级系统状态监控工具，使用 C 语言编写。

当前版本可以输出：

- 程序版本
- 系统主机名
- CPU 温度
- 系统运行时间
- 系统平均负载
- 内存使用情况

## 环境要求

- Linux（当前在 RK3568 / ARM64 开发板上使用）
- GCC
- Make

在 Debian 或 Ubuntu 系统中可以安装构建工具：

```bash
sudo apt update
sudo apt install build-essential
```

## 编译

在项目根目录运行：

```bash
make
```

编译完成后会在项目根目录生成 `rkmon` 可执行文件。

## 运行

```bash
./rkmon
```

输出示例：

```text
rkmon v0.2
====================
hostname : rk3568
cpu_temp : 45.2 C
uptime   : 0 days 02:56:10
loadavg  : 0.03 0.01 0.00 1/353 3465
memory   : 479 MB / 3902 MB
```

## 清理

```bash
make clean
```

## 项目结构

```text
.
|-- Makefile
|-- README.md
|-- src/
|   |-- main.c
|   |-- rkmon.c
|   `-- rkmon.h
`-- LICENSE
```

## 系统状态数据说明

- 主机名从 `/etc/hostname` 读取。
- CPU 温度从 `/sys/class/thermal/thermal_zone0/temp` 读取。不同内核或设备树配置可能使用其他 thermal zone。
- 系统运行时间从 `/proc/uptime` 读取，并显示为 `days HH:MM:SS`。
- 系统平均负载从 `/proc/loadavg` 读取，当前版本直接打印完整一行。
- 内存信息从 `/proc/meminfo` 读取，通过 `MemTotal - MemAvailable` 计算已用内存，并以 MB 为单位显示。

读取对应文件失败或数据无效时，程序会为该项目显示 `N/A`。

## 许可证

本项目使用 MIT License，详情见 [LICENSE](LICENSE)。
