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
 * @brief 读取 /etc/hostname，获取开发板主机名。
 */
static int get_hostname(char *buf, size_t size)
{
    return read_first_line("/etc/hostname", buf, size);
}

/**
 * @brief 读取 thermal_zone0/temp，获取 SoC 摄氏温度。
 */
static int get_soc_temp(double *temp_c)
{
    FILE *fp;
    int temp_milli;

    if (temp_c == NULL) {
        return -1;
    }

    fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fp == NULL) {
        return -1;
    }

    if (fscanf(fp, "%d", &temp_milli) != 1) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    *temp_c = temp_milli / 1000.0;
    return 0;
}

/**
 * @brief 读取 /proc/uptime 第一列，获取系统运行秒数。
 */
static int get_uptime(unsigned long *seconds)
{
    FILE *fp;
    double uptime;

    if (seconds == NULL) {
        return -1;
    }

    fp = fopen("/proc/uptime", "r");
    if (fp == NULL) {
        return -1;
    }

    if (fscanf(fp, "%lf", &uptime) != 1 || uptime < 0) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    *seconds = (unsigned long)uptime;
    return 0;
}

/**
 * @brief 解析 /proc/loadavg，获取负载、任务数量和最近创建的 PID。
 */
static int get_loadavg(double *load1,
                       double *load5,
                       double *load15,
                       unsigned int *running_tasks,
                       unsigned int *total_tasks,
                       unsigned int *last_pid)
{
    FILE *fp;

    if (load1 == NULL || load5 == NULL || load15 == NULL ||
        running_tasks == NULL || total_tasks == NULL ||
        last_pid == NULL) {
        return -1;
    }

    fp = fopen("/proc/loadavg", "r");
    if (fp == NULL) {
        return -1;
    }

    if (fscanf(fp, "%lf %lf %lf %u/%u %u",
               load1, load5, load15,
               running_tasks, total_tasks, last_pid) != 6) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

/**
 * @brief 解析 /proc/meminfo，获取内存使用量、总量和使用率。
 */
static int get_memory(unsigned long *used_mb,
                      unsigned long *total_mb,
                      double *percent)
{
    FILE *fp;
    char line[BUF_SIZE];
    unsigned long total_kb = 0;
    unsigned long available_kb = 0;

    if (used_mb == NULL || total_mb == NULL || percent == NULL) {
        return -1;
    }

    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        return -1;
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
        return -1;
    }

    fclose(fp);

    if (total_kb == 0 || available_kb == 0 || available_kb > total_kb) {
        return -1;
    }

    *total_mb = total_kb / 1024;
    *used_mb = (total_kb - available_kb) / 1024;

    if (*total_mb == 0) {
        return -1;
    }

    *percent = (total_kb - available_kb) * 100.0 / total_kb;
    return 0;
}

/**
 * @brief 使用 statvfs("/") 获取根分区使用量、总量和使用率。
 */
static int get_disk_root(unsigned long *used_mb,
                         unsigned long *total_mb,
                         double *percent)
{
    struct statvfs fs;
    unsigned long long block_size;
    unsigned long long total_bytes;
    unsigned long long used_bytes;

    if (used_mb == NULL || total_mb == NULL || percent == NULL) {
        return -1;
    }

    if (statvfs("/", &fs) != 0 || fs.f_blocks == 0) {
        return -1;
    }

    block_size = fs.f_frsize;
    if (block_size == 0) {
        return -1;
    }

    total_bytes = (unsigned long long)fs.f_blocks * block_size;
    used_bytes = (unsigned long long)(fs.f_blocks - fs.f_bfree) * block_size;

    *total_mb = (unsigned long)(total_bytes / (1024 * 1024));
    *used_mb = (unsigned long)(used_bytes / (1024 * 1024));

    if (*total_mb == 0) {
        return -1;
    }

    *percent = used_bytes * 100.0 / total_bytes;
    return 0;
}

/**
 * @brief 读取 /sys/class/net/wlan0/operstate，获取 wlan0 状态。
 */
static int get_wlan0_state(char *buf, size_t size)
{
    return read_first_line("/sys/class/net/wlan0/operstate", buf, size);
}

/**
 * @brief 使用 getifaddrs() 获取 wlan0 的 IPv4 地址。
 */
static int get_wlan0_ip(char *buf, size_t size)
{
    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;
    int ret = -1;

    if (buf == NULL || size == 0) {
        return -1;
    }

    if (getifaddrs(&ifaddr) != 0) {
        return -1;
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
        if (inet_ntop(AF_INET, &addr->sin_addr, buf, size) != NULL) {
            ret = 0;
            break;
        }
    }

    freeifaddrs(ifaddr);
    return ret;
}

/**
 * @brief 解析 /proc/net/route，获取默认网关和接口名。
 */
