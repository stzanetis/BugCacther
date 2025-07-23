#include "Measurements.h"
#include "../../config.h"

DHT dht(DHTPIN, DHT11);
Arducam_Mega myCAM(CAMCS);

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

bool initCamera() {
  Serial.println("Initializing camera...");
  if (myCAM.begin() == 0) {
    myCAM.setImageQuality(HIGH_QUALITY); // Best quality
    return true;
  } else {
    return false;
  }
}

bool captureImage() {
  if (myCAM.takePicture(CAM_IMAGE_MODE_WQXGA2, CAM_IMAGE_PIX_FMT_JPG) != 0) {
    return false;
  }
  return true;
}