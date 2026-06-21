/**
 * @file ps5_unlock_ui.c
 * @brief UI implementation for PS5 unlock feature
 * 
 * This module adds UI elements to allow users to unlock their PS5
 * directly from VitaRPS5 without needing external devices.
 * 
 * Features:
 * - Unlock button in the host selection screen
 * - Unlock confirmation dialog
 * - Unlock progress indicator
 * - Unlock success/failure feedback
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vita2d.h>
#include <psp2/kernel/threadmgr.h>

#include "ui_screens.h"
#include "ps5_unlock.h"
#include "context.h"

// ============================================================================
// Constants
// ============================================================================

#define UNLOCK_BUTTON_WIDTH 120
#define UNLOCK_BUTTON_HEIGHT 40
#define UNLOCK_BUTTON_MARGIN 10

#define UNLOCK_HINT_DURATION_US (3000000)  // 3 seconds
#define UNLOCK_PROGRESS_DURATION_US (5000000)  // 5 seconds

// ============================================================================
// UI State
// ============================================================================

typedef struct {
    bool is_unlocking;
    uint64_t unlock_start_time_us;
    bool unlock_success;
    bool unlock_failed;
    char unlock_message[256];
} PS5UnlockUIState;

static PS5UnlockUIState unlock_ui_state = {0};

// ============================================================================
// Unlock Thread
// ============================================================================

/**
 * Background thread for PS5 unlock operation
 */
static int ps5_unlock_thread_func(SceSize args, void *argp) {
    VitaChiakiHost *host = (VitaChiakiHost *)argp;
    
    if (!host) {
        LOGE("Unlock thread: Invalid host");
        unlock_ui_state.unlock_failed = true;
        snprintf(unlock_ui_state.unlock_message, sizeof(unlock_ui_state.unlock_message),
                "Error: Invalid host");
        unlock_ui_state.is_unlocking = false;
        return -1;
    }

    LOGD("Unlock thread: Starting unlock for %s", host->hostname);

    // Attempt to unlock
    int result = ps5_unlock_console(host);

    if (result == 0) {
        unlock_ui_state.unlock_success = true;
        snprintf(unlock_ui_state.unlock_message, sizeof(unlock_ui_state.unlock_message),
                "PS5 unlock signal sent successfully!");
        LOGD("Unlock thread: Success");
    } else {
        unlock_ui_state.unlock_failed = true;
        snprintf(unlock_ui_state.unlock_message, sizeof(unlock_ui_state.unlock_message),
                "Failed to unlock PS5. Check network connection.");
        LOGE("Unlock thread: Failed");
    }

    unlock_ui_state.is_unlocking = false;
    return 0;
}

// ============================================================================
// Public UI Functions
// ============================================================================

/**
 * Initialize PS5 unlock UI state
 */
void ps5_unlock_ui_init(void) {
    memset(&unlock_ui_state, 0, sizeof(unlock_ui_state));
}

/**
 * Request PS5 unlock from UI
 * 
 * This function starts a background thread to unlock the PS5
 * and displays progress feedback to the user.
 * 
 * @param host The PS5 host to unlock
 */
void ps5_unlock_ui_request_unlock(VitaChiakiHost *host) {
    if (!host) {
        LOGE("Unlock UI: Invalid host");
        return;
    }

    if (!ps5_can_unlock_console(host)) {
        LOGE("Unlock UI: Cannot unlock this host");
        unlock_ui_state.unlock_failed = true;
        snprintf(unlock_ui_state.unlock_message, sizeof(unlock_ui_state.unlock_message),
                "This console cannot be unlocked (not registered)");
        return;
    }

    // Reset state
    unlock_ui_state.is_unlocking = true;
    unlock_ui_state.unlock_success = false;
    unlock_ui_state.unlock_failed = false;
    unlock_ui_state.unlock_start_time_us = sceKernelGetProcessTimeWide();
    memset(unlock_ui_state.unlock_message, 0, sizeof(unlock_ui_state.unlock_message));

    LOGD("Unlock UI: Starting unlock request for %s", host->hostname);

    // Create background thread for unlock operation
    SceUID thread_id = sceKernelCreateThread(
        "ps5_unlock_thread",
        ps5_unlock_thread_func,
        0x10000100,  // Priority (same as other background threads)
        0x4000,      // Stack size
        0,
        NULL
    );

    if (thread_id < 0) {
        LOGE("Unlock UI: Failed to create unlock thread: 0x%x", (unsigned int)thread_id);
        unlock_ui_state.is_unlocking = false;
        unlock_ui_state.unlock_failed = true;
        snprintf(unlock_ui_state.unlock_message, sizeof(unlock_ui_state.unlock_message),
                "Failed to start unlock operation");
        return;
    }

    // Start the thread
    sceKernelStartThread(thread_id, sizeof(VitaChiakiHost *), &host);
}

/**
 * Draw unlock button in host selection screen
 * 
 * @param x X position
 * @param y Y position
 * @param host The host to unlock
 * @return true if button was pressed, false otherwise
 */
