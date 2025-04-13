#include "main.h"
#include "string.h"
#include "stdio.h"



///TODO: add states check what comend is for HAL_UART_RxCpltCallback for each commend.


//Current ID 0x1D + 0xE3
extern UART_HandleTypeDef huart1;
uint8_t sds_response_ready = 0;
uint8_t sds_command_waiting = 0;
uint8_t SDS_receivedData[10];

uint8_t SDS_sleepModeCommand[19] = {
    0xAA, 0xB4, 0x06, 0x01, 
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x1D, 
    0xE3, 0x07, 0xAB};
uint8_t SDS_workModeCommand[19] = {
    0xAA, 0xB4, 0x06, 0x01, 
    0x01, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x1D, 
    0xE3, 0x08, 0xAB};
uint8_t SDS_activeModeCommand[19] = {
    0xAA, 0xB4, 0x02, 0x01, 
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x1D, 
    0xE3, 0x03, 0xAB
  };
uint8_t SDS_queryModeCommand[19] = {
    0xAA, 0xB4, 0x02, 0x01, 
    0x01, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x1D, 
    0xE3, 0x04, 0xAB
  };
uint8_t SDS_queryCurrentWorkModeCommand[19] = {
    0xAA, 0xB4, 0x02, 0x00, 
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x1D, 
    0xE3, 0x02, 0xAB
    };

uint8_t SDS_queryDataCommand[19] = {
    0xAA, 0xB4, 0x04, 0x00, 
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0xFF, 
    0xFF, 0x02, 0xAB
  };

// uint8_t SDS_sleepModeResponse[10] ={0xAA, 0xC5, 0x06, 0x01, 0x00, 0x00, 0x1D, 0xE3, 0x08, 0xAB}



void SDS_logReceivedData() {
    printf("SDS Received Data: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
        SDS_receivedData[0], 
        SDS_receivedData[1], 
        SDS_receivedData[2],
        SDS_receivedData[3], 
        SDS_receivedData[4], 
        SDS_receivedData[5],
        SDS_receivedData[6], 
        SDS_receivedData[7], 
        SDS_receivedData[8],
        SDS_receivedData[9]
      );
}

void SDS_logPMData(){
    uint16_t pm25_raw = (SDS_receivedData[3] << 8) | SDS_receivedData[2];
    uint16_t pm10_raw = (SDS_receivedData[5] << 8) | SDS_receivedData[4];
    printf("PM2.5: %d.%d µg/m³, PM10: %d.%d µg/m³\r\n",
        pm25_raw / 10, pm25_raw % 10,
        pm10_raw / 10, pm10_raw % 10);
}


void SDS_wait(void) {
    uint32_t timeout = HAL_GetTick() + 100;
    while (!sds_response_ready && HAL_GetTick() < timeout);
    
    if (sds_response_ready) {
        // validate data
    } else {
        printf("Timeout waiting for SDS011 response\r\n");
    }
}

void SDS_getData(void) {
    // Validate frame header and tail
    if (SDS_receivedData[0] == 0xAA &&
        SDS_receivedData[1] == 0xC0 &&
        SDS_receivedData[9] == 0xAB) {
        SDS_logPMData();
    } else {
        // printf("❌ Invalid SDS011 query data response\r\n");
        // SDS_logReceivedData();
    }
}

//set to sleep mode
void SDS_setSleepMode(void) {
    sds_response_ready = 0;
    sds_command_waiting = 1;
    HAL_UART_Receive_DMA(&huart1, SDS_receivedData, 10);  // prepare for response
    HAL_UART_Transmit(&huart1, SDS_sleepModeCommand, 19, HAL_MAX_DELAY);
    
    // SDS_wait();
    if (
        SDS_receivedData[0] == 0xAA &&
        SDS_receivedData[1] == 0xC5 &&
        SDS_receivedData[2] == 0x06 &&
        SDS_receivedData[3] == 0x01 &&
        SDS_receivedData[4] == 0x00
        // && SDS_receivedData[9] == 0xAB //seems bug here.
    ) {
        printf("✅ SDS011 set to sleep mode\r\n");
        // SDS_logReceivedData();
    } else {
        printf("❌ Invalid SDS011 sleep response\r\n");
        // SDS_logReceivedData();
    }
}

//set to work mode
void SDS_setWorkMode(void) {
    sds_response_ready = 0;
    sds_command_waiting = 1;
    HAL_UART_Receive_DMA(&huart1, SDS_receivedData, 10);
    HAL_UART_Transmit(&huart1, SDS_workModeCommand, 19,HAL_MAX_DELAY);

    // SDS_wait();
    if (
        SDS_receivedData[0] == 0xAA &&
        SDS_receivedData[1] == 0xC5 &&
        SDS_receivedData[2] == 0x06 &&
        SDS_receivedData[3] == 0x01 &&
        SDS_receivedData[4] == 0x01 &&
        SDS_receivedData[9] == 0xAB
    ) {
        printf("✅ SDS011 set to work mode\r\n");
        // SDS_logReceivedData();
    } else {
        printf("❌ Invalid SDS011 work response\r\n");
        // SDS_logReceivedData();
    }

}


// set to active reporting mode
void SDS_setActiveReportingMode(void) {
    sds_response_ready = 0;
    sds_command_waiting = 1;
    HAL_UART_Receive_DMA(&huart1, SDS_receivedData, 10);
    HAL_UART_Transmit(&huart1, SDS_activeModeCommand, 19, HAL_MAX_DELAY);

    // SDS_wait();
    if (
        SDS_receivedData[0] == 0xAA &&
        SDS_receivedData[1] == 0xC5 &&
        SDS_receivedData[2] == 0x02 &&
        SDS_receivedData[3] == 0x01 &&
        SDS_receivedData[4] == 0x01 &&
        SDS_receivedData[9] == 0xAB
    ) {
        printf("✅ SDS011 set to active reporting mode\r\n");
    } else {
        printf("❌ Invalid SDS011 active mode response\r\n");
    }
}

// set to query reporting mode
void SDS_setQueryReportingMode() {
    sds_response_ready = 0;
    sds_command_waiting = 1;
    HAL_UART_Receive_DMA(&huart1, SDS_receivedData, 10);
    HAL_UART_Transmit(&huart1, SDS_queryModeCommand, 19, HAL_MAX_DELAY);

    // SDS_wait();
    if (
        SDS_receivedData[0] == 0xAA &&
        SDS_receivedData[1] == 0xC5 &&
        SDS_receivedData[2] == 0x02 &&
        SDS_receivedData[3] == 0x01 &&
        SDS_receivedData[4] == 0x01 &&
        SDS_receivedData[9] == 0xAB
    ) {
        printf("✅ SDS011 set to query reporting mode\r\n");
    } else {
        printf("❌ Invalid SDS011 query mode response\r\n");
        // SDS_logReceivedData();
    }
}

// query current work mode
void SDS_queryCurrentWorkMode() {
    sds_response_ready = 0;
    sds_command_waiting = 1;
    HAL_UART_Receive_DMA(&huart1, SDS_receivedData, 10);
    HAL_UART_Transmit(&huart1, SDS_queryCurrentWorkModeCommand, 19, HAL_MAX_DELAY);

    // SDS_wait();
    if (
        SDS_receivedData[0] == 0xAA &&
        SDS_receivedData[1] == 0xC5 &&
        SDS_receivedData[2] == 0x02 &&
        SDS_receivedData[3] == 0x00 &&
        SDS_receivedData[9] == 0xAB
    ) {
        if (SDS_receivedData[4] == 0x00) {
            printf("SDS011 is in query mode\r\n");
        } else if (SDS_receivedData[4] == 0x01) {
            printf("SDS011 is in active mode\r\n");
        } else {
            printf("❓ SDS011 returned unknown mode byte: 0x%02X\r\n", SDS_receivedData[4]);
        }
    } else {
        printf("❌ Invalid SDS011 mode query response\r\n");
        // SDS_logReceivedData();
    }
}

void SDS_queryData() {
    sds_response_ready = 0;
    sds_command_waiting = 1;

    HAL_UART_Receive_DMA(&huart1, SDS_receivedData, 10);
    HAL_UART_Transmit(&huart1, SDS_queryDataCommand, 19, HAL_MAX_DELAY);

    // SDS_wait();
    // if (SDS_receivedData[0] == 0xAA &&
    //     SDS_receivedData[1] == 0xC0 &&
    //     SDS_receivedData[9] == 0xAB) {
    //     SDS_logPMData();
    // } else {
    //     printf("❌ Invalid SDS011 query data response\r\n");
    //     // SDS_logReceivedData();
    // }
}

