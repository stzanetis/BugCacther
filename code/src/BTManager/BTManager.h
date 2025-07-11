#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define ID_BLT  "BugCatcher-BT"

#define SERVICE_UUID        "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "0000ffe1-0000-1000-8000-00805f9b34fb"

void initBLE();
bool stopBLE();
void sendBLEResponse(String message);
String getBLECommand();
void clearBLECommand(); 