bool ps5_unlock_ui_draw_button(int x, int y, VitaChiakiHost *host) {
    if (!host) return false;

    // Don't show button if already unlocking or if host cannot be unlocked
    if (unlock_ui_state.is_unlocking || !ps5_can_unlock_console(host)) {
        return false;
    }

    // Button styling
    uint32_t button_color = UI_COLOR_PRIMARY_BLUE;
    uint32_t text_color = UI_COLOR_TEXT_PRIMARY;
    const char *button_text = "Unlock";

    // Draw button background
    vita2d_draw_rectangle(x, y, UNLOCK_BUTTON_WIDTH, UNLOCK_BUTTON_HEIGHT, button_color);

    // Draw button border
    vita2d_draw_rectangle(x, y, UNLOCK_BUTTON_WIDTH, 2, UI_COLOR_TEXT_PRIMARY);
    vita2d_draw_rectangle(x, y + UNLOCK_BUTTON_HEIGHT - 2, UNLOCK_BUTTON_WIDTH, 2, UI_COLOR_TEXT_PRIMARY);
    vita2d_draw_rectangle(x, y, 2, UNLOCK_BUTTON_HEIGHT, UI_COLOR_TEXT_PRIMARY);
    vita2d_draw_rectangle(x + UNLOCK_BUTTON_WIDTH - 2, y, 2, UNLOCK_BUTTON_HEIGHT, UI_COLOR_TEXT_PRIMARY);

    // Draw button text
    int text_width = ui_text_width(font, FONT_SIZE_BODY, button_text);
    int text_x = x + (UNLOCK_BUTTON_WIDTH - text_width) / 2;
    int text_y = y + (UNLOCK_BUTTON_HEIGHT - FONT_SIZE_BODY) / 2;
    ui_text_draw(font, text_x, text_y, text_color, FONT_SIZE_BODY, button_text);

    return true;
}

/**
 * Draw unlock progress/status indicator
 * 
 * Shows progress while unlocking, and success/failure messages
 */
void ps5_unlock_ui_draw_status(void) {
    if (!unlock_ui_state.is_unlocking && !unlock_ui_state.unlock_success && 
        !unlock_ui_state.unlock_failed) {
        return;
    }

    uint64_t current_time_us = sceKernelGetProcessTimeWide();
    uint64_t elapsed_us = current_time_us - unlock_ui_state.unlock_start_time_us;

    // Auto-dismiss after timeout
    if (elapsed_us > UNLOCK_PROGRESS_DURATION_US) {
        unlock_ui_state.is_unlocking = false;
        unlock_ui_state.unlock_success = false;
        unlock_ui_state.unlock_failed = false;
        return;
    }

    // Draw status message at bottom of screen
    int message_y = VITA_HEIGHT - 60;
    uint32_t message_color = UI_COLOR_TEXT_PRIMARY;

    if (unlock_ui_state.is_unlocking) {
        // Draw spinner
        int spinner_x = VITA_WIDTH / 2 - 20;
        int spinner_y = message_y - 40;
        float rotation = (float)((elapsed_us * 720) % 360000000) / 1000000.0f;
        ui_draw_spinner(spinner_x, spinner_y, 15, 3, rotation, UI_COLOR_PRIMARY_BLUE);

        ui_text_draw(font, 50, message_y, message_color, FONT_SIZE_BODY, "Unlocking PS5...");
    } else if (unlock_ui_state.unlock_success) {
        message_color = UI_COLOR_SUCCESS;
        ui_text_draw(font, 50, message_y, message_color, FONT_SIZE_BODY, "✓ PS5 Unlocked!");
    } else if (unlock_ui_state.unlock_failed) {
        message_color = UI_COLOR_ERROR;
        ui_text_draw(font, 50, message_y, message_color, FONT_SIZE_BODY, "✗ Unlock Failed");
    }

    // Draw detailed message if available
    if (unlock_ui_state.unlock_message[0]) {
        ui_text_draw(font, 50, message_y + 30, UI_COLOR_TEXT_SECONDARY, FONT_SIZE_SMALL,
                    unlock_ui_state.unlock_message);
    }
}

/**
 * Handle unlock button input
 * 
 * @param host The host to unlock
 * @return true if unlock was requested, false otherwise
 */
bool ps5_unlock_ui_handle_input(VitaChiakiHost *host) {
    if (!host) return false;

    // Check if user pressed the unlock button (e.g., Triangle button)
    // This should be integrated into the main input handling loop
    // For now, this is a placeholder for the input handling logic

    return false;
}

/**
 * Get current unlock status message
 * 
 * @return Status message string
 */
const char *ps5_unlock_ui_get_status_message(void) {
    if (unlock_ui_state.is_unlocking) {
        return "Unlocking PS5...";
    } else if (unlock_ui_state.unlock_success) {
        return "PS5 Unlocked Successfully!";
    } else if (unlock_ui_state.unlock_failed) {
        return unlock_ui_state.unlock_message;
    }
    return "";
}

/**
 * Check if unlock operation is in progress
 * 
 * @return true if unlocking, false otherwise
 */
bool ps5_unlock_ui_is_unlocking(void) {
    return unlock_ui_state.is_unlocking;
}
