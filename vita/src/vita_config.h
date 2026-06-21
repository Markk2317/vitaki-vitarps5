/**
 * VitaRPS5 Optimization Configuration
 * 
 * This header centralizes all performance optimization settings
 * for PS Vita Remote Play streaming.
 */

#ifndef VITA_CONFIG_H
#define VITA_CONFIG_H

/* ============================================================================
 * PERFORMANCE OPTIMIZATION SETTINGS
 * ============================================================================ */

/* Thread Scheduling */
#define VITA_VIDEO_THREAD_PRIORITY 64           // High priority for video decode
#define VITA_VIDEO_THREAD_AFFINITY SCE_KERNEL_CPU_MASK_USER_0
#define VITA_AUDIO_THREAD_PRIORITY 80           // High priority for audio
#define VITA_AUDIO_THREAD_AFFINITY SCE_KERNEL_CPU_MASK_USER_2

/* Frame Processing */
#define VITA_FRAME_VALIDATION_ENABLED 1         // Validate frames before decode
#define VITA_FRAME_MIN_SIZE 5                   // Minimum valid frame size
#define VITA_FRAME_MAX_SIZE (2 * 1024 * 1024)   // 2 MB max frame size

/* Bitrate Adaptation */
#define VITA_BITRATE_MAX_KBPS 3500              // Maximum bitrate for PSN
#define VITA_BITRATE_MIN_KBPS 1500              // Minimum bitrate for stability
#define VITA_PACKET_LOSS_THRESHOLD 5.0f         // % packet loss threshold

/* Network Retry */
#define VITA_PSN_CONNECT_MAX_RETRIES 3          // Max PSN connection retries
#define VITA_PSN_CONNECT_RETRY_DELAY_MS 2000    // Delay between retries
#define VITA_PSN_PREPARE_TIMEOUT_SEC 30         // PSN prepare timeout

/* Memory Configuration */
#define VITA_NETWORK_MEMORY (4 * 1024 * 1024)   // 4 MB network buffer
#define VITA_AUDIO_BUFFER_SIZE (256 * 1024)     // 256 KB audio buffer
#define VITA_VIDEO_BUFFER_SIZE (1024 * 1024)    // 1 MB video buffer

/* Clock Configuration */
#define VITA_ARM_CLOCK_MHZ 444                  // Max stable ARM frequency
#define VITA_GPU_CLOCK_MHZ 222                  // Max stable GPU frequency
#define VITA_BUS_CLOCK_MHZ 222                  // Max stable BUS frequency
#define VITA_XBAR_CLOCK_MHZ 166                 // GPU crossbar frequency

/* ============================================================================
 * FEATURE FLAGS
 * ============================================================================ */

#define VITA_OPTIMIZE_PERFORMANCE 1             // Enable all optimizations
#define VITA_ENABLE_ADAPTIVE_BITRATE 1          // Enable bitrate adaptation
#define VITA_ENABLE_THREAD_AFFINITY 1           // Enable thread affinity
#define VITA_ENABLE_FRAME_VALIDATION 1          // Enable frame validation
#define VITA_ENABLE_RETRY_LOGIC 1               // Enable retry logic
#define VITA_ENABLE_SOCKET_OPTIMIZATION 1       // Enable socket optimization
#define VITA_ENABLE_ENHANCED_LOGGING 1          // Enable detailed logging

/* ============================================================================
 * LOGGING CONFIGURATION
 * ============================================================================ */

#define VITA_LOG_THREAD_OPTIMIZATION 1          // Log thread optimization
#define VITA_LOG_PSN_CONNECTION 1               // Log PSN connection attempts
#define VITA_LOG_BITRATE_ADAPTATION 1           // Log bitrate changes
#define VITA_LOG_DECODE_TIMING 1                // Log decode timing
#define VITA_LOG_NETWORK_STATS 1                // Log network statistics

/* ============================================================================
 * VALIDATION MACROS
 * ============================================================================ */

#define VITA_VALIDATE_FRAME(buf, size) \
    ((buf) != NULL && (size) > VITA_FRAME_MIN_SIZE && (size) < VITA_FRAME_MAX_SIZE)

#define VITA_VALIDATE_BITRATE(br) \
    ((br) >= VITA_BITRATE_MIN_KBPS && (br) <= VITA_BITRATE_MAX_KBPS)

#endif // VITA_CONFIG_H
