/**
 * @file main.c
 * @brief rkmon 程序入口和状态采集主流程。
 */

#include <stdio.h>

#include "rkmon.h"

/**
 * @brief 打印程序版本，并依次输出各项系统状态。
 *
 * @return 程序正常结束时返回 0。
 */
int main(void)
{
    printf("rkmon v0.2\n");
    printf("====================\n");

    print_hostname();
    print_cpu_temp();
    print_uptime();
    print_loadavg();
    print_memory();

    return 0;
}
