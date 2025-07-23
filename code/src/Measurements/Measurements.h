#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <DHT.h>

// Include hardware config
#include "../config_hardware.h"

// Isolate ArduCam includes
#ifdef ISOLATE_ARDUCAM_HEADERS
#pragma push_macro("USART1_BASE")
#pragma push_macro("GPIOA")
#pragma push_macro("GPIOB")
#undef USART1_BASE
#undef GPIOA
#undef GPIOB
#endif

#include "Arducam_Mega.h"

#ifdef ISOLATE_ARDUCAM_HEADERS
#pragma pop_macro("GPIOB")
#pragma pop_macro("GPIOA")
#pragma pop_macro("USART1_BASE")
#endif

#define DHTPIN   4   // DHT Pin
#define FLASHPIN 27  // Flash Pin
#define CAMCS   7   // Camera CS Pin

void initDHT();
bool getDHTMeasurement(float* temp, float* hum);
bool initCamera();
bool captureImage();

#endif // MEASUREMENTS_H