#include <DHT.h>

#define DHTPIN  4   // DHT Pin

#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

void initDHT();
bool getDHTMeasurement(float* temp, float* hum);

#endif // MEASUREMENTS_H