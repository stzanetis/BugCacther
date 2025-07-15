#include "WiFiManager.h"

// Local variables
static String localSSID = "";
static String localPassword = "";

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