#include <WiFi.h>
#include <HTTPClient.h>

#define BACKEND_URL "https://your-backend-url.com/api/data"

#define CHUNK_SIZE  2042  // Image chunk size in bytes

void setWiFiSSID(String ssid);
void setWiFiPassword(String password);
bool connectWiFi();
void disconnectWiFi();