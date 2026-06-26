/**
 * @file collect.h
 * @brief rkmon 状态采集接口声明。
 */

#ifndef COLLECT_H
#define COLLECT_H

#include "rkmon.h"

void rkmon_status_init(RkmonStatus *status);
int rkmon_collect(RkmonStatus *status);

#endif
