# rkmon

`rkmon` 是一个面向 RK3568 Linux 开发板的轻量级系统状态监控工具，使用 C 语言编写。

当前版本可以输出：

- 程序版本
- 系统主机名
- CPU 温度

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
rkmon v0.1
====================
hostname : rk3568
cpu_temp : 45.2 C
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
|   `-- main.c
`-- LICENSE
```

## 温度数据说明

程序默认从 `/sys/class/thermal/thermal_zone0/temp` 读取 CPU 温度。不同内核或设备树配置可能使用其他 thermal zone；读取失败时程序会显示 `N/A`。

## 许可证

本项目使用 MIT License，详情见 [LICENSE](LICENSE)。