static int get_gateway(char *gateway_ip,
                       size_t gateway_size,
                       char *iface,
                       size_t iface_size)
{
    FILE *fp;
    char line[BUF_SIZE];

    if (gateway_ip == NULL || gateway_size == 0 ||
        iface == NULL || iface_size == 0) {
        return -1;
    }

    fp = fopen("/proc/net/route", "r");
    if (fp == NULL || fgets(line, sizeof(line), fp) == NULL) {
        if (fp != NULL) {
            fclose(fp);
        }
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char route_iface[32];
        unsigned int destination;
        unsigned int gateway;

        if (sscanf(line, "%31s %x %x",
                   route_iface, &destination, &gateway) != 3) {
            continue;
        }

        if (destination == 0) {
            unsigned char b1 = gateway & 0xff;
            unsigned char b2 = (gateway >> 8) & 0xff;
            unsigned char b3 = (gateway >> 16) & 0xff;
            unsigned char b4 = (gateway >> 24) & 0xff;

            if (snprintf(gateway_ip, gateway_size, "%u.%u.%u.%u",
                         (unsigned int)b1, (unsigned int)b2,
                         (unsigned int)b3, (unsigned int)b4) < 0) {
                fclose(fp);
                return -1;
            }

            if (snprintf(iface, iface_size, "%s", route_iface) < 0) {
                fclose(fp);
                return -1;
            }
            fclose(fp);
            return 0;
        }
    }

    if (ferror(fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return -1;
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
 * @brief 使用 get_xxx() 采集完整状态，并以 JSON 对象格式输出。
 */
void print_json(void)
{
    char hostname[BUF_SIZE];
    double soc_temp_c;
    unsigned long uptime_sec;
    double load1;
    double load5;
    double load15;
    unsigned int running_tasks;
    unsigned int total_tasks;
    unsigned int last_pid;
    unsigned long memory_used_mb;
    unsigned long memory_total_mb;
    double memory_percent;
    unsigned long disk_used_mb;
    unsigned long disk_total_mb;
    double disk_percent;
    char wlan0_state[BUF_SIZE];
    char wlan0_ip[INET_ADDRSTRLEN];
    char gateway_ip[INET_ADDRSTRLEN];
    char gateway_iface[32];
    int has_hostname;
    int has_soc_temp;
    int has_uptime;
    int has_loadavg;
    int has_memory;
    int has_disk;
    int has_wlan0_state;
    int has_wlan0_ip;
    int has_gateway;

    has_hostname = (get_hostname(hostname, sizeof(hostname)) == 0);
    has_soc_temp = (get_soc_temp(&soc_temp_c) == 0);
    has_uptime = (get_uptime(&uptime_sec) == 0);
    has_loadavg = (get_loadavg(&load1, &load5, &load15,
                               &running_tasks, &total_tasks,
                               &last_pid) == 0);
    has_memory = (get_memory(&memory_used_mb, &memory_total_mb,
                             &memory_percent) == 0);
    has_disk = (get_disk_root(&disk_used_mb, &disk_total_mb,
                              &disk_percent) == 0);
    has_wlan0_state = (get_wlan0_state(wlan0_state,
                                       sizeof(wlan0_state)) == 0);
    has_wlan0_ip = (get_wlan0_ip(wlan0_ip, sizeof(wlan0_ip)) == 0);
    has_gateway = (get_gateway(gateway_ip, sizeof(gateway_ip),
                               gateway_iface, sizeof(gateway_iface)) == 0);

    printf("{\n");
    printf("  \"version\": \"%s\",\n", RKMON_VERSION);

    if (has_hostname) {
        printf("  \"hostname\": \"%s\",\n", hostname);
    } else {
        printf("  \"hostname\": null,\n");
    }

    if (has_soc_temp) {
        printf("  \"soc_temp_c\": %.1f,\n", soc_temp_c);
    } else {
        printf("  \"soc_temp_c\": null,\n");
    }

    if (has_uptime) {
        printf("  \"uptime_sec\": %lu,\n", uptime_sec);
    } else {
        printf("  \"uptime_sec\": null,\n");
    }

    if (has_loadavg) {
        printf("  \"loadavg\": {\n");
        printf("    \"1min\": %.2f,\n", load1);
        printf("    \"5min\": %.2f,\n", load5);
        printf("    \"15min\": %.2f\n", load15);
        printf("  },\n");
        printf("  \"tasks\": {\n");
        printf("    \"running\": %u,\n", running_tasks);
        printf("    \"total\": %u\n", total_tasks);
        printf("  },\n");
        printf("  \"last_pid\": %u,\n", last_pid);
    } else {
        printf("  \"loadavg\": null,\n");
        printf("  \"tasks\": null,\n");
        printf("  \"last_pid\": null,\n");
    }

    if (has_memory) {
        printf("  \"memory\": {\n");
        printf("    \"used_mb\": %lu,\n", memory_used_mb);
        printf("    \"total_mb\": %lu,\n", memory_total_mb);
        printf("    \"used_percent\": %.1f\n", memory_percent);
        printf("  },\n");
    } else {
        printf("  \"memory\": null,\n");
    }

    if (has_disk) {
        printf("  \"disk_root\": {\n");
        printf("    \"used_mb\": %lu,\n", disk_used_mb);
        printf("    \"total_mb\": %lu,\n", disk_total_mb);
        printf("    \"used_percent\": %.1f\n", disk_percent);
        printf("  },\n");
    } else {
        printf("  \"disk_root\": null,\n");
    }

    printf("  \"network\": {\n");
    if (has_wlan0_state) {
        printf("    \"wlan0_state\": \"%s\",\n", wlan0_state);
    } else {
        printf("    \"wlan0_state\": null,\n");
    }

    if (has_wlan0_ip) {
        printf("    \"wlan0_ip\": \"%s\",\n", wlan0_ip);
    } else {
        printf("    \"wlan0_ip\": null,\n");
    }

    if (has_gateway) {
        printf("    \"gateway\": \"%s\",\n", gateway_ip);
        printf("    \"gateway_iface\": \"%s\"\n", gateway_iface);
    } else {
        printf("    \"gateway\": null,\n");
        printf("    \"gateway_iface\": null\n");
    }
    printf("  }\n");
    printf("}\n");
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
    printf("--json       Show all status in JSON format\n");
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
