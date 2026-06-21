/**
 * Network Optimizations for PS Vita Remote Play
 * 
 * This header contains socket optimization constants and utilities
 * for maximizing network performance on PS Vita.
 */

#ifndef VITA_NETWORK_OPTIMIZATIONS_H
#define VITA_NETWORK_OPTIMIZATIONS_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

/* Socket Buffer Sizes */
#define VITA_SOCKET_SNDBUF (512 * 1024)      // 512 KB send buffer
#define VITA_SOCKET_RCVBUF (256 * 1024)      // 256 KB receive buffer
#define VITA_SOCKET_LOWAT (16 * 1024)        // 16 KB low water mark

/* Network QoS Configuration */
#define VITA_IP_TOS_EXPEDITED_FORWARDING 0xB8  // DSCP EF (0xB8 = 184)
#define VITA_TCP_NODELAY 1                      // Disable Nagle's algorithm
#define VITA_TCP_KEEPALIVE 1                    // Enable TCP keepalive

/* Network Timeout Configuration */
#define VITA_SOCKET_CONNECT_TIMEOUT_MS 5000     // 5 second connect timeout
#define VITA_SOCKET_RECV_TIMEOUT_MS 10000       // 10 second receive timeout
#define VITA_SOCKET_SEND_TIMEOUT_MS 5000        // 5 second send timeout

/* Network Retry Configuration */
#define VITA_NETWORK_RETRY_COUNT 3              // Number of retries
#define VITA_NETWORK_RETRY_DELAY_MS 1000        // Delay between retries

/**
 * Apply network optimizations to a socket
 * 
 * @param sock Socket file descriptor
 * @return 0 on success, -1 on error
 */
static inline int vita_socket_optimize(int sock) {
    int optval;
    
    // Set send buffer size
    optval = VITA_SOCKET_SNDBUF;
    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval)) < 0) {
        return -1;
    }
    
    // Set receive buffer size
    optval = VITA_SOCKET_RCVBUF;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval)) < 0) {
        return -1;
    }
    
    // Set low water mark
    optval = VITA_SOCKET_LOWAT;
    if (setsockopt(sock, SOL_SOCKET, SO_SNDLOWAT, &optval, sizeof(optval)) < 0) {
        return -1;
    }
    
    // Disable Nagle's algorithm for lower latency
    optval = VITA_TCP_NODELAY;
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) < 0) {
        return -1;
    }
    
    // Enable TCP keepalive
    optval = VITA_TCP_KEEPALIVE;
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
        return -1;
    }
    
    // Set IP ToS for QoS (Expedited Forwarding)
    optval = VITA_IP_TOS_EXPEDITED_FORWARDING;
    if (setsockopt(sock, IPPROTO_IP, IP_TOS, &optval, sizeof(optval)) < 0) {
        return -1;
    }
    
    return 0;
}

#endif // VITA_NETWORK_OPTIMIZATIONS_H
