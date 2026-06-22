/**
 * @file rkmon.c
 * @brief rkmon 系统状态采集函数实现。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>

#include "rkmon.h"

#define BUF_SIZE 256
#define LABEL_WIDTH 12

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
        printf("%-*s: N/A\n", LABEL_WIDTH, "hostname");
        return;
    }

    printf("%-*s: %s\n", LABEL_WIDTH, "hostname", buf);
}

/**
 * @brief 读取 soc-thermal 对应的 thermal_zone0/temp，并打印 SoC 温度。
 */
void print_soc_temp(void)
{
    char buf[BUF_SIZE];
    int temp_milli;

    if (read_first_line("/sys/class/thermal/thermal_zone0/temp",
                        buf, sizeof(buf)) != 0) {
        printf("%-*s: N/A\n", LABEL_WIDTH, "soc_temp");
        return;
    }

    temp_milli = atoi(buf);
    printf("%-*s: %.1f C\n", LABEL_WIDTH, "soc_temp",
           temp_milli / 1000.0);
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
        printf("%-*s: N/A\n", LABEL_WIDTH, "uptime");
        return;
    }

    fclose(fp);

    total_seconds = (long long)uptime;
    days = total_seconds / 86400;
    hours = (total_seconds % 86400) / 3600;
    minutes = (total_seconds % 3600) / 60;
    seconds = total_seconds % 60;

    printf("%-*s: %lld days %02lld:%02lld:%02lld\n",
           LABEL_WIDTH, "uptime", days, hours, minutes, seconds);
}

/**
 * @brief 解析 /proc/loadavg 中的负载、任务数量和最近创建的 PID。
 */
void print_loadavg(void)
{
    FILE *fp = fopen("/proc/loadavg", "r");
    double load1;
    double load5;
    double load15;
    unsigned int running_tasks;
    unsigned int total_tasks;
    unsigned int last_pid;

    if (fp == NULL || fscanf(fp, "%lf %lf %lf %u/%u %u",
                             &load1, &load5, &load15,
                             &running_tasks, &total_tasks,
                             &last_pid) != 6) {
        if (fp != NULL) {
            fclose(fp);
        }
        printf("%-*s: N/A\n", LABEL_WIDTH, "loadavg");
        return;
    }

    fclose(fp);

    printf("%-*s: 1min    5min    15min\n", LABEL_WIDTH, "loadavg");
    printf("%*s%.2f    %.2f    %.2f\n",
           LABEL_WIDTH + 2, "", load1, load5, load15);
    printf("%-*s: running %u / total %u\n",
           LABEL_WIDTH, "tasks", running_tasks, total_tasks);
    printf("%-*s: %u\n", LABEL_WIDTH, "last_pid", last_pid);
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
        printf("%-*s: N/A\n", LABEL_WIDTH, "memory");
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
        printf("%-*s: N/A\n", LABEL_WIDTH, "memory");
        return;
    }

    {
        unsigned long used_kb = total_kb - available_kb;
        double percent = used_kb * 100.0 / total_kb;

        printf("%-*s: used %lu MB / total %lu MB (%.1f%%)\n",
               LABEL_WIDTH, "memory", used_kb / 1024,
               total_kb / 1024, percent);
    }
}

/**
 * @brief 使用 statvfs("/") 获取并打印根分区使用情况。
 */
void print_disk_root(void)
{
    struct statvfs fs;
    unsigned long long block_size;
    unsigned long long total_bytes;
    unsigned long long used_bytes;
    double percent;

    if (statvfs("/", &fs) != 0 || fs.f_blocks == 0) {
        printf("%-*s: N/A\n", LABEL_WIDTH, "disk_root");
        return;
    }

    block_size = fs.f_frsize;
    total_bytes = (unsigned long long)fs.f_blocks * block_size;
    used_bytes = (unsigned long long)(fs.f_blocks - fs.f_bfree) * block_size;
    percent = used_bytes * 100.0 / total_bytes;

    printf("%-*s: used %llu MB / total %llu MB (%.1f%%)\n",
           LABEL_WIDTH, "disk_root", used_bytes / (1024 * 1024),
           total_bytes / (1024 * 1024), percent);
}
