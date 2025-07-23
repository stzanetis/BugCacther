#include "WiFiManager.h"

// Local variables
static String localSSID = "";
static String localPassword = "";
//int temp;
//int hum;

void setWiFiSSID(String ssid) {
  ssid.trim();
  if(ssid.length() > 0 && ssid.endsWith("\n")) {
    ssid.remove(ssid.length() - 1);
  }
  localSSID = ssid;
}

void setWiFiPassword(String password) {
  password.trim();
  if (password.length() > 0 && password.endsWith("\n")) {
    password.remove(password.length() - 1);
  }
  localPassword = password;
}

bool connectWiFi() {
  if (localSSID.isEmpty() || localPassword.isEmpty()) {
    Serial.println("WiFi SSID or Password not set");
    return false;
  } else {
    WiFi.begin(localSSID.c_str(), localPassword.c_str());
    
    // Add timeout for connection
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - startTime > 30000) { // 30 second timeout
        Serial.println("\nWiFi connection timed out");
        return false;
      }
    }

    return true;
  }
}

void disconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

//bool sendMeasurement(bool useFlash) {
//  if(!connectWifi()) {
//    Serial.println("Failed to connect to WiFi");
//    return false;
//  }

//  if(useFlash) {
//    // Enable flash before capturing
//    pinMode(FLASHPIN, OUTPUT);
//    digitalWrite(FLASHPIN, HIGH);
//    delay(100);
//  }
  
//  // Capture image and temp/hum measurement
//  if(initCamera() && initDHT()) {
//    if(!captureImage()) {
//      return false;
//    }
//    getDHTMeasurement(&temp, &hum);
//  } else {
//    return false;
//  }

//  if(useFlash) {
//    // Disable flash
//    digitalWrite(FLASHPIN, LOW);
//  }

//  return true;
//}