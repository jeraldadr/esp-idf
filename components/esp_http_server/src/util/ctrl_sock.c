/*
 * SPDX-FileCopyrightText: 2018-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "ctrl_sock.h"

#if CONFIG_IDF_TARGET_LINUX
#define IPV4_ENABLED      1
#define IPV6_ENABLED      1
#define LOOPBACK_ENABLED  1
#else   // CONFIG_IDF_TARGET_LINUX
#define IPV4_ENABLED      CONFIG_LWIP_IPV4
#define IPV6_ENABLED      CONFIG_LWIP_IPV6
#define LOOPBACK_ENABLED  CONFIG_LWIP_NETIF_LOOPBACK
#endif  // !CONFIG_IDF_TARGET_LINUX

/* Control socket, because in some network stacks select can't be woken up any
 * other way
 */
int cs_create_ctrl_sock(int port)
{
#if !LOOPBACK_ENABLED
    ESP_LOGE("esp_http_server", "Please enable LWIP_NETIF_LOOPBACK for %s API", __func__);
    return -1;
#endif

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        return -1;
    }

    int ret;
    struct sockaddr_storage addr = {};
    socklen_t addr_len = 0;
#if IPV4_ENABLED
    struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;
    addr4->sin_family = AF_INET;
    addr4->sin_port = htons(port);
    inet_aton("127.0.0.1", &addr4->sin_addr);
    addr_len = sizeof(struct sockaddr_in);
#else
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&addr;
    addr6->sin6_family = AF_INET6;
    addr6->sin6_port = htons(port);
    inet6_aton("::1", &addr6->sin6_addr);
    addr_len = sizeof(struct sockaddr_in6);
#endif
    ret = bind(fd, (struct sockaddr *)&addr, addr_len);
    if (ret < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

void cs_free_ctrl_sock(int fd)
{
    close(fd);
}

int cs_send_to_ctrl_sock(int send_fd, int port, void *data, unsigned int data_len)
{
    int ret;
    struct sockaddr_storage to_addr = {};
    socklen_t addr_len = 0;
#if IPV4_ENABLED
    struct sockaddr_in *addr4 = (struct sockaddr_in *)&to_addr;
    addr4->sin_family = AF_INET;
    addr4->sin_port = htons(port);
    inet_aton("127.0.0.1", &addr4->sin_addr);
    addr_len = sizeof(struct sockaddr_in);
#else
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&to_addr;
    addr6->sin6_family = AF_INET6;
    addr6->sin6_port = htons(port);
    inet6_aton("::1", &addr6->sin6_addr);
    addr_len = sizeof(struct sockaddr_in6);
#endif
    ret = sendto(send_fd, data, data_len, 0, (struct sockaddr *)&to_addr, addr_len);

    if (ret < 0) {
        return -1;
    }
    return ret;
}

int cs_recv_from_ctrl_sock(int fd, void *data, unsigned int data_len)
{
    int ret;
    ret = recvfrom(fd, data, data_len, 0, NULL, NULL);

    if (ret < 0) {
        return -1;
    }
    return ret;
}
