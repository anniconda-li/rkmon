# rkmon

`rkmon` 是一个面向 RK3568 Linux 开发板的轻量级系统状态监控工具，使用 C 语言编写。

当前版本可以输出：

- 程序版本
- 系统主机名
- SoC 温度
- 系统运行时间
- 系统平均负载
- 内存使用情况
- 根分区使用情况
- wlan0 接口状态
- wlan0 IPv4 地址
- 默认网关及其网络接口
- `--help` 帮助参数
- `--version` 版本参数
- `--system` 系统基础信息查询参数
- `--network` 网络信息查询参数
- `--temp` 温度查询参数
- `--memory` 内存查询参数
- `--disk` 根分区磁盘查询参数
- `--json` JSON 完整状态输出
- `--watch N` 清屏刷新完整状态
- `--udp <ip> <port>` UDP JSON 上报

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
./rkmon --help
./rkmon --version
./rkmon --system
./rkmon --network
./rkmon --temp
./rkmon --memory
./rkmon --disk
./rkmon --json
./rkmon --watch 1
./rkmon --watch 2
./rkmon --watch 5
./rkmon --udp 192.168.3.100 9000
```

输出示例：

```text
rkmon v0.9
====================
hostname    : rk3568
soc_temp    : 50.0 C
uptime      : 0 days 05:25:52
loadavg     : 1min    5min    15min
              0.00    0.00    0.00
tasks       : running 3 / total 349
last_pid    : 4174
memory      : used 477 MB / total 3902 MB (12.2%)
disk_root   : used 2300 MB / total 14500 MB (15.8%)
wlan0_state : up
wlan0_ip    : 192.168.3.216
gateway     : 192.168.3.1 dev wlan0
```

查看帮助：

```bash
./rkmon --help
```

查看版本：

```bash
./rkmon --version
```

单项或分组查询：

```bash
./rkmon --system
./rkmon --network
./rkmon --temp
./rkmon --memory
./rkmon --disk
```

JSON 输出：

```bash
./rkmon --json
```

`--json` 会输出完整状态的 JSON 对象，方便后续脚本处理、日志记录、systemd 服务或 HTTP 上报使用。

UDP JSON 上报：

```bash
./rkmon --udp 192.168.3.100 9000
```

`--udp <ip> <port>` 会采集一次完整状态，生成 JSON 字符串，并通过 UDP 发送到指定 IP 和端口。

在电脑或服务器上监听 UDP：

```bash
nc -ul 9000
```

如果这个命令不可用，可以尝试：

```bash
nc -u -l -p 9000
```

然后在板子上执行：

```bash
./rkmon --udp 192.168.3.100 9000
```

预期电脑或服务器终端收到一段 JSON。

清屏刷新完整状态：

```bash
./rkmon --watch 1
./rkmon --watch 2
./rkmon --watch 5
```

`--watch N` 会每隔 N 秒刷新一次完整状态，进入 watch 模式时清屏一次，后续刷新时只把光标移动到左上角并覆盖旧内容，以减少终端闪烁。按 `Ctrl+C` 退出。

以下参数会返回错误：

```bash
./rkmon --watch 0
./rkmon --watch abc
./rkmon --watch -1
./rkmon --udp
./rkmon --udp 192.168.3.100
./rkmon --udp 192.168.3.100 abc
./rkmon --udp 192.168.3.100 0
./rkmon --udp 192.168.3.100 65536
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
|   |-- rkmon.h
|   |-- collect.c
|   |-- collect.h
|   |-- output.c
|   |-- output.h
|   |-- net.c
|   `-- net.h
`-- LICENSE
```

源码职责划分：

- `src/main.c`：参数解析和主流程。
- `src/rkmon.h`：公共宏和 `RkmonStatus` 状态结构体。
- `src/collect.c`：状态采集。
- `src/collect.h`：状态采集接口。
- `src/output.c`：文本输出和 JSON 输出。
- `src/output.h`：输出接口。
- `src/net.c`：UDP 网络发送。
- `src/net.h`：网络发送接口。

## 系统状态数据说明

- 主机名从 `/etc/hostname` 读取。
- SoC 温度从 `/sys/class/thermal/thermal_zone0/temp` 读取；该 thermal zone 的类型为 `soc-thermal`。不同内核或设备树配置可能使用其他 thermal zone。
- 系统运行时间从 `/proc/uptime` 读取，并显示为 `days HH:MM:SS`。
- 系统平均负载从 `/proc/loadavg` 读取，分别显示 1、5、15 分钟负载、任务数量和最近创建的 PID。
- 内存信息从 `/proc/meminfo` 读取，通过 `MemTotal - MemAvailable` 计算已用内存，并显示容量和使用率。
- 根分区信息通过 `statvfs("/")` 获取，并显示已用容量、总容量和使用率。
- wlan0 接口状态从 `/sys/class/net/wlan0/operstate` 读取。
- wlan0 IPv4 地址通过 `getifaddrs()` 获取。
- 默认网关从 `/proc/net/route` 读取，并将十六进制小端地址转换为 IPv4 地址。

读取对应文件、调用系统接口失败或数据无效时，程序会为该项目显示 `unavailable`。

## 版本历史

### v0.9

- Refactor project structure
- Add `RkmonStatus` data structure
- Split collection, output, and network code
- Keep all v0.8 features unchanged
- Prepare for future HTTP reporting

### v0.8

- Add UDP JSON report
- Add `build_json()` to reuse JSON output
- Add `send_udp_report()`
- Add `--udp <ip> <port>`
- Validate UDP port with `strtol`

### v0.7

- Add `--json` output
- Add internal `get_xxx` data collection helpers
- Keep existing text output
- Use `null` for unavailable JSON fields

### v0.6.1

- Optimize `--watch` refresh mode
- Clear screen only once when entering watch mode
- Move cursor to top-left on each refresh
- Reduce terminal flicker

### v0.6

- Add `--watch N` refresh mode
- Clear screen before each refresh
- Validate watch interval with `strtol`
- Reuse full status output function

### v0.5

- Add single/group query options
- Add `--system`
- Add `--network`
- Add `--temp`
- Add `--memory`
- Add `--disk`
- Centralize version string with `RKMON_VERSION`

### v0.4

- Add `wlan0_state`
- Add `wlan0_ip`
- Add default gateway
- Add `--help`
- Add `--version`

## 许可证

本项目使用 MIT License，详情见 [LICENSE](LICENSE)。
