/**
 * @file output.c
 * @brief rkmon 文本和 JSON 输出函数实现。
 */

#include <stdarg.h>
#include <stdio.h>

#include "output.h"

/**
 * @brief 安全追加格式化内容到 JSON 缓冲区。
 */
static int append_json(char **pos, size_t *remaining, const char *fmt, ...)
{
    va_list args;
    int written;

    if (pos == NULL || *pos == NULL || remaining == NULL ||
        *remaining == 0 || fmt == NULL) {
        return -1;
    }

    va_start(args, fmt);
    written = vsnprintf(*pos, *remaining, fmt, args);
    va_end(args);

    if (written < 0 || (size_t)written >= *remaining) {
        return -1;
    }

    *pos += written;
    *remaining -= (size_t)written;
    return 0;
}

void rkmon_print_header(void)
{
    printf("rkmon %s\n", RKMON_VERSION);
    printf("====================\n");
}

static void print_hostname_line(const RkmonStatus *status)
{
    if (status != NULL && status->has_hostname) {
        printf("%-*s: %s\n", LABEL_WIDTH, "hostname", status->hostname);
        return;
    }

    printf("%-*s: unavailable\n", LABEL_WIDTH, "hostname");
}

static void print_soc_temp_line(const RkmonStatus *status)
{
    if (status != NULL && status->has_soc_temp) {
        printf("%-*s: %.1f C\n",
               LABEL_WIDTH, "soc_temp", status->soc_temp_c);
        return;
    }

    printf("%-*s: unavailable\n", LABEL_WIDTH, "soc_temp");
}

static void print_uptime_line(const RkmonStatus *status)
{
    unsigned long total_seconds;
    unsigned long days;
    unsigned long hours;
    unsigned long minutes;
    unsigned long seconds;

    if (status == NULL || !status->has_uptime) {
        printf("%-*s: unavailable\n", LABEL_WIDTH, "uptime");
        return;
    }

    total_seconds = status->uptime_sec;
    days = total_seconds / 86400;
    hours = (total_seconds % 86400) / 3600;
    minutes = (total_seconds % 3600) / 60;
    seconds = total_seconds % 60;

    printf("%-*s: %lu days %02lu:%02lu:%02lu\n",
           LABEL_WIDTH, "uptime", days, hours, minutes, seconds);
}

static void print_loadavg_lines(const RkmonStatus *status)
{
    if (status == NULL || !status->has_loadavg) {
        printf("%-*s: unavailable\n", LABEL_WIDTH, "loadavg");
        printf("%-*s: unavailable\n", LABEL_WIDTH, "tasks");
        printf("%-*s: unavailable\n", LABEL_WIDTH, "last_pid");
        return;
    }

    printf("%-*s: 1min    5min    15min\n", LABEL_WIDTH, "loadavg");
    printf("%*s%.2f    %.2f    %.2f\n",
           LABEL_WIDTH + 2, "", status->load1,
           status->load5, status->load15);
    printf("%-*s: running %u / total %u\n",
           LABEL_WIDTH, "tasks", status->running_tasks,
           status->total_tasks);
    printf("%-*s: %u\n", LABEL_WIDTH, "last_pid", status->last_pid);
}

static void print_memory_line(const RkmonStatus *status)
{
    if (status != NULL && status->has_memory) {
        printf("%-*s: used %lu MB / total %lu MB (%.1f%%)\n",
               LABEL_WIDTH, "memory", status->mem_used_mb,
               status->mem_total_mb, status->mem_used_percent);
        return;
    }

    printf("%-*s: unavailable\n", LABEL_WIDTH, "memory");
}

static void print_disk_line(const RkmonStatus *status)
{
    if (status != NULL && status->has_disk) {
        printf("%-*s: used %lu MB / total %lu MB (%.1f%%)\n",
               LABEL_WIDTH, "disk_root", status->disk_used_mb,
               status->disk_total_mb, status->disk_used_percent);
        return;
    }

    printf("%-*s: unavailable\n", LABEL_WIDTH, "disk_root");
}

static void print_wlan0_state_line(const RkmonStatus *status)
{
    if (status != NULL && status->has_wlan0_state) {
        printf("%-*s: %s\n",
               LABEL_WIDTH, "wlan0_state", status->wlan0_state);
        return;
    }

    printf("%-*s: unavailable\n", LABEL_WIDTH, "wlan0_state");
}

