#include <BluetoothSerial.h>
#include <U8g2lib.h>
#include <Ticker.h>

#include "config.h"
#include "src/Measurements/Measurements.h"

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, DSCL, DSDA, U8X8_PIN_NONE);
BluetoothSerial SerialBT;
Ticker measureTimer;

volatile bool measureFlag = false;
volatile bool displayFlag = true;
volatile bool bluetoothFlag = true;
volatile bool wifiFlag = true;

volatile unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;

// Track bluetooth state
bool bluetoothInit = false;

float temperature, humidity;

void measureTicker() {
  measureFlag = true;
}

void btnISR() {
  unsigned long currentTime = millis();
  
  // Only process button press if enough time has passed since last press
  if (currentTime - lastButtonPress > debounceDelay) {
    bluetoothFlag = !bluetoothFlag; // Toggle Bluetooth state
    digitalWrite(LEDPIN, bluetoothFlag ? LOW : HIGH); // Turn LED on/off based on Bluetooth state
    Serial.println(bluetoothFlag ? "Bluetooth Enabled" : "Bluetooth Disabled");
    lastButtonPress = currentTime;
  }
}

void setup() {
  Serial.begin(115200);                   // Baud rate for serial communication
  u8g2.begin();                           // Enable display
  u8g2.setFont(u8g2_font_luRS08_tf);      // Set display text font
  initDHT();                              // Initialize DHT sensor

  pinMode (LEDPIN, OUTPUT);               // Set LED pin as output
  digitalWrite(LEDPIN, HIGH);             // Turn on LED

  pinMode(BTNPIN, INPUT_PULLUP);
  attachInterrupt(BTNPIN, btnISR, FALLING);

  measureTimer.attach(10, measureTicker); // Get a measurement every 10 seconds

  Serial.println("Setup Complete");
}

void loop() {
  // Store the flag state for display
  bool currentMeasureFlag = measureFlag;
  
  // Handle periodic measurements
  if(measureFlag) {
    measureFlag = false;
    //while(!getDHTMeasurement(&temperature, &humidity)) {
    //  Serial.println("DHT Measurement failed");
    //}
    Serial.println("Periodic DHT Measurement");
  }

  // Initialize/Deinitialize Bluetooth only when flag changes
  if(bluetoothFlag && !bluetoothInit) {
    SerialBT.begin(BTID);
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
    } else {
      SerialBT.println("Invalid command");
    }
  }

  // Handle display updates
  if(displayFlag) {
    char BTStringBuff[30];
    char WiFiStringBuff[30];

    sprintf(BTStringBuff, "Bluetooth: %s", bluetoothFlag ? "Enabled" : "Disabled");
    sprintf(WiFiStringBuff, "WiFi: %s", wifiFlag ? "Connected" : "Disconnected");

    for(int i = 0; i < 5; i++) {
      u8g2.clearBuffer();
      u8g2.drawStr(0, 14, BTStringBuff);
      u8g2.drawStr(0, 28, WiFiStringBuff);
      if(currentMeasureFlag && i % 2 == 0) {
        u8g2.drawCircle(4, 60, 3);
      }
      u8g2.sendBuffer();
      delay(50);
    }
  }
  
  delay(50);
}