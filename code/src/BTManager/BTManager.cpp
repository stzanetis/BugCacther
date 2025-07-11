#include "BTManager.h"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
String command = "";

String getBLECommand() {
  return command;
}

void clearBLECommand() {
  command = "";
}

// BLE Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      //Serial.println("BLE Client connected");
    };

    void onDisconnect(BLEServer* pServer) {
      //Serial.println("BLE Client disconnected");
      
      // Restart advertising
      BLEDevice::startAdvertising();
      //Serial.println("Restarting BLE advertising");
    }
};

// BLE Characteristic Callbacks
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      command = pCharacteristic->getValue().c_str();
      command.trim();
    }
};

void initBLE() {
  BLEDevice::init(ID_BLT);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);

  BLEDevice::startAdvertising();
  //Serial.println("BLE advertising started - waiting for connections...");
}

bool stopBLE() {
  if (pServer) {
    BLEDevice::getAdvertising()->stop();
    BLEDevice::deinit(false);
    
    pServer = NULL;
    pCharacteristic = NULL;
    
    //Serial.println("BLE stopped");
    return true;
  }
  return false;
}

void sendBLEResponse(String message) {
  if (pCharacteristic) {
    pCharacteristic->setValue(message.c_str());
    pCharacteristic->notify();
    //Serial.println("BLE Response sent: " + message);
  }
}
