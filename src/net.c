/**
 * @file net.c
 * @brief rkmon 网络发送函数实现。
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "net.h"

int rkmon_send_udp_report(const char *ip,
                          unsigned short port,
                          const char *json)
{
    int sockfd;
    ssize_t sent;
    struct sockaddr_in addr;
    size_t json_len;

    if (ip == NULL || json == NULL || port == 0) {
        return -1;
    }

    json_len = strlen(json);
    if (json_len == 0) {
        return -1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }

    sent = sendto(sockfd, json, json_len, 0,
                  (struct sockaddr *)&addr, sizeof(addr));
    if (sent < 0) {
        perror("sendto");
        close(sockfd);
        return -1;
    }

    close(sockfd);
    return (int)sent;
}
