#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc.h"

#include "src/DisplayManager/DisplayManager.h"
#include "src/Measurements/Measurements.h"
#include "src/WifiManager/WiFiManager.h"
#include "src/BTManager/BTManager.h"
#include "config.h"

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

// Global variables kept between deep sleep cycles
RTC_DATA_ATTR char wifiSSID[32] = "";             // Store SSID (max 32 chars)
RTC_DATA_ATTR char wifiPassword[64] = "";         // Store password (max 64 chars)
RTC_DATA_ATTR bool wifiCredentialsSet = false;    // Track if credentials are valid
RTC_DATA_ATTR uint32_t measurementInterval = 20;  // Seconds between measurements
RTC_DATA_ATTR int32_t timeShiftHours = 0;         // Hour shift for measurements

// Gloabal variables
static unsigned long lastScheduledMeasurement = 0;
float temperature, humidity;
char* statusBluetooth = "Disabled";
char* statusWiFi      = "Disabled";

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
  // Stop display task
  if(displayTaskHandle != NULL) {
    stopDisplayTask = true;
    stopDisplay();
    vTaskDelay(300 / portTICK_PERIOD_MS);
    delay(500);
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
  Serial.begin(115200);
  initDHT();

  lastScheduledMeasurement = millis();
  handleWakeUp();

  // Restore WiFi credentials from RTC memory after wake-up
  if(wifiCredentialsSet) {
    setWiFiSSID(String(wifiSSID));
    setWiFiPassword(String(wifiPassword));
  }

  pinMode (LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);

  pinMode(BTN_ME, INPUT_PULLUP);
  attachInterrupt(BTN_ME, BTN_ME_ISR, FALLING);
  pinMode(SW_DIS, INPUT_PULLUP);
  attachInterrupt(SW_DIS, SW_DIS_ISR, FALLING);
  pinMode(SW_BLT, INPUT_PULLUP);
  attachInterrupt(SW_BLT, SW_BLT_ISR, FALLING);

  // Create display data structure
  static DisplayData displayData = {
    &statusBluetooth,
    &statusWiFi,
    &temperature,
    &humidity,
    &flagCurrentMeasure,
    &stopDisplayTask,
    &displayTaskHandle
  };

  // Pass the data structure to the display task
  initDisplay();
  xTaskCreatePinnedToCore(displayInfo, "displayInfo", 4096, &displayData, 1, &displayTaskHandle, 0);

  Serial.println("Setup Complete");
  if(!flagWakeUp && !flagTimer) {
    enterDeepSleep();
  }
}

void loop() {
  static unsigned long lastActivity = millis();

  // Handle sleep request when already awake
  if(flagSleepRequest) {
    Serial.println("Sleep requested by button");
    flagSleepRequest = false;
    enterDeepSleep();
  }

  // Check for scheduled measurements even when awake
  if(isTimeForScheduledMeasurement() && !flagTimer) {
    Serial.println("Time for scheduled measurement while awake");
    flagTimer = true;
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
  if(flagBluetooth && strcmp(statusBluetooth, "Disabled") == 0) {
    initBLE();
    statusBluetooth = "Enabled";
    lastActivity = millis();
  } else if(!flagBluetooth && (strcmp(statusBluetooth, "Connected") == 0 || strcmp(statusBluetooth, "Enabled") == 0)) {
    if(stopBLE()) {
      statusBluetooth = "Disabled";
    }
    lastActivity = millis();
  }

  // Handle bluetooth commands
  String command = getBLECommand();
  command.trim();
  if(!(command == "")) {
    if(strcmp(statusBluetooth, "Enabled") == 0) {
      statusBluetooth = "Connected";
    }
    lastActivity = millis();

    if(command == "measure") {
      flagCurrentMeasure = true; 
      if(getDHTMeasurement(&temperature, &humidity)) {
        Serial.println("BT: DHT Measurement success");
      }
    } else if(command.startsWith("wifi_ssid:")) {
      String ssid = command.substring(10);
      ssid.trim();
      
      // Save to RTC memory
      strncpy(wifiSSID, ssid.c_str(), sizeof(wifiSSID) - 1);
      wifiSSID[sizeof(wifiSSID) - 1] = '\0';
      wifiCredentialsSet = true;
      
      // Set in WiFi Manager
      setWiFiSSID(ssid);
      sendBLEResponse("SSID set and saved: " + String(wifiSSID));
      Serial.println("BT: SSID set and saved to RTC memory: " + String(wifiSSID));

    } else if(command.startsWith("wifi_password:")) {
      String password = command.substring(14);
      password.trim();

      // Save to RTC memory
      strncpy(wifiPassword, password.c_str(), sizeof(wifiPassword) - 1);
      wifiPassword[sizeof(wifiPassword) - 1] = '\0';

      // Set in WiFiManager
      setWiFiPassword(password);
      sendBLEResponse("Password set and saved");
      Serial.println("BT: PASSWORD set and saved to RTC memory");

    } else if(command == "wifi_connect") {
      // Restore credentials from RTC memory before connecting
      if(wifiCredentialsSet) {
        setWiFiSSID(String(wifiSSID));
        setWiFiPassword(String(wifiPassword));
      }
      
      if(connectWiFi()) {
        statusWiFi = "Connected";
      } else {
        statusWiFi = "Failed";
      }
    } else if(.startsWith("shift:")) {
      String shiftStr = command.substring(6);
      timeShiftHours = shiftStr.toInt();
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