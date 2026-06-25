/**
 * @file rkmon.c
 * @brief rkmon 系统状态采集函数实现。
 */

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
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
        printf("%-*s: unavailable\n", LABEL_WIDTH, "hostname");
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
        printf("%-*s: unavailable\n", LABEL_WIDTH, "soc_temp");
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
        printf("%-*s: unavailable\n", LABEL_WIDTH, "uptime");
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
        printf("%-*s: unavailable\n", LABEL_WIDTH, "loadavg");
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
        printf("%-*s: unavailable\n", LABEL_WIDTH, "memory");
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

    if (ferror(fp)) {
        fclose(fp);
        printf("%-*s: unavailable\n", LABEL_WIDTH, "memory");
        return;
    }

    fclose(fp);

    if (total_kb == 0 || available_kb == 0 || available_kb > total_kb) {
        printf("%-*s: unavailable\n", LABEL_WIDTH, "memory");
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
        printf("%-*s: unavailable\n", LABEL_WIDTH, "disk_root");
        return;
    }

    block_size = fs.f_frsize;
    if (block_size == 0) {
        printf("%-*s: unavailable\n", LABEL_WIDTH, "disk_root");
        return;
    }

    total_bytes = (unsigned long long)fs.f_blocks * block_size;
    used_bytes = (unsigned long long)(fs.f_blocks - fs.f_bfree) * block_size;
    percent = used_bytes * 100.0 / total_bytes;

    printf("%-*s: used %llu MB / total %llu MB (%.1f%%)\n",
           LABEL_WIDTH, "disk_root", used_bytes / (1024 * 1024),
           total_bytes / (1024 * 1024), percent);
}

/**
 * @brief 读取 /sys/class/net/wlan0/operstate，并打印 wlan0 接口状态。
 */
void print_wlan0_state(void)
{
    char state[BUF_SIZE];

    if (read_first_line("/sys/class/net/wlan0/operstate",
                        state, sizeof(state)) != 0) {
        printf("%-*s: unavailable\n", LABEL_WIDTH, "wlan0_state");
        return;
    }

    printf("%-*s: %s\n", LABEL_WIDTH, "wlan0_state", state);
}

/**
 * @brief 使用 getifaddrs() 获取并打印 wlan0 的 IPv4 地址。
 */
void print_wlan0_ip(void)
{
    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;
    char ip[INET_ADDRSTRLEN];
    int found = 0;

    if (getifaddrs(&ifaddr) != 0) {
        printf("%-*s: unavailable\n", LABEL_WIDTH, "wlan0_ip");
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        struct sockaddr_in *addr;

        if (ifa->ifa_addr == NULL) {
            continue;
        }

        if (strcmp(ifa->ifa_name, "wlan0") != 0 ||
            ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        addr = (struct sockaddr_in *)ifa->ifa_addr;
        if (inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip)) == NULL) {
            continue;
        }

        printf("%-*s: %s\n", LABEL_WIDTH, "wlan0_ip", ip);
        found = 1;
        break;
    }

    freeifaddrs(ifaddr);

    if (!found) {
        printf("%-*s: unavailable\n", LABEL_WIDTH, "wlan0_ip");
    }
}

/**
 * @brief 解析 /proc/net/route，并打印默认网关及其网络接口。
 */
void print_gateway(void)
{
    FILE *fp = fopen("/proc/net/route", "r");
    char line[BUF_SIZE];
    int found = 0;

    if (fp == NULL || fgets(line, sizeof(line), fp) == NULL) {
        if (fp != NULL) {
            fclose(fp);
        }
        printf("%-*s: unavailable\n", LABEL_WIDTH, "gateway");
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char iface[32];
        unsigned int destination;
        unsigned int gateway;

        if (sscanf(line, "%31s %x %x",
                   iface, &destination, &gateway) != 3) {
            continue;
        }

        if (destination == 0) {
            unsigned char b1 = gateway & 0xff;
            unsigned char b2 = (gateway >> 8) & 0xff;
            unsigned char b3 = (gateway >> 16) & 0xff;
            unsigned char b4 = (gateway >> 24) & 0xff;

            printf("%-*s: %u.%u.%u.%u dev %s\n",
                   LABEL_WIDTH, "gateway",
                   (unsigned int)b1, (unsigned int)b2,
                   (unsigned int)b3, (unsigned int)b4, iface);
            found = 1;
            break;
        }
    }

    if (ferror(fp)) {
        found = 0;
    }

    fclose(fp);

    if (!found) {
        printf("%-*s: unavailable\n", LABEL_WIDTH, "gateway");
    }
}

/**
 * @brief 打印 rkmon 命令行帮助信息。
 */
void print_help(void)
{
    printf("rkmon - RK3568 board status monitor\n\n");
    printf("Usage:\n");
    printf("rkmon [option]\n\n");
    printf("rkmon --watch N\n\n");
    printf("Options:\n");
    printf("--help       Show this help message\n");
    printf("--version    Show version information\n");
    printf("--system     Show system information\n");
    printf("--network    Show network information\n");
    printf("--temp       Show SoC temperature\n");
    printf("--memory     Show memory usage\n");
    printf("--disk       Show root disk usage\n");
    printf("--watch N    Refresh all status every N seconds\n\n");
    printf("Default:\n");
    printf("Show all status information once\n\n");
    printf("Watch:\n");
    printf("N must be an integer from 1 to 3600\n");
    printf("Press Ctrl+C to stop\n");
}

/**
 * @brief 打印 rkmon 程序版本。
 */
void print_version(void)
{
    printf("rkmon %s\n", RKMON_VERSION);
}