static void print_wlan0_ip_line(const RkmonStatus *status)
{
    if (status != NULL && status->has_wlan0_ip) {
        printf("%-*s: %s\n", LABEL_WIDTH, "wlan0_ip", status->wlan0_ip);
        return;
    }

    printf("%-*s: unavailable\n", LABEL_WIDTH, "wlan0_ip");
}

static void print_gateway_line(const RkmonStatus *status)
{
    if (status != NULL && status->has_gateway) {
        printf("%-*s: %s dev %s\n",
               LABEL_WIDTH, "gateway",
               status->gateway, status->gateway_iface);
        return;
    }

    printf("%-*s: unavailable\n", LABEL_WIDTH, "gateway");
}

void rkmon_print_text(const RkmonStatus *status)
{
    rkmon_print_header();
    print_hostname_line(status);
    print_soc_temp_line(status);
    print_uptime_line(status);
    print_loadavg_lines(status);
    print_memory_line(status);
    print_disk_line(status);
    print_wlan0_state_line(status);
    print_wlan0_ip_line(status);
    print_gateway_line(status);
}

void rkmon_print_system(const RkmonStatus *status)
{
    print_hostname_line(status);
    print_uptime_line(status);
    print_loadavg_lines(status);
}

void rkmon_print_network(const RkmonStatus *status)
{
    print_wlan0_state_line(status);
    print_wlan0_ip_line(status);
    print_gateway_line(status);
}

void rkmon_print_temp(const RkmonStatus *status)
{
    print_soc_temp_line(status);
}

void rkmon_print_memory(const RkmonStatus *status)
{
    print_memory_line(status);
}

void rkmon_print_disk(const RkmonStatus *status)
{
    print_disk_line(status);
}

