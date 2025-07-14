#include <U8g2lib.h>
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc.h"

#include "src/WifiManager/WiFiManager.h"
#include "src/Measurements/Measurements.h"
#include "src/BTManager/BTManager.h"
#include "config.h"

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCLPIN, SDAPIN, U8X8_PIN_NONE);

TaskHandle_t displayTaskHandle = NULL;
volatile bool stopDisplayTask  = false;

// Interrupt flags
volatile bool flagButton    = false;
volatile bool flagTimer     = false;
volatile bool flagBluetooth = false;
volatile bool flagWiFi      = false;
volatile bool flagWakeUp    = false;
volatile bool flagCurrentMeasure = false;
volatile bool flagSleepRequest   = false;

// Debouncing variables
volatile unsigned long lastBTN_ME = 0;
volatile unsigned long lastSW_DIS = 0;
volatile unsigned long lastSW_BLT = 0;

// Status strings
char* statusBluetooth = "Disabled";
char* statusWiFi      = "Disabled";

// Gloabal variables
static unsigned long lastScheduledMeasurement = 0;  // Track last scheduled measurement time
RTC_DATA_ATTR uint32_t measurementInterval = 20;    // Seconds between measurements
RTC_DATA_ATTR int32_t timeShiftHours = 0;
float temperature;
float humidity;

void BTN_ME_ISR() {
  unsigned long currentTime = millis();
  
  // Software debounce for button press
  if (currentTime - lastBTN_ME > 200) {
    flagButton = !flagButton;
    lastBTN_ME = currentTime;
  }
}

void SW_DIS_ISR() {
  unsigned long currentTime = millis();
  
  // Software debounce for button press
  if (currentTime - lastSW_DIS > 200) {
    if(flagWakeUp) {
      flagSleepRequest = true;
      u8g2.clearBuffer();
      u8g2.sendBuffer();
      delay(500);
    }
    lastSW_DIS = currentTime;
  }
}

void SW_BLT_ISR() {
  unsigned long currentTime = millis();
  
  // Software debounce for button press
  if (currentTime - lastSW_BLT > 200) {
    flagBluetooth = !flagBluetooth;
    lastSW_BLT = currentTime;
  }
}

void handleWakeUp() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Woke up from timer - taking measurement");
      flagTimer = true;
      break;
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Woke up from the button");
      flagWakeUp = true;
      break;
    default:
      Serial.println("Woke up from other source");
      break;
  }
}

bool isTimeForScheduledMeasurement() {
  return (millis() - lastScheduledMeasurement) >= (measurementInterval * 1000UL);
}

void enterDeepSleep() {
  Serial.println("Entering deep sleep...");
  
  // Stop display task
  if(displayTaskHandle != NULL) {
    stopDisplayTask = true;
    u8g2.clearBuffer();
    u8g2.sendBuffer();

    vTaskDelay(300 / portTICK_PERIOD_MS);
  }

  // Turn off LED
  digitalWrite(LEDPIN, LOW);

  // Disable bluetooth if active
  if(statusBluetooth != "Disabled") {
    stopBLE();
  }

  // Disconnect WiFi if active
  if(statusWiFi != "Disabled") {
    disconnectWiFi();
  }

  // Configure wake-up sources
  uint64_t sleepTimeUs = (measurementInterval + (timeShiftHours * 3600ULL)) * 1000000ULL;
  esp_sleep_enable_timer_wakeup(sleepTimeUs);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)SW_DIS, 0);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200); // Baud rate for serial communication
  u8g2.begin();         // Enable display
  initDHT();            // Initialize DHT sensor

  lastScheduledMeasurement = millis();  // Initialize measurement schedule
  handleWakeUp();                       // Handle wake-up reason

  pinMode (LEDPIN, OUTPUT);   // Set LED pin as output
  digitalWrite(LEDPIN, HIGH); // Turn on LED

  pinMode(BTN_ME, INPUT_PULLUP);  // Set Measure Button pin as input with pull-up resistor
  attachInterrupt(BTN_ME, BTN_ME_ISR, FALLING); // Set ISR for Measure Button
  pinMode(SW_DIS, INPUT_PULLUP);  // Set Display Switch pin as input with pull-up resistor
  attachInterrupt(SW_DIS, SW_DIS_ISR, FALLING); // Set ISR for Display Switch
  pinMode(SW_BLT, INPUT_PULLUP);  // Set Bluetooth Switch pin as input with pull-up resistor
  attachInterrupt(SW_BLT, SW_BLT_ISR, FALLING); // Set ISR for Bluetooth Switch

  xTaskCreatePinnedToCore(displayInfo, "displayInfo", 4096, NULL, 1, &displayTaskHandle, 0);

  Serial.println("Setup Complete");
  if(!flagWakeUp && !flagTimer) {
    enterDeepSleep();
  }
}

void loop() {
  static unsigned long lastActivity = millis();

  // Check for scheduled measurements even when awake
  if(isTimeForScheduledMeasurement() && !flagTimer) {
    Serial.println("Time for scheduled measurement while awake");
    flagTimer = true;
  }

  // Handle sleep request when already awake
  if(flagSleepRequest) {
    Serial.println("Sleep requested by button");
    flagSleepRequest = false;
    enterDeepSleep();
  }

  // Handle timer-based measurements
  if(flagTimer) {
    flagCurrentMeasure = true;
    flagTimer = false;

    lastScheduledMeasurement = millis();
    while(!getDHTMeasurement(&temperature, &humidity)) {
      Serial.println("DHT Measurement failed");
      delay(1000); // Wait 1s before retrying
    }
    Serial.println("Scheduled DHT Measurement");

    if(!flagWakeUp) {
      delay(5000); // Delay before going back to sleep
      enterDeepSleep();
    } else {
      lastActivity = millis(); // Update activity if display is active
    }
  }

  // Handle capture button
  if(flagButton) {
    flagCurrentMeasure = true;
    flagButton = false;
    while(!getDHTMeasurement(&temperature, &humidity)) {
      Serial.println("DHT Measurement failed");
      delay(1000); // Wait 1s before retrying
    }
    Serial.println("Button DHT Measurement");
    lastActivity = millis();
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
    lastActivity = millis();

    if(command == "measure") {
      flagCurrentMeasure = true; 
      if(getDHTMeasurement(&temperature, &humidity)) {
        Serial.println("BT: DHT Measurement success");
        sendBLEResponse("Temperature: " + String(temperature, 1) + "°C, Humidity: " + String(humidity, 1) + "%");
      }
    }else if(command.startsWith("wifi_ssid:")) {
      setWiFiSSID(command.substring(10));
      Serial.println("BT: SSID set success");
    } else if(command.startsWith("wifi_password:")) {
      setWiFiPassword(command.substring(14));
      Serial.println("BT: PASSWORD set success");
    } else if(command == "wifi_test") {
      if(connectWiFi()) {
        statusWiFi = "Connected";
      }
    } else {
      sendBLEResponse("Invalid command: " + command);
    }

    clearBLECommand(); // Clear command after processing
  }

  // Auto-sleep after 5 minutes of inactivity
  if(millis() - lastActivity > 300000) {
    Serial.println("Auto-sleeping due to inactivity");
    enterDeepSleep();
  }

  delay(50);
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
    } else{
      u8g2.clearBuffer();

      u8g2.setFont(u8g2_font_luBS08_tf);
      u8g2.drawStr(0, 18, "BT: ");
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
    }
  }
}