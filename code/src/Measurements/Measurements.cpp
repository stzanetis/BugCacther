#include <DHT.h>

#include "Measurements.h"
#include "../../config.h"

DHT dht(DHTPIN, DHT11);

void initDHT() {
  dht.begin();  // Initialize the DHT sensor
  delay(500);   // DHT sensor delay for stability
}

bool getDHTMeasurement(float* temp, float* hum) {
  *temp = dht.readTemperature();  // Get temperature in Celsius
  *hum = dht.readHumidity();      // Get humidity in percentage

  // Check if the readings are valid
  if (isnan(*temp) || isnan(*hum)) {
    return false;
  }

  return true;
}