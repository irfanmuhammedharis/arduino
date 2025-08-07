/*
 * Configuration file for ESP32 RFID Audio Recorder
 * 
 * This file contains all configurable parameters that can be adjusted
 * for different hardware setups or use cases.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// PIN CONFIGURATION
// ============================================================================

// RFID RC522 Pins
#define RFID_SS_PIN     5
#define RFID_RST_PIN    22

// SD Card Pins
#define SD_CS_PIN       15

// I2S Audio Pins
#define I2S_WS          25
#define I2S_SCK         26
#define I2S_SD          33

// Input/Output Pins
#define BUTTON_PIN      4
#define STATUS_LED_PIN  2

// ============================================================================
// AUDIO CONFIGURATION
// ============================================================================

// I2S Settings
#define I2S_PORT        I2S_NUM_0
#define SAMPLE_RATE     16000                    // Hz - Audio sample rate
#define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_32BIT
#define RECORD_TIME     10                       // seconds - Max recording time
#define WAVE_HEADER_SIZE 44                      // bytes - WAV file header size

// Audio Quality Settings
#define DMA_BUF_COUNT   4                        // Number of DMA buffers
#define DMA_BUF_LEN     1024                     // DMA buffer length
#define AUDIO_BUFFER_SIZE 1024                   // Audio processing buffer size

// ============================================================================
// BUTTON CONFIGURATION
// ============================================================================

// Button Timing (milliseconds)
#define BUTTON_DEBOUNCE_DELAY   50               // Button debounce time
#define BUTTON_LONG_PRESS_TIME  1000             // Long press threshold
#define BUTTON_VERY_LONG_PRESS  3000             // Very long press (for special functions)

// ============================================================================
// RFID DETECTION CONFIGURATION
// ============================================================================

// RFID Timing (milliseconds)
#define RFID_DETECTION_WINDOW   200              // Stable detection window
#define RFID_REMOVAL_DELAY      1000             // Delay before confirming removal
#define RFID_RETRY_ATTEMPTS     3                // Detection retry attempts
#define RFID_SCAN_INTERVAL      100              // Time between RFID scans

// Advanced RFID Settings
#define RFID_MIN_DETECTION_COUNT 2               // Minimum detections before confirming presence
#define RFID_PRESENCE_TIMEOUT   5000             // Max time between detections before removal

// ============================================================================
// SYSTEM CONFIGURATION
// ============================================================================

// Serial Communication
#define SERIAL_BAUD_RATE        115200           // Serial communication speed
#define SERIAL_TIMEOUT          1000             // Serial read timeout

// File System
#define MAX_FILENAME_LENGTH     64               // Maximum filename length
#define AUDIO_FILE_PREFIX       "audio_"         // Prefix for audio files
#define AUDIO_FILE_EXTENSION    ".wav"           // Audio file extension

// Memory Management
#define MAX_AUDIO_FILES         100              // Maximum audio files to track
#define FILE_BUFFER_SIZE        512              // File operation buffer size

// ============================================================================
// LED STATUS CONFIGURATION
// ============================================================================

// LED Blink Patterns (milliseconds)
#define LED_SLOW_BLINK_PERIOD   1000             // Slow blink period (idle state)
#define LED_FAST_BLINK_PERIOD   200              // Fast blink period (recording/error)
#define LED_VERY_FAST_BLINK     100              // Very fast blink (critical error)

// LED Brightness (if using PWM)
#define LED_BRIGHTNESS_LOW      64               // Low brightness level
#define LED_BRIGHTNESS_HIGH     255              // High brightness level

// ============================================================================
// ERROR HANDLING CONFIGURATION
// ============================================================================

// Retry Settings
#define MAX_INIT_RETRIES        3                // Maximum initialization retries
#define SD_INIT_RETRY_DELAY     1000             // Delay between SD init retries
#define ERROR_RECOVERY_INTERVAL 5000             // Error recovery attempt interval

// Watchdog Settings
#define WATCHDOG_TIMEOUT        30000            // Watchdog timeout (milliseconds)
#define SYSTEM_HEALTH_CHECK     10000            // System health check interval

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================

// Debug Levels
#define DEBUG_LEVEL_NONE        0
#define DEBUG_LEVEL_ERROR       1
#define DEBUG_LEVEL_WARNING     2
#define DEBUG_LEVEL_INFO        3
#define DEBUG_LEVEL_VERBOSE     4

// Current Debug Level
#define DEBUG_LEVEL             DEBUG_LEVEL_INFO

// Debug Features
#define ENABLE_SERIAL_DEBUG     true             // Enable serial debug output
#define ENABLE_LED_DEBUG        true             // Enable LED debug indicators
#define ENABLE_TIMING_DEBUG     false            // Enable timing measurements
#define ENABLE_MEMORY_DEBUG     false            // Enable memory usage tracking

// ============================================================================
// PERFORMANCE TUNING
// ============================================================================

// CPU and Power Management
#define CPU_FREQUENCY           240              // MHz - CPU frequency
#define ENABLE_LIGHT_SLEEP      false            // Enable light sleep mode
#define SLEEP_TIMEOUT           30000            // Sleep timeout when idle

// SPI Bus Settings
#define SPI_FREQUENCY           4000000          // Hz - SPI bus frequency
#define SPI_MAX_TRANSFER_SIZE   4092             // Maximum SPI transfer size

// Task Priorities (if using FreeRTOS tasks)
#define TASK_PRIORITY_HIGH      3
#define TASK_PRIORITY_NORMAL    2
#define TASK_PRIORITY_LOW       1

// ============================================================================
// HARDWARE VARIANT SUPPORT
// ============================================================================

// Uncomment for specific hardware variants
// #define HARDWARE_V1_0                         // Original hardware version
// #define HARDWARE_V2_0                         // Updated hardware version
// #define CUSTOM_PCB                            // Custom PCB layout

// Board-specific configurations
#ifdef HARDWARE_V1_0
  #undef BUTTON_PIN
  #define BUTTON_PIN 0                           // Different button pin for v1.0
#endif

// ============================================================================
// FEATURE ENABLES/DISABLES
// ============================================================================

// Feature Flags
#define ENABLE_AUDIO_PLAYBACK   true             // Enable audio playback feature
#define ENABLE_SERIAL_COMMANDS  true             // Enable serial command interface
#define ENABLE_FILE_MANAGEMENT  true             // Enable file management commands
#define ENABLE_STATUS_LED       true             // Enable status LED
#define ENABLE_BUTTON_FEEDBACK  true             // Enable button press feedback

// Advanced Features
#define ENABLE_AUDIO_COMPRESSION false           // Enable audio compression
#define ENABLE_WIFI_FEATURES    false            // Enable WiFi connectivity
#define ENABLE_BLUETOOTH        false            // Enable Bluetooth features
#define ENABLE_OTA_UPDATES      false            // Enable over-the-air updates

#endif // CONFIG_H