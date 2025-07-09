#include <BluetoothSerial.h>
#include <U8g2lib.h>
#include <Ticker.h>

#include "config.h"
#include "src/Measurements/Measurements.h"

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCLPIN, SDAPIN, U8X8_PIN_NONE);
BluetoothSerial SerialBT;
Ticker measureTimer;

volatile bool measureFlag   = false;
volatile bool displayFlag   = false;
volatile bool bluetoothFlag = false;
volatile bool wifiFlag      = false;

volatile unsigned long lastBTN_ME = 0;
volatile unsigned long lastSW_DIS = 0;
volatile unsigned long lastSW_BLT = 0;

// Track state of initializations
bool bluetoothInit      = false;
bool wifiInit           = false;
bool wifiConnected      = false;
bool currentMeasureFlag = false;

float temperature;
float humidity;

void measureTicker() {
  measureFlag = true;
}

void BTN_ME_ISR() {
  unsigned long currentTime = millis();
  
  // Software debounce for button press
  if (currentTime - lastBTN_ME > 200) {
    measureFlag = !measureFlag;
    Serial.println("Measuring button pressed");
    lastBTN_ME = currentTime;
  }
}

void SW_DIS_ISR() {
  unsigned long currentTime = millis();
  
  // Software debounce for button press
  if (currentTime - lastSW_DIS > 200) {
    displayFlag = !displayFlag;
    Serial.println("Display switch pressed");
    lastSW_DIS = currentTime;
  }
}

void SW_BLT_ISR() {
  unsigned long currentTime = millis();
  
  // Software debounce for button press
  if (currentTime - lastSW_BLT > 200) {
    bluetoothFlag = !bluetoothFlag;
    Serial.println("Bluetooth switch pressed");
    lastSW_BLT = currentTime;
  }
}

void setup() {
  Serial.begin(115200);                   // Baud rate for serial communication
  u8g2.begin();                         // Enable display
  initDHT();                              // Initialize DHT sensor

  pinMode (LEDPIN, OUTPUT);               // Set LED pin as output
  digitalWrite(LEDPIN, HIGH);             // Turn on LED

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
  // Store the measurement flag state
  currentMeasureFlag = measureFlag;

  if(!measureFlag || !bluetoothFlag || !displayFlag || !wifiFlag) {
    __WFI(); // Enter low power mode if no flags are set
  }

  // Handle periodic measurements
  if(measureFlag) {
    measureFlag = false;
    while(!getDHTMeasurement(&temperature, &humidity)) {
      Serial.println("DHT Measurement failed");
      delay(200); // Wait before retrying
    }
    Serial.println("Periodic DHT Measurement");
  }

  // Initialize/Deinitialize bluetooth only when flag changes
  if(bluetoothFlag && !bluetoothInit) {
    SerialBT.begin(ID_BLT);
    bluetoothInit = true;
    Serial.println("Bluetooth started");
  } else if(!bluetoothFlag && bluetoothInit) {
    SerialBT.end();
    bluetoothInit = false;
    Serial.println("Bluetooth stopped");
  }

  // Handle bluetooth communication
  if(bluetoothInit && SerialBT.available()) {
    String option = SerialBT.readStringUntil('\n');
    option.trim();

    if(option == "measure") {
      currentMeasureFlag = true;
      if(getDHTMeasurement(&temperature, &humidity)) {
        Serial.println("BT: DHT Measurement success");
      } else {
        Serial.println("BT: DHT Measurement failed");
      }
    } else if(option == "wifipass") {
      // PASS
    } else if(option == "wificssid") {
      // PASS
    } else {
      SerialBT.println("Invalid command");
    }
  }

  // Handle display updates
  if(displayFlag) {
    displayInfo();
  } else if(!displayFlag) {
    u8g2.clearBuffer();
    u8g2.sendBuffer();
  }
}

void displayInfo() {
  for(int i = 0; i < 5; i++) {
    u8g2.clearBuffer();

    char BTStringBuff[30];
    char WiFiStringBuff[30];

    sprintf(BTStringBuff, "Bluetooth: %s", bluetoothFlag ? "Enabled" : "Disabled");
    sprintf(WiFiStringBuff, "WiFi: %s", wifiFlag ? "Enabled" : "Disabled");

    u8g2.setFont(u8g2_font_luBS08_tf);
    u8g2.drawStr(0, 14, BTStringBuff);
    u8g2.setFont(u8g2_font_luRS08_tf);
    u8g2.drawStr(0, 28, WiFiStringBuff);

    if(currentMeasureFlag && i % 2 == 0) {
      u8g2.drawCircle(4, 60, 3);
    }

    u8g2.sendBuffer();
    delay(50);
  }
}