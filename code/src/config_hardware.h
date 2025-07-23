#ifndef CONFIG_HARDWARE_H
#define CONFIG_HARDWARE_H

// Prevent conflicting platform definitions
#ifndef ESP32
#define ESP32
#endif

// Undefine any conflicting macros before including libraries
#ifdef USART1_BASE
#undef USART1_BASE
#endif

#ifdef GPIOA
#undef GPIOA
#endif

#ifdef GPIOB
#undef GPIOB
#endif

// Include ESP32-specific headers first
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

// Define hardware isolation macros
#define ISOLATE_BLE_HEADERS
#define ISOLATE_ARDUCAM_HEADERS

#endif // CONFIG_HARDWARE_H