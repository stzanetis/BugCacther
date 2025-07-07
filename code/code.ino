#include <BluetoothSerial.h>
#include <U8g2lib.h>
#include <DHT.h>
#include <Ticker.h>

#define DHTPIN 4
#define DSDA 22
#define DSCL 20

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, DSCL, DSDA, U8X8_PIN_NONE);
DHT dht(DHTPIN, DHT11);
BluetoothSerial SerialBT; // Bluetooth Serial object
Ticker dhtTimer;          // Timer for DHT measurement

volatile bool dhtFlag = false;
float temperature;
float humidity;

// DHT Timer ISR
void TickerDHT() {
  dhtFlag = true;
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  SerialBT.begin("BugCatcher-BT");

  dhtTimer.attach(5, TickerDHT);

  Serial.println("Setup Complete");
}

void loop() {
  if (dhtFlag) {
    dhtFlag = false;
    if (getDHTMeasurement()) {
      Serial.println("Periodic DHT Measurement");
    }
  }

  if (SerialBT.available()) {
    String option = SerialBT.readStringUntil('\n');
    option.trim();

    if (option == "measure") {
      bool success = getDHTMeasurement();
      if (success) {
        u8g2.clearBuffer();
        u8g2.drawStr(0, 10, "DHT Success");
        u8g2.sendBuffer();
      } else {
        u8g2.clearBuffer();
        u8g2.drawStr(0, 10, "DHT Failed");
        u8g2.sendBuffer();
      }
    } else {
      SerialBT.println("Invalid Argument");
    }
  }
}

bool getDHTMeasurement() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return false;
  }

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%  Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");
  return true;
}