int rkmon_build_json(const RkmonStatus *status, char *buf, size_t size)
{
    char *pos = buf;
    size_t remaining = size;

    if (status == NULL || buf == NULL || size == 0) {
        return -1;
    }

    if (append_json(&pos, &remaining, "{\n") != 0 ||
        append_json(&pos, &remaining,
                    "  \"version\": \"%s\",\n", RKMON_VERSION) != 0) {
        return -1;
    }

    if (status->has_hostname) {
        if (append_json(&pos, &remaining,
                        "  \"hostname\": \"%s\",\n",
                        status->hostname) != 0) {
            return -1;
        }
    } else if (append_json(&pos, &remaining,
                           "  \"hostname\": null,\n") != 0) {
        return -1;
    }

    if (status->has_soc_temp) {
        if (append_json(&pos, &remaining,
                        "  \"soc_temp_c\": %.1f,\n",
                        status->soc_temp_c) != 0) {
            return -1;
        }
    } else if (append_json(&pos, &remaining,
                           "  \"soc_temp_c\": null,\n") != 0) {
        return -1;
    }

    if (status->has_uptime) {
        if (append_json(&pos, &remaining,
                        "  \"uptime_sec\": %lu,\n",
                        status->uptime_sec) != 0) {
            return -1;
        }
    } else if (append_json(&pos, &remaining,
                           "  \"uptime_sec\": null,\n") != 0) {
        return -1;
    }

    if (status->has_loadavg) {
        if (append_json(&pos, &remaining, "  \"loadavg\": {\n") != 0 ||
            append_json(&pos, &remaining,
                        "    \"1min\": %.2f,\n", status->load1) != 0 ||
            append_json(&pos, &remaining,
                        "    \"5min\": %.2f,\n", status->load5) != 0 ||
            append_json(&pos, &remaining,
                        "    \"15min\": %.2f\n", status->load15) != 0 ||
            append_json(&pos, &remaining, "  },\n") != 0 ||
            append_json(&pos, &remaining, "  \"tasks\": {\n") != 0 ||
            append_json(&pos, &remaining,
                        "    \"running\": %u,\n",
                        status->running_tasks) != 0 ||
            append_json(&pos, &remaining,
                        "    \"total\": %u\n",
                        status->total_tasks) != 0 ||
            append_json(&pos, &remaining, "  },\n") != 0 ||
            append_json(&pos, &remaining,
                        "  \"last_pid\": %u,\n",
                        status->last_pid) != 0) {
            return -1;
        }
    } else if (append_json(&pos, &remaining, "  \"loadavg\": null,\n") != 0 ||
               append_json(&pos, &remaining, "  \"tasks\": null,\n") != 0 ||
               append_json(&pos, &remaining, "  \"last_pid\": null,\n") != 0) {
        return -1;
    }

    if (status->has_memory) {
        if (append_json(&pos, &remaining, "  \"memory\": {\n") != 0 ||
            append_json(&pos, &remaining,
                        "    \"used_mb\": %lu,\n",
                        status->mem_used_mb) != 0 ||
            append_json(&pos, &remaining,
                        "    \"total_mb\": %lu,\n",
                        status->mem_total_mb) != 0 ||
            append_json(&pos, &remaining,
                        "    \"used_percent\": %.1f\n",
                        status->mem_used_percent) != 0 ||
            append_json(&pos, &remaining, "  },\n") != 0) {
            return -1;
        }
    } else if (append_json(&pos, &remaining, "  \"memory\": null,\n") != 0) {
        return -1;
    }

    if (status->has_disk) {
        if (append_json(&pos, &remaining, "  \"disk_root\": {\n") != 0 ||
            append_json(&pos, &remaining,
                        "    \"used_mb\": %lu,\n",
                        status->disk_used_mb) != 0 ||
            append_json(&pos, &remaining,
                        "    \"total_mb\": %lu,\n",
                        status->disk_total_mb) != 0 ||
            append_json(&pos, &remaining,
                        "    \"used_percent\": %.1f\n",
                        status->disk_used_percent) != 0 ||
            append_json(&pos, &remaining, "  },\n") != 0) {
            return -1;
        }
    } else if (append_json(&pos, &remaining,
                           "  \"disk_root\": null,\n") != 0) {
        return -1;
    }

    if (append_json(&pos, &remaining, "  \"network\": {\n") != 0) {
        return -1;
    }

    if (status->has_wlan0_state) {
        if (append_json(&pos, &remaining,
                        "    \"wlan0_state\": \"%s\",\n",
                        status->wlan0_state) != 0) {
            return -1;
        }
    } else if (append_json(&pos, &remaining,
                           "    \"wlan0_state\": null,\n") != 0) {
        return -1;
    }

    if (status->has_wlan0_ip) {
        if (append_json(&pos, &remaining,
                        "    \"wlan0_ip\": \"%s\",\n",
                        status->wlan0_ip) != 0) {
            return -1;
        }
    } else if (append_json(&pos, &remaining,
                           "    \"wlan0_ip\": null,\n") != 0) {
        return -1;
    }

    if (status->has_gateway) {
        if (append_json(&pos, &remaining,
                        "    \"gateway\": \"%s\",\n",
                        status->gateway) != 0 ||
            append_json(&pos, &remaining,
                        "    \"gateway_iface\": \"%s\"\n",
                        status->gateway_iface) != 0) {
            return -1;
        }
    } else if (append_json(&pos, &remaining,
                           "    \"gateway\": null,\n") != 0 ||
               append_json(&pos, &remaining,
                           "    \"gateway_iface\": null\n") != 0) {
        return -1;
    }

    if (append_json(&pos, &remaining, "  }\n") != 0 ||
        append_json(&pos, &remaining, "}") != 0) {
        return -1;
    }

    return (int)(pos - buf);
}

void rkmon_print_json(const RkmonStatus *status)
{
    char json[4096];

    if (rkmon_build_json(status, json, sizeof(json)) < 0) {
        fprintf(stderr, "failed to build JSON\n");
        return;
    }

    printf("%s\n", json);
}

void rkmon_print_help(void)
{
    printf("rkmon - RK3568 board status monitor\n\n");
    printf("Usage:\n");
    printf("rkmon [option]\n");
    printf("rkmon --watch N\n");
    printf("rkmon --udp <ip> <port>\n\n");
    printf("Options:\n");
    printf("--help       Show this help message\n");
    printf("--version    Show version information\n");
    printf("--system     Show system information\n");
    printf("--network    Show network information\n");
    printf("--temp       Show SoC temperature\n");
    printf("--memory     Show memory usage\n");
    printf("--disk       Show root disk usage\n");
    printf("--json       Show all status in JSON format\n");
    printf("--watch N    Refresh all status every N seconds\n");
    printf("--udp IP P   Send JSON report to UDP IP:port\n\n");
    printf("Default:\n");
    printf("Show all status information once\n\n");
    printf("Watch:\n");
    printf("N must be an integer from 1 to 3600\n");
    printf("Press Ctrl+C to stop\n\n");
    printf("UDP:\n");
    printf("Example:\n");
    printf("rkmon --udp 192.168.3.100 9000\n");
}

void rkmon_print_version(void)
{
    printf("rkmon %s\n", RKMON_VERSION);
}
