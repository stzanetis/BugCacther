#include "WiFiManager.h"

String wifiSSID = "";
String wifiPassword = "";

void setWiFiSSID(String ssid) {
  ssid.trim();
  if(ssid.length() > 0) {
    ssid.remove(ssid.length() - 1);
  }
  Serial.println("Setting WiFi SSID: " + ssid);
  wifiSSID = ssid;
}

void setWiFiPassword(String password) {
  password.trim();
  if (password.length() > 0) {
    password.remove(password.length() - 1);
  }
  Serial.println("Setting WiFi Password: " + password);
	wifiPassword = password;
}

bool connectWiFi() {
  if (wifiSSID.isEmpty() || wifiPassword.isEmpty()) {
    Serial.println("WiFi SSID or Password not set");
    return false;
  } else {
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    
    // Add timeout for connection
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - startTime > 1200000) {
      Serial.println("\nWiFi connection timed out");
      return false;
      }
    }
    
    Serial.println("\nConnected to WiFi");
    return true;
  }
}

void disconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}