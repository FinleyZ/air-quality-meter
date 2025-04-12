#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f1xx_hal.h"

typedef struct {
    uint16_t humidity_raw;
    uint16_t temperature_raw;
    uint8_t checksum;
    uint8_t is_valid;
} DHT_Data;

void DHT_SetPinOutput(void);
void DHT_SetPinInput(void);
void DHT_Start(void);
// uint8_t DHT_CheckResponse(void);
// uint8_t DHT_ReadByte(void);
void DHT_ReadResponse(uint16_t GPIO_Pin);
void DHT_Touch(void);
DHT_Data DHT_GetLatestData(void);
void DHT_PrintLatestData(void);
#endif