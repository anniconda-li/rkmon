/**
 * @file output.h
 * @brief rkmon 文本和 JSON 输出接口声明。
 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include <stddef.h>

#include "rkmon.h"

void rkmon_print_header(void);
void rkmon_print_text(const RkmonStatus *status);
void rkmon_print_system(const RkmonStatus *status);
void rkmon_print_network(const RkmonStatus *status);
void rkmon_print_temp(const RkmonStatus *status);
void rkmon_print_memory(const RkmonStatus *status);
void rkmon_print_disk(const RkmonStatus *status);

int rkmon_build_json(const RkmonStatus *status, char *buf, size_t size);
void rkmon_print_json(const RkmonStatus *status);

void rkmon_print_help(void);
void rkmon_print_version(void);

#endif
