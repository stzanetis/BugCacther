#include <U8g2lib.h>
#include <Ticker.h>

#include "src/WifiManager/WiFiManager.h"
#include "src/Measurements/Measurements.h"
#include "src/BTManager/BTManager.h"
#include "config.h"

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCLPIN, SDAPIN, U8X8_PIN_NONE);
Ticker measureTimer;

TaskHandle_t displayTaskHandle = NULL;
volatile bool stopDisplayTask = false;

// Interrupt flags
volatile bool flagButton    = false;
volatile bool flagTimer     = false;
volatile bool flagDisplay   = false;
volatile bool flagBluetooth = false;
volatile bool flagWiFi      = false;
volatile bool flagCurrentMeasure = false;

// Debouncing variables
volatile unsigned long lastBTN_ME = 0;
volatile unsigned long lastSW_DIS = 0;
volatile unsigned long lastSW_BLT = 0;

// Status strings
char* statusBluetooth = "Disabled";
char* statusWiFi      = "Disabled";

// Gloabal variables
float temperature;
float humidity;

void measureTicker() {
  flagTimer = true;  // Spoof button press for periodic measurement
}

void BTN_ME_ISR() {
  unsigned long currentTime = millis();
  
  // Software debounce for button press
  if (currentTime - lastBTN_ME > 200) {
    flagButton = !flagButton;
    //Serial.println("Measuring button pressed");
    lastBTN_ME = currentTime;
  }
}

void SW_DIS_ISR() {
  unsigned long currentTime = millis();
  
  // Software debounce for button press
  if (currentTime - lastSW_DIS > 200) {
    flagDisplay = !flagDisplay;
    //Serial.println("Display switch pressed");
    lastSW_DIS = currentTime;
  }
}

void SW_BLT_ISR() {
  unsigned long currentTime = millis();
  
  // Software debounce for button press
  if (currentTime - lastSW_BLT > 200) {
    flagBluetooth = !flagBluetooth;
    //Serial.println("Bluetooth switch pressed");
    lastSW_BLT = currentTime;
  }
}

void setup() {
  Serial.begin(115200); // Baud rate for serial communication
  u8g2.begin();         // Enable display
  initDHT();            // Initialize DHT sensor

  pinMode (LEDPIN, OUTPUT);   // Set LED pin as output
  digitalWrite(LEDPIN, HIGH); // Turn on LED

  pinMode(BTN_ME, INPUT_PULLUP);  // Set Measure Button pin as input with pull-up resistor
  attachInterrupt(BTN_ME, BTN_ME_ISR, FALLING); // Set ISR for Measure Button
  pinMode(SW_DIS, INPUT_PULLUP);  // Set Display Switch pin as input with pull-up resistor
  attachInterrupt(SW_DIS, SW_DIS_ISR, FALLING); // Set ISR for Display Switch
  pinMode(SW_BLT, INPUT_PULLUP);  // Set Bluetooth Switch pin as input with pull-up resistor
  attachInterrupt(SW_BLT, SW_BLT_ISR, FALLING); // Set ISR for Bluetooth Switch

  // Get a measurement every 20 seconds
  measureTimer.attach(20, measureTicker);

  Serial.println("Setup Complete");
}

void loop() {
  // Handle periodic measurements
  if(flagTimer) {
    flagCurrentMeasure = flagTimer;
    flagTimer = false;
    while(!getDHTMeasurement(&temperature, &humidity)) {
      Serial.println("DHT Measurement failed");
      delay(1000); // Wait 1s before retrying
    }
    Serial.println("Periodic DHT Measurement");
  }

  if(flagButton) {
    flagCurrentMeasure = flagButton;
    flagButton = false;
    while(!getDHTMeasurement(&temperature, &humidity)) {
      Serial.println("DHT Measurement failed");
      delay(1000); // Wait 1s before retrying
    }
    Serial.println("Button DHT Measurement");
  }

  // Initialize/Deinitialize bluetooth only when flag changes
  if(flagBluetooth && statusBluetooth == "Disabled") {
    initBLE();
    statusBluetooth = "Enabled";
  } else if(!flagBluetooth && (statusBluetooth == "Connected" || statusBluetooth == "Enabled")) {
    if(stopBLE()) {
      statusBluetooth = "Disabled";
    }
  }

  // Handle bluetooth commands
  String command = getBLECommand();
  command.trim();
  if(!(command == "")) {
    statusBluetooth = "Connected"; 
    
    if(command == "measure") {
      flagCurrentMeasure = true; 
      if(getDHTMeasurement(&temperature, &humidity)) {
        Serial.println("BT: DHT Measurement success");
        sendBLEResponse("Temperature: " + String(temperature, 1) + "°C, Humidity: " + String(humidity, 1) + "%");
      }
    }else if(command.startsWith("wifi_ssid:")) {
      setWiFiSSID(command.substring(10));
    } else if(command.startsWith("wifi_password:")) {
      setWiFiPassword(command.substring(14));
    } else if(command == "wifi_test") {
      if(connectWiFi()) {
        statusWiFi = "Connected";
      }
    } else {
      sendBLEResponse("Invalid command: " + command);
    }

    clearBLECommand(); // Clear command after processing
  }

  // Handle display updates
  static bool flagLastDisplay = flagDisplay;
  if(flagDisplay != flagLastDisplay) {
    if(flagDisplay && displayTaskHandle == NULL) {
      xTaskCreatePinnedToCore(displayInfo, "displayInfo", 4096, NULL, 1, &displayTaskHandle, 0);
    } else if(!flagDisplay && displayTaskHandle != NULL) {
      stopDisplayTask = true; // Signal task to stop
    }
    flagLastDisplay = flagDisplay;
  }
}

void displayInfo(void *parameter) {
  while(true) {
    if(stopDisplayTask) {
      u8g2.clearBuffer();
      u8g2.sendBuffer();
      
      // Clean up and delete task
      displayTaskHandle = NULL;
      stopDisplayTask = false;
      vTaskDelete(NULL);
    }
    
    if(flagDisplay) {
      u8g2.clearBuffer();

      u8g2.setFont(u8g2_font_luBS08_tf);
      u8g2.drawStr(0, 18, "Blu-T: ");
      u8g2.setFont(u8g2_font_luRS10_tf);
      u8g2.drawStr(36, 18, statusBluetooth);

      u8g2.setFont(u8g2_font_luBS08_tf);
      u8g2.drawStr(1, 36, "Wi-Fi: ");
      u8g2.setFont(u8g2_font_luRS10_tf);
      u8g2.drawStr(36, 36, statusWiFi);

      // Show capture indicator
      if(flagCurrentMeasure) {
        u8g2.drawCircle(120, 56, 6);
        u8g2.drawDisc(120, 56, 3);
        flagCurrentMeasure = false;
      }

      u8g2.sendBuffer();
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    } else {
      u8g2.clearBuffer();
      u8g2.sendBuffer();
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}