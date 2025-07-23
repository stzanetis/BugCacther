#ifndef BTMANAGER_H
#define BTMANAGER_H

// Include hardware config first
#include "../config_hardware.h"

// Isolate BLE includes
#ifdef ISOLATE_BLE_HEADERS
#pragma push_macro("USART1_BASE")
#pragma push_macro("GPIOA")
#pragma push_macro("GPIOB")
#undef USART1_BASE
#undef GPIOA
#undef GPIOB
#endif

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#ifdef ISOLATE_BLE_HEADERS
#pragma pop_macro("GPIOB")
#pragma pop_macro("GPIOA")
#pragma pop_macro("USART1_BASE")
#endif

#define ID_BLT  "BugCatcher-BT"
#define SERVICE_UUID        "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "0000ffe1-0000-1000-8000-00805f9b34fb"

void initBLE();
bool stopBLE();
void sendBLEResponse(String message);
String getBLECommand();
void clearBLECommand();

#endif // BTMANAGER_H