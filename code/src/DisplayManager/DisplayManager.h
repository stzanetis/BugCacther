#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <U8g2lib.h>

#define SDAPIN  22  // I2C Display SDA
#define SCLPIN  20  // I2C Display SCL

extern U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2;

struct DisplayData {
  char** statusBluetooth;  // Pointer to char pointer
  char** statusWiFi;       // Pointer to char pointer
  float* temperature;
  float* humidity;
  volatile bool* flagCurrentMeasure;
  volatile bool* stopDisplayTask;
  TaskHandle_t* displayTaskHandle;
};

void initDisplay();
void stopDisplay();
void displayInfo(void *parameter);

#endif