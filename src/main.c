/**
 * @file main.c
 * @brief rkmon 程序入口和状态采集主流程。
 */

#include <stdio.h>
#include <string.h>

#include "rkmon.h"

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
 * @brief 处理简单命令行参数，或依次输出各项系统状态。
 *
 * @param argc 命令行参数数量。
 * @param argv 命令行参数数组。
 * @return 程序正常结束时返回 0，参数错误时返回 1。
 */
int main(int argc, char *argv[])
{
    if (argc == 1) {
        print_all_info();
        return 0;
    }

    if (argc > 2) {
        fprintf(stderr, "too many arguments\n");
        fprintf(stderr, "try: rkmon --help\n");
        return 1;
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

    fprintf(stderr, "unknown option: %s\n", argv[1]);
    fprintf(stderr, "try: rkmon --help\n");
    return 1;
}
