/**
 * @file rkmon.c
 * @brief rkmon 系统状态采集函数实现。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rkmon.h"

#define BUF_SIZE 256

/**
 * @brief 读取文本文件第一行，并移除行尾换行符。
 *
 * @param path 待读取文件的路径。
 * @param buf 保存读取结果的缓冲区。
 * @param size 缓冲区大小，单位为字节。
 * @return 读取成功返回 0，打开或读取失败返回 -1。
 */
static int read_first_line(const char *path, char *buf, size_t size)
{
    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        return -1;
    }

    if (fgets(buf, size, fp) == NULL) {
        fclose(fp);
        return -1;
    }

    buf[strcspn(buf, "\r\n")] = '\0';
    fclose(fp);

    return 0;
}

/**
 * @brief 读取 /etc/hostname，并打印开发板主机名。
 */
void print_hostname(void)
{
    char buf[BUF_SIZE];

    if (read_first_line("/etc/hostname", buf, sizeof(buf)) != 0) {
        printf("hostname : N/A\n");
        return;
    }

    printf("hostname : %s\n", buf);
}

/**
 * @brief 读取 thermal_zone0/temp，并将毫摄氏度转换为摄氏度后打印。
 */
void print_cpu_temp(void)
{
    char buf[BUF_SIZE];
    int temp_milli;

    if (read_first_line("/sys/class/thermal/thermal_zone0/temp",
                        buf, sizeof(buf)) != 0) {
        printf("cpu_temp : N/A\n");
        return;
    }

    temp_milli = atoi(buf);
    printf("cpu_temp : %.1f C\n", temp_milli / 1000.0);
}

/**
 * @brief 读取 /proc/uptime 第一列，并格式化为 days HH:MM:SS。
 */
void print_uptime(void)
{
    FILE *fp = fopen("/proc/uptime", "r");
    double uptime;
    long long total_seconds;
    long long days;
    long long hours;
    long long minutes;
    long long seconds;

    if (fp == NULL || fscanf(fp, "%lf", &uptime) != 1) {
        if (fp != NULL) {
            fclose(fp);
        }
        printf("uptime   : N/A\n");
        return;
    }

    fclose(fp);

    total_seconds = (long long)uptime;
    days = total_seconds / 86400;
    hours = (total_seconds % 86400) / 3600;
    minutes = (total_seconds % 3600) / 60;
    seconds = total_seconds % 60;

    printf("uptime   : %lld days %02lld:%02lld:%02lld\n",
           days, hours, minutes, seconds);
}

/**
 * @brief 读取并打印 /proc/loadavg 的完整第一行。
 */
void print_loadavg(void)
{
    char buf[BUF_SIZE];

    if (read_first_line("/proc/loadavg", buf, sizeof(buf)) != 0) {
        printf("loadavg  : N/A\n");
        return;
    }

    printf("loadavg  : %s\n", buf);
}

/**
 * @brief 解析 /proc/meminfo 中的 MemTotal 和 MemAvailable。
 *
 * 使用 MemTotal 减去 MemAvailable 计算已用内存，并以 MB 为单位打印。
 */
void print_memory(void)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    char line[BUF_SIZE];
    unsigned long total_kb = 0;
    unsigned long available_kb = 0;

    if (fp == NULL) {
        printf("memory   : N/A\n");
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "MemTotal: %lu kB", &total_kb) == 1) {
            continue;
        }

        if (sscanf(line, "MemAvailable: %lu kB", &available_kb) == 1) {
            continue;
        }
    }

    fclose(fp);

    if (total_kb == 0 || available_kb == 0 || available_kb > total_kb) {
        printf("memory   : N/A\n");
        return;
    }

    printf("memory   : %lu MB / %lu MB\n",
           (total_kb - available_kb) / 1024, total_kb / 1024);
}
