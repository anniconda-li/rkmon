/**
 * @file rkmon.h
 * @brief rkmon 系统状态采集函数声明。
 */

#ifndef RKMON_H
#define RKMON_H

#include <stddef.h>

/** @brief rkmon 当前版本号。 */
#define RKMON_VERSION "v0.8"

/** @brief 读取并打印系统主机名。 */
void print_hostname(void);

/** @brief 读取并打印 SoC 温度。 */
void print_soc_temp(void);

/** @brief 读取并打印系统运行时间。 */
void print_uptime(void);

/** @brief 读取并打印系统平均负载。 */
void print_loadavg(void);

/** @brief 读取并打印系统内存使用情况。 */
void print_memory(void);

/** @brief 读取并打印根分区使用情况。 */
void print_disk_root(void);

/** @brief 读取并打印 wlan0 接口状态。 */
void print_wlan0_state(void);

/** @brief 获取并打印 wlan0 的 IPv4 地址。 */
void print_wlan0_ip(void);

/** @brief 读取并打印默认网关。 */
void print_gateway(void);

/** @brief 以 JSON 格式打印完整状态。 */
void print_json(void);

/** @brief 构建完整状态 JSON 字符串。 */
int build_json(char *buf, size_t size);

/** @brief 通过 UDP 发送完整状态 JSON。 */
int send_udp_report(const char *ip, unsigned short port);

/** @brief 打印命令行帮助信息。 */
void print_help(void);

/** @brief 打印程序版本。 */
void print_version(void);

#endif
