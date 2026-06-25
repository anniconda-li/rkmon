/**
 * @file main.c
 * @brief rkmon 程序入口和状态采集主流程。
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rkmon.h"

#define WATCH_MAX_INTERVAL 3600

/**
 * @brief 输出完整系统状态。
 */
static void print_all_info(void)
{
    print_version();
    printf("====================\n");

    print_hostname();
    print_soc_temp();
    print_uptime();
    print_loadavg();
    print_memory();
    print_disk_root();
    print_wlan0_state();
    print_wlan0_ip();
    print_gateway();
}

/**
 * @brief 输出系统基础信息。
 */
static void print_system_info(void)
{
    print_hostname();
    print_uptime();
    print_loadavg();
}

/**
 * @brief 输出网络状态信息。
 */
static void print_network_info(void)
{
    print_wlan0_state();
    print_wlan0_ip();
    print_gateway();
}

/**
 * @brief 解析 --watch 的刷新间隔。
 *
 * @param s 待解析的字符串。
 * @param interval 保存解析后的秒数。
 * @return 解析成功返回 0，解析失败返回 -1。
 */
static int parse_watch_interval(const char *s, unsigned int *interval)
{
    char *endptr = NULL;
    long value;

    if (s == NULL || interval == NULL || s[0] == '\0') {
        return -1;
    }

    errno = 0;
    value = strtol(s, &endptr, 10);

    if (errno != 0 || endptr == s || *endptr != '\0') {
        return -1;
    }

    if (value <= 0 || value > WATCH_MAX_INTERVAL) {
        return -1;
    }

    if (value == LONG_MIN || value == LONG_MAX) {
        return -1;
    }

    *interval = (unsigned int)value;
    return 0;
}

/**
 * @brief 清屏并按指定间隔持续刷新完整状态。
 *
 * @param interval 刷新间隔，单位为秒。
 */
static void watch_all_info(unsigned int interval)
{
    printf("\033[2J\033[H");

    while (1) {
        printf("\033[H");
        print_all_info();
        fflush(stdout);
        sleep(interval);
    }
}

/**
 * @brief 处理简单命令行参数，或依次输出各项系统状态。
 *
 * @param argc 命令行参数数量。
 * @param argv 命令行参数数组。
 * @return 程序正常结束时返回 0，参数错误时返回 1。
 */
int main(int argc, char *argv[])
{
    unsigned int interval;

    if (argc == 1) {
        print_all_info();
        return 0;
    }

    if (argc > 3) {
        fprintf(stderr, "too many arguments\n");
        fprintf(stderr, "try: rkmon --help\n");
        return 1;
    }

    if (argc == 3) {
        if (strcmp(argv[1], "--watch") != 0) {
            fprintf(stderr, "unknown option combination\n");
            fprintf(stderr, "try: rkmon --help\n");
            return 1;
        }

        if (parse_watch_interval(argv[2], &interval) != 0) {
            fprintf(stderr, "invalid watch interval: %s\n", argv[2]);
            fprintf(stderr, "try: rkmon --help\n");
            return 1;
        }

        watch_all_info(interval);
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0) {
        print_version();
        return 0;
    }

    if (strcmp(argv[1], "--system") == 0) {
        print_system_info();
        return 0;
    }

    if (strcmp(argv[1], "--network") == 0) {
        print_network_info();
        return 0;
    }

    if (strcmp(argv[1], "--temp") == 0) {
        print_soc_temp();
        return 0;
    }

    if (strcmp(argv[1], "--memory") == 0) {
        print_memory();
        return 0;
    }

    if (strcmp(argv[1], "--disk") == 0) {
        print_disk_root();
        return 0;
    }

    if (strcmp(argv[1], "--json") == 0) {
        print_json();
        return 0;
    }

    if (strcmp(argv[1], "--watch") == 0) {
        fprintf(stderr, "missing watch interval\n");
        fprintf(stderr, "usage: rkmon --watch N\n");
        return 1;
    }

    fprintf(stderr, "unknown option: %s\n", argv[1]);
    fprintf(stderr, "try: rkmon --help\n");
    return 1;
}
