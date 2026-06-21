/**
 * @file ps5_unlock.c
 * @brief PS5 Unlock/Unblock feature for VitaRPS5
 * 
 * This module implements the ability to unlock/unblock a PS5 console
 * directly from the PS Vita, allowing users to wake up a locked PS5
 * without needing a phone, TV, or controller.
 * 
 * The implementation uses the PS5's REST API to send unlock commands
 * via the network.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psp2/net/http.h>
#include <psp2/net/net.h>
#include <psp2/kernel/threadmgr.h>
#include <chiaki/log.h>

#include "context.h"
#include "host.h"

// ============================================================================
// Constants
// ============================================================================

#define PS5_UNLOCK_PORT 9295
#define PS5_UNLOCK_TIMEOUT_MS 5000
#define PS5_UNLOCK_RETRY_COUNT 3
#define PS5_UNLOCK_RETRY_DELAY_MS 1000

// PS5 REST API endpoints for unlock
#define PS5_UNLOCK_ENDPOINT "/api/v1/system/wake"
#define PS5_STANDBY_ENDPOINT "/api/v1/system/standby"

// ============================================================================
// PS5 Unlock Request Structure
// ============================================================================

typedef struct {
    VitaChiakiHost *host;
    int retry_count;
    uint32_t start_time_ms;
} PS5UnlockRequest;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Send HTTP request to PS5 to unlock/wake it
 * 
 * @param hostname PS5 hostname or IP address
 * @param port PS5 port (usually 9295)
 * @param endpoint API endpoint (e.g., "/api/v1/system/wake")
 * @return 0 on success, -1 on failure
 */
static int ps5_send_unlock_request(const char *hostname, int port, const char *endpoint) {
    if (!hostname || !endpoint) {
        LOGE("PS5 unlock: Invalid hostname or endpoint");
        return -1;
    }

    LOGD("PS5 unlock: Sending request to %s:%d%s", hostname, port, endpoint);

    // Create HTTP connection
    int http_template = sceHttpCreateTemplate(
        "VitaRPS5/1.0",  // User-Agent
        SCE_HTTP_VERSION_1_1,
        SCE_HTTP_FLAG_NONE
    );
    
    if (http_template < 0) {
        LOGE("PS5 unlock: Failed to create HTTP template: 0x%x", (unsigned int)http_template);
        return -1;
    }

    // Set connection timeout
    sceHttpSetConnectTimeOut(http_template, PS5_UNLOCK_TIMEOUT_MS);
    sceHttpSetRecvTimeOut(http_template, PS5_UNLOCK_TIMEOUT_MS);
    sceHttpSetSendTimeOut(http_template, PS5_UNLOCK_TIMEOUT_MS);

    // Create connection
    int http_conn = sceHttpCreateConnection(
        http_template,
        hostname,
        port,
        SCE_HTTP_FLAG_NONE
    );
    
    if (http_conn < 0) {
        LOGE("PS5 unlock: Failed to create HTTP connection: 0x%x", (unsigned int)http_conn);
        sceHttpDeleteTemplate(http_template);
        return -1;
    }

    // Create request
    int http_req = sceHttpCreateRequest(
        http_conn,
        SCE_HTTP_METHOD_POST,
        endpoint,
        0
    );
    
    if (http_req < 0) {
        LOGE("PS5 unlock: Failed to create HTTP request: 0x%x", (unsigned int)http_req);
        sceHttpDeleteConnection(http_conn);
        sceHttpDeleteTemplate(http_template);
        return -1;
    }

    // Add headers
    sceHttpAddRequestHeader(http_req, "Content-Type", "application/json", SCE_HTTP_HEADER_ADD);
    sceHttpAddRequestHeader(http_req, "Accept", "application/json", SCE_HTTP_HEADER_ADD);

    // Send request (empty body for wake command)
    int send_result = sceHttpSendRequest(http_req, NULL, 0);
    
    if (send_result < 0) {
        LOGE("PS5 unlock: Failed to send HTTP request: 0x%x", (unsigned int)send_result);
        sceHttpDeleteRequest(http_req);
        sceHttpDeleteConnection(http_conn);
        sceHttpDeleteTemplate(http_template);
        return -1;
    }

    // Read response status
    int status_code = 0;
    int status_result = sceHttpGetStatusCode(http_req, &status_code);
    
    if (status_result < 0) {
        LOGE("PS5 unlock: Failed to get HTTP status code: 0x%x", (unsigned int)status_result);
        sceHttpDeleteRequest(http_req);
        sceHttpDeleteConnection(http_conn);
        sceHttpDeleteTemplate(http_template);
        return -1;
    }

    LOGD("PS5 unlock: HTTP response status: %d", status_code);

    // Read response body (for diagnostics)
    char response_buffer[256] = {0};
    int read_result = sceHttpReadData(http_req, response_buffer, sizeof(response_buffer) - 1);
    
    if (read_result > 0) {
        response_buffer[read_result] = '\0';
        LOGD("PS5 unlock: Response: %s", response_buffer);
    }

    // Cleanup
    sceHttpDeleteRequest(http_req);
    sceHttpDeleteConnection(http_conn);
    sceHttpDeleteTemplate(http_template);

    // Check if successful (200-299 status codes)
    return (status_code >= 200 && status_code < 300) ? 0 : -1;
}

