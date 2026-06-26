/**
 * @file rkmon.h
 * @brief rkmon 公共宏和状态结构体定义。
 */

#ifndef RKMON_H
#define RKMON_H

/** @brief rkmon 当前版本号。 */
#define RKMON_VERSION "v0.9"

/** @brief 文本输出字段名宽度。 */
#define LABEL_WIDTH 12

/**
 * @brief rkmon 一次完整采集得到的状态数据。
 */
typedef struct {
    char hostname[64];
    int has_hostname;

    double soc_temp_c;
    int has_soc_temp;

    unsigned long uptime_sec;
    int has_uptime;

    double load1;
    double load5;
    double load15;
    unsigned int running_tasks;
    unsigned int total_tasks;
    unsigned int last_pid;
    int has_loadavg;

    unsigned long mem_used_mb;
    unsigned long mem_total_mb;
    double mem_used_percent;
    int has_memory;

    unsigned long disk_used_mb;
    unsigned long disk_total_mb;
    double disk_used_percent;
    int has_disk;

    char wlan0_state[32];
    int has_wlan0_state;

    char wlan0_ip[64];
    int has_wlan0_ip;

    char gateway[64];
    char gateway_iface[32];
    int has_gateway;
} RkmonStatus;

#endif
