/**
 * @file rkmon.h
 * @brief rkmon 系统状态采集函数声明。
 */

#ifndef RKMON_H
#define RKMON_H

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

#endif
