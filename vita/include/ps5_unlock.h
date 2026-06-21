/**
 * @file ps5_unlock.h
 * @brief PS5 Unlock/Unblock feature for VitaRPS5
 * 
 * Public API for unlocking PS5 consoles directly from PS Vita
 */

#ifndef PS5_UNLOCK_H
#define PS5_UNLOCK_H

#include <stdbool.h>
#include "host.h"

// ============================================================================
// Backend API
// ============================================================================

/**
 * Unlock/wake a PS5 console
 * 
 * Attempts to unlock a PS5 that is in standby or locked state using:
 * 1. HTTP REST API (primary method)
 * 2. Wake-on-LAN magic packet (fallback)
 * 
 * @param host The PS5 host to unlock
 * @return 0 on success, -1 on failure
 */
int ps5_unlock_console(VitaChiakiHost *host);

/**
 * Check if a PS5 console can be unlocked
 * 
 * @param host The PS5 host to check
 * @return true if the host can be unlocked, false otherwise
 */
bool ps5_can_unlock_console(VitaChiakiHost *host);

// ============================================================================
// UI API
// ============================================================================

/**
 * Initialize PS5 unlock UI state
 * 
 * Should be called once during app initialization
 */
void ps5_unlock_ui_init(void);

/**
 * Request PS5 unlock from UI
 * 
 * Starts a background thread to unlock the PS5 and displays progress feedback.
 * 
 * @param host The PS5 host to unlock
 */
void ps5_unlock_ui_request_unlock(VitaChiakiHost *host);

/**
 * Draw unlock button in host selection screen
 * 
 * @param x X position of button
 * @param y Y position of button
 * @param host The host to unlock
 * @return true if button should be displayed, false otherwise
 */
bool ps5_unlock_ui_draw_button(int x, int y, VitaChiakiHost *host);

/**
 * Draw unlock progress/status indicator
 * 
 * Shows progress while unlocking, and success/failure messages
 */
void ps5_unlock_ui_draw_status(void);

/**
 * Get current unlock status message
 * 
 * @return Status message string
 */
const char *ps5_unlock_ui_get_status_message(void);

/**
 * Check if unlock operation is in progress
 * 
 * @return true if unlocking, false otherwise
 */
bool ps5_unlock_ui_is_unlocking(void);

#endif  // PS5_UNLOCK_H
