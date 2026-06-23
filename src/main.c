/**
 * @file main.c
 * @brief rkmon 程序入口和状态采集主流程。
 */

#include <stdio.h>
#include <string.h>

#include "rkmon.h"

/**
 * @brief 处理简单命令行参数，或依次输出各项系统状态。
 *
 * @param argc 命令行参数数量。
 * @param argv 命令行参数数组。
 * @return 程序正常结束时返回 0。
 */
int main(int argc, char *argv[])
{
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            print_help();
            return 0;
        }

        if (strcmp(argv[1], "--version") == 0) {
            print_version();
            return 0;
        }

        fprintf(stderr, "rkmon: unknown option: %s\n", argv[1]);
        fprintf(stderr, "try: rkmon --help\n");
        return 1;
    }

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

    return 0;
}