/**
 * Send wake-on-LAN magic packet to PS5
 * 
 * @param host_mac MAC address of PS5
 * @return 0 on success, -1 on failure
 */
static int ps5_send_wol_packet(const uint8_t *host_mac) {
    if (!host_mac) {
        LOGE("PS5 unlock: Invalid MAC address");
        return -1;
    }

    LOGD("PS5 unlock: Sending WoL magic packet to %02x:%02x:%02x:%02x:%02x:%02x",
         host_mac[0], host_mac[1], host_mac[2], host_mac[3], host_mac[4], host_mac[5]);

    // Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        LOGE("PS5 unlock: Failed to create socket: %d", sock);
        return -1;
    }

    // Enable broadcast
    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        LOGE("PS5 unlock: Failed to enable broadcast");
        close(sock);
        return -1;
    }

    // Build magic packet (6 bytes of 0xFF + 16 repetitions of MAC address)
    uint8_t magic_packet[102];
    memset(magic_packet, 0xFF, 6);
    for (int i = 0; i < 16; i++) {
        memcpy(&magic_packet[6 + i * 6], host_mac, 6);
    }

    // Send to broadcast address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9);  // Discard port
    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    int send_result = sendto(sock, magic_packet, sizeof(magic_packet), 0,
                            (struct sockaddr *)&addr, sizeof(addr));
    
    close(sock);

    if (send_result < 0) {
        LOGE("PS5 unlock: Failed to send WoL packet: %d", send_result);
        return -1;
    }

    LOGD("PS5 unlock: WoL packet sent successfully");
    return 0;
}

// ============================================================================
// Public API
// ============================================================================

/**
 * Unlock/wake a PS5 console
 * 
 * This function attempts to unlock a PS5 that is in standby or locked state.
 * It tries multiple methods:
 * 1. Send HTTP request to PS5 REST API
 * 2. Send WoL magic packet
 * 3. Retry with delays
 * 
 * @param host The PS5 host to unlock
 * @return 0 on success, -1 on failure
 */
int ps5_unlock_console(VitaChiakiHost *host) {
    if (!host) {
        LOGE("PS5 unlock: Invalid host");
        return -1;
    }

    if (!host->hostname) {
        LOGE("PS5 unlock: Host missing hostname");
        return -1;
    }

    LOGD("PS5 unlock: Attempting to unlock %s", host->hostname);

    // Try HTTP REST API first (most reliable for locked consoles)
    for (int attempt = 0; attempt < PS5_UNLOCK_RETRY_COUNT; attempt++) {
        LOGD("PS5 unlock: HTTP attempt %d/%d", attempt + 1, PS5_UNLOCK_RETRY_COUNT);
        
        int result = ps5_send_unlock_request(host->hostname, PS5_UNLOCK_PORT, PS5_UNLOCK_ENDPOINT);
        if (result == 0) {
            LOGD("PS5 unlock: Successfully unlocked via HTTP");
            return 0;
        }

        if (attempt < PS5_UNLOCK_RETRY_COUNT - 1) {
            sceKernelDelayThread(PS5_UNLOCK_RETRY_DELAY_MS * 1000);  // Convert to microseconds
        }
    }

    // Fallback: Try WoL if we have MAC address
    if (host->server_mac[0] || host->server_mac[1] || host->server_mac[2] ||
        host->server_mac[3] || host->server_mac[4] || host->server_mac[5]) {
        LOGD("PS5 unlock: Trying WoL fallback");
        int wol_result = ps5_send_wol_packet(host->server_mac);
        if (wol_result == 0) {
            LOGD("PS5 unlock: Successfully sent WoL packet");
            return 0;
        }
    }

    LOGE("PS5 unlock: Failed to unlock console after %d attempts", PS5_UNLOCK_RETRY_COUNT);
    return -1;
}

/**
 * Check if a PS5 console can be unlocked
 * 
 * @param host The PS5 host to check
 * @return true if the host can be unlocked, false otherwise
 */
bool ps5_can_unlock_console(VitaChiakiHost *host) {
    if (!host) return false;
    if (!host->hostname) return false;
    
    // Can unlock if host is registered and has network connectivity
    bool is_registered = (host->type & REGISTERED) != 0;
    bool has_discovery = (host->type & DISCOVERED) != 0 && host->discovery_state != NULL;
    
    return is_registered || has_discovery;
}
