#include "context.h"
#include "host_feedback.h"
#include "host_metrics.h"
#include "host_quit.h"
#include "host_callbacks.h"
#include "video.h"

#include <psp2/kernel/processmgr.h>

// Startup can include console wake + decoder warmup. Keep a short grace for
// burst suppression and a longer hard grace for severe unrecovered churn.
#define LOSS_RESTART_STARTUP_SOFT_GRACE_US (2500 * 1000ULL)
#define LOSS_RESTART_STARTUP_HARD_GRACE_US (20 * 1000 * 1000ULL)

/**
 * Handle LOGIN_PIN_REQUEST event from console.
 * Activates PIN entry UI and waits for user input.
 */
static void host_handle_login_pin_request(ChiakiEvent *event) {
  bool pin_incorrect = event->login_pin_request.pin_incorrect;
  uint64_t event_time = sceKernelGetProcessTimeCounter();
  
  LOGD("[TIMING] LOGIN_PIN_REQUEST event received (timestamp: %llu, incorrect=%s)", event_time, pin_incorrect ? "true" : "false");
  
  // Initialize PIN entry state
  context.ui_state.pin_entry_active = true;
  context.ui_state.pin_entry_incorrect = pin_incorrect;
  context.ui_state.pin_entry_cursor = 0;
  memset(context.ui_state.pin_entry_buffer, 0, sizeof(context.ui_state.pin_entry_buffer));
  memset(context.ui_state.pin_entry_digit_set, 0, sizeof(context.ui_state.pin_entry_digit_set));
  
  LOGD("PIN entry activated (incorrect=%s)", pin_incorrect ? "true" : "false");
}

void host_event_cb(ChiakiEvent *event, void *user) {
  switch (event->type) {
    case CHIAKI_EVENT_CONNECTED:
      LOGD("EventCB CHIAKI_EVENT_CONNECTED");
      context.stream.stream_start_us = sceKernelGetProcessTimeWide();
      context.stream.loss_restart_soft_grace_until_us =
          context.stream.stream_start_us + LOSS_RESTART_STARTUP_SOFT_GRACE_US;
      context.stream.loss_restart_grace_until_us =
          context.stream.stream_start_us + LOSS_RESTART_STARTUP_HARD_GRACE_US;
      context.stream.post_reconnect_window_until_us = 0;
      context.stream.inputs_ready = true;
      context.stream.next_stream_allowed_us = 0;
      context.stream.retry_holdoff_ms = 0;
      context.stream.retry_holdoff_until_us = 0;
      context.stream.retry_holdoff_active = false;
      context.stream.restart_handshake_failures = 0;
      context.stream.last_restart_handshake_fail_us = 0;
      context.stream.restart_cooloff_until_us = 0;
      context.stream.last_restart_source[0] = '\0';
      context.stream.restart_source_attempts = 0;
      LOGD("PIPE/SESSION connected gen=%u reconnect_gen=%u post_window_ms=%llu",
           context.stream.session_generation, context.stream.reconnect_generation,
           context.stream.post_reconnect_window_until_us
               ? (unsigned long long)((context.stream.post_reconnect_window_until_us -
                                       context.stream.stream_start_us) /
                                      1000ULL)
               : 0ULL);
      ui_connection_set_stage(UI_CONNECTION_STAGE_STARTING_STREAM);
      if (context.stream.fast_restart_active) {
        context.stream.fast_restart_active = false;
        context.stream.reconnect_overlay_active = false;
      }
      break;
    case CHIAKI_EVENT_LOGIN_PIN_REQUEST:
      LOGD("EventCB CHIAKI_EVENT_LOGIN_PIN_REQUEST (incorrect=%s)",
           event->login_pin_request.pin_incorrect ? "true" : "false");
      host_handle_login_pin_request(event);
      break;
    case CHIAKI_EVENT_RUMBLE:
      LOGD("EventCB CHIAKI_EVENT_RUMBLE");
      break;
    case CHIAKI_EVENT_QUIT:
      host_handle_quit_event(event);
      break;
  }
}

bool host_video_cb(uint8_t *buf, size_t buf_size, int32_t frames_lost, bool frame_recovered,
                   void *user) {
  if (context.stream.stop_requested)
    return false;
  if (!context.stream.video_first_frame_logged) {
    LOGD("VIDEO CALLBACK: First frame received (size=%zu)", buf_size);
    context.stream.video_first_frame_logged = true;
  }
  if (frames_lost > 0) {
    host_handle_loss_event(frames_lost, frame_recovered);
    host_handle_unrecovered_frame_loss(frames_lost, frame_recovered);
  }
  context.stream.is_streaming = true;
  context.stream.reset_reconnect_gen = false;  // Streaming started — consume the reset flag
  if (ui_connection_overlay_active())
    ui_connection_complete();
  if (context.stream.reconnect_overlay_active)
    context.stream.reconnect_overlay_active = false;
  int err = vita_h264_decode_frame(buf, buf_size);
  if (err != 0) {
    LOGE("Error during video decode: %d", err);
    return false;
  }
  host_metrics_update_latency();
  return true;
}


/**
 * Submit the entered PIN to the Chiaki session.
 * Converts the 4-digit PIN buffer to bytes and sends it to the console.
 * 
 * PIN Encoding: ASCII (0x31-0x39 for digits "1"-"9", 0x30 for "0")
 * This is the most common encoding for PIN transmission in remote play protocols.
 */
void host_submit_login_pin(void) {
  if (!context.ui_state.pin_entry_active) {
    LOGD("PIN submission attempted but PIN entry not active");
    return;
  }
  
  // Convert 4-digit PIN buffer to ASCII bytes
  // Example: buffer [1,2,3,4] → bytes [0x31, 0x32, 0x33, 0x34]
  uint8_t pin_bytes[4];
  for (int i = 0; i < 4; i++) {
    // Convert digit (0-9) to ASCII character code (0x30-0x39)
    pin_bytes[i] = 0x30 + context.ui_state.pin_entry_buffer[i];
  }
  
  LOGD("Submitting PIN to console (bytes: %02x %02x %02x %02x)",
       pin_bytes[0], pin_bytes[1], pin_bytes[2], pin_bytes[3]);
  
  uint64_t time_before = sceKernelGetProcessTimeCounter();
  LOGD("[TIMING] About to call chiaki_session_set_login_pin (timestamp: %llu)", time_before);
  
  ChiakiErrorCode err = chiaki_session_set_login_pin(&context.stream.session, pin_bytes, 4);
  
  uint64_t time_after = sceKernelGetProcessTimeCounter();
  LOGD("[TIMING] chiaki_session_set_login_pin returned (timestamp: %llu, delta: %llu)", time_after, time_after - time_before);
  
  if (err != CHIAKI_ERR_SUCCESS) {
    LOGE("Failed to set login PIN: %s", chiaki_error_string(err));
  }
  
  // Deactivate PIN entry UI
  context.ui_state.pin_entry_active = false;
}
