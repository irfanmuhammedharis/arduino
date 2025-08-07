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

// I2S Audio Pins (Recording)
#define I2S_WS          25
#define I2S_SCK         26
#define I2S_SD          33

// I2S Audio Pins (Playback) - Using I2S1
#define I2S_PLAYBACK_WS   32
#define I2S_PLAYBACK_SCK  14
#define I2S_PLAYBACK_SD   12

// Input/Output Pins
#define BUTTON_PIN      21
#define STATUS_LED_PIN  2

// ============================================================================
// AUDIO CONFIGURATION
// ============================================================================

// I2S Settings
#define I2S_PORT_RX        I2S_NUM_0     // Recording port
#define I2S_PORT_TX        I2S_NUM_1     // Playback port
#define SAMPLE_RATE        16000         // Hz - Audio sample rate
#define BITS_PER_SAMPLE    I2S_BITS_PER_SAMPLE_16BIT
#define RECORD_TIME        15            // seconds - Max recording time per session
#define WAVE_HEADER_SIZE   44            // bytes - WAV file header size

// Audio Quality Settings
#define DMA_BUF_COUNT      8             // Number of DMA buffers
#define DMA_BUF_LEN        512           // DMA buffer length
#define AUDIO_BUFFER_SIZE  1024          // Audio processing buffer size

// ============================================================================
// BUTTON CONFIGURATION
// ============================================================================

// Button Timing (milliseconds)
#define BUTTON_DEBOUNCE_DELAY   50       // Button debounce time
#define BUTTON_LONG_PRESS_TIME  1000     // Long press threshold

// ============================================================================
// RFID DETECTION CONFIGURATION
// ============================================================================

// RFID Timing (milliseconds)
#define RFID_DETECTION_WINDOW   200      // Stable detection window
#define RFID_REMOVAL_DELAY      1000     // Delay before confirming removal
#define RFID_RETRY_ATTEMPTS     3        // Detection retry attempts
#define RFID_SCAN_INTERVAL      50       // Time between RFID scans

// ============================================================================
// SYSTEM CONFIGURATION
// ============================================================================

// Serial Communication
#define SERIAL_BAUD_RATE        115200   // Serial communication speed

// File System
#define MAX_FILENAME_LENGTH     64       // Maximum filename length
#define AUDIO_FILE_PREFIX       "audio_" // Prefix for audio files
#define AUDIO_FILE_EXTENSION    ".wav"   // Audio file extension

// ============================================================================
// LED STATUS CONFIGURATION
// ============================================================================

// LED Blink Patterns (milliseconds)
#define LED_SLOW_BLINK_PERIOD   1000     // Slow blink period (idle state)
#define LED_FAST_BLINK_PERIOD   200      // Fast blink period (recording/error)

// ============================================================================
// ERROR HANDLING CONFIGURATION
// ============================================================================

// Retry Settings
#define MAX_INIT_RETRIES        3        // Maximum initialization retries
#define SD_INIT_RETRY_DELAY     1000     // Delay between SD init retries
#define ERROR_RECOVERY_INTERVAL 5000     // Error recovery attempt interval
#define SYSTEM_HEALTH_CHECK     10000    // System health check interval

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

// ============================================================================
// FEATURE ENABLES/DISABLES
// ============================================================================

// Feature Flags
#define ENABLE_AUDIO_PLAYBACK   true     // Enable audio playback feature
#define ENABLE_SERIAL_COMMANDS  true     // Enable serial command interface
#define ENABLE_STATUS_LED       true     // Enable status LED
#define ENABLE_DIAGNOSTICS      true     // Enable diagnostic functions

#endif // CONFIG_H