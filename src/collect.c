/**
 * @file collect.c
 * @brief rkmon 状态采集函数实现。
 */

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/statvfs.h>

#include "collect.h"

#define BUF_SIZE 256

/**
 * @brief 读取文本文件第一行，并移除行尾换行符。
 */
static int read_first_line(const char *path, char *buf, size_t size)
{
    FILE *fp;

    if (path == NULL || buf == NULL || size == 0) {
        return -1;
    }

    fp = fopen(path, "r");
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

/** @brief 读取 /etc/hostname，获取开发板主机名。 */
static int get_hostname(char *buf, size_t size)
{
    return read_first_line("/etc/hostname", buf, size);
}

/** @brief 读取 thermal_zone0/temp，获取 SoC 摄氏温度。 */
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

/** @brief 读取 /proc/uptime 第一列，获取系统运行秒数。 */
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

/** @brief 解析 /proc/loadavg，获取负载、任务数量和最近创建的 PID。 */
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

/** @brief 解析 /proc/meminfo，获取内存使用量、总量和使用率。 */
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

    *used_mb = (total_kb - available_kb) / 1024;
    *total_mb = total_kb / 1024;
    *percent = (total_kb - available_kb) * 100.0 / total_kb;
    return 0;
}

/** @brief 使用 statvfs("/") 获取根分区使用量、总量和使用率。 */
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

    *used_mb = (unsigned long)(used_bytes / (1024 * 1024));
    *total_mb = (unsigned long)(total_bytes / (1024 * 1024));
    *percent = used_bytes * 100.0 / total_bytes;
    return 0;
}

/** @brief 读取 /sys/class/net/wlan0/operstate，获取 wlan0 状态。 */
static int get_wlan0_state(char *buf, size_t size)
{
    return read_first_line("/sys/class/net/wlan0/operstate", buf, size);
}

/** @brief 使用 getifaddrs() 获取 wlan0 的 IPv4 地址。 */
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

/** @brief 解析 /proc/net/route，获取默认网关和接口名。 */
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

void rkmon_status_init(RkmonStatus *status)
{
    if (status == NULL) {
        return;
    }

    memset(status, 0, sizeof(*status));
}

int rkmon_collect(RkmonStatus *status)
{
    if (status == NULL) {
        return -1;
    }

    rkmon_status_init(status);

    status->has_hostname =
        (get_hostname(status->hostname, sizeof(status->hostname)) == 0);
    status->has_soc_temp = (get_soc_temp(&status->soc_temp_c) == 0);
    status->has_uptime = (get_uptime(&status->uptime_sec) == 0);
    status->has_loadavg =
        (get_loadavg(&status->load1, &status->load5, &status->load15,
                     &status->running_tasks, &status->total_tasks,
                     &status->last_pid) == 0);
    status->has_memory =
        (get_memory(&status->mem_used_mb, &status->mem_total_mb,
                    &status->mem_used_percent) == 0);
    status->has_disk =
        (get_disk_root(&status->disk_used_mb, &status->disk_total_mb,
                       &status->disk_used_percent) == 0);
    status->has_wlan0_state =
        (get_wlan0_state(status->wlan0_state,
                         sizeof(status->wlan0_state)) == 0);
    status->has_wlan0_ip =
        (get_wlan0_ip(status->wlan0_ip, sizeof(status->wlan0_ip)) == 0);
    status->has_gateway =
        (get_gateway(status->gateway, sizeof(status->gateway),
                     status->gateway_iface,
                     sizeof(status->gateway_iface)) == 0);

    return 0;
}
