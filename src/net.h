/**
 * @file net.h
 * @brief rkmon 网络发送接口声明。
 */

#ifndef NET_H
#define NET_H

int rkmon_send_udp_report(const char *ip,
                          unsigned short port,
                          const char *json);

#endif
