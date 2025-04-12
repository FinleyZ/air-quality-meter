#include "dht.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal.h"
#include "main.h"
#include <stdio.h>
#include "string.h"

DHT_Data latest_dht_data = {0};

void Timer_Delay_us(uint32_t us);


void DHT_SetPinOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT_DATA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT_DATA_GPIO_Port, &GPIO_InitStruct);
}

void DHT_SetPinInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT_DATA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT_DATA_GPIO_Port, &GPIO_InitStruct);
}

void DHT_Start(void){
    DHT_SetPinOutput();
    HAL_GPIO_WritePin(DHT_DATA_GPIO_Port, DHT_DATA_Pin, GPIO_PIN_RESET);
    HAL_Delay(18); // Delay 18 ms
    // HAL_GPIO_WritePin(DHT_DATA_GPIO_Port, DHT_DATA_Pin, GPIO_PIN_SET);
    DHT_SetPinInput();
}

void DHT_ReadResponse(uint16_t GPIO_Pin){
    static uint32_t last_edge_time = 0;
    static uint16_t pulse_width = 0;
    static uint8_t bit_index = 0;
    static uint8_t byte_index = 0;
    static uint8_t dht_data[5] = {0};
    static enum { WAIT_FOR_RESPONSE_LOW, WAIT_FOR_RESPONSE_HIGH, READING_BITS } state = WAIT_FOR_RESPONSE_LOW;
  
    uint32_t now = Timer_GetMicros();
    GPIO_PinState state_pin = HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin);

    switch (state) {

        case WAIT_FOR_RESPONSE_LOW:
            if (state_pin == GPIO_PIN_RESET) {
                last_edge_time = now;
            } else if (state_pin == GPIO_PIN_SET) {
                pulse_width = now - last_edge_time;
                if (pulse_width >= 60 && pulse_width <= 90) {
                    state = WAIT_FOR_RESPONSE_HIGH;
                    last_edge_time = now;
                }
            }
            break;

        case WAIT_FOR_RESPONSE_HIGH:
            if (state_pin == GPIO_PIN_RESET) {
                pulse_width = now - last_edge_time;
                if (pulse_width >= 60 && pulse_width <= 90) {
                    state = READING_BITS;
                    last_edge_time = now;
                    bit_index = 0;
                    byte_index = 0;
                    memset(dht_data, 0, sizeof(dht_data));
                }
            }
            break;

        case READING_BITS:
            if (state_pin == GPIO_PIN_SET) {
                last_edge_time = now;// start recording the time been set
            } else {
                pulse_width = now - last_edge_time;
                if (pulse_width > 10 && pulse_width < 80) { // calculate either 1 or 0
                    uint8_t bit = (pulse_width > 40) ? 1 : 0;
                    dht_data[byte_index] |= (bit << (7 - bit_index));

                    if (++bit_index >= 8) { //if 1 byte are filled
                        bit_index = 0;
                        if (++byte_index >= 5) { //if 5 byte are filled
                            uint16_t humidity_raw = (dht_data[0] << 8) | dht_data[1];
                            uint16_t temp_raw = (dht_data[2] << 8) | dht_data[3];
                            uint8_t sum = dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3];

                            // printf("Humidity: %d.%d%%  Temp: %d.%d°C\r\n",
                            //        humidity_raw / 10, humidity_raw % 10,
                            //        temp_raw / 10, temp_raw % 10);

                            if (sum == dht_data[4]) {
                                latest_dht_data.humidity_raw = humidity_raw;
                                latest_dht_data.temperature_raw = temp_raw;
                                latest_dht_data.checksum = sum;
                                latest_dht_data.is_valid = 1;
                                // printf("Checksum OK\r\n");
                            } else {
                                latest_dht_data.is_valid = 0;
                                // printf("Checksum FAIL\r\n");
                                printf("Raw data: %X %X %X %X %X\r\n", dht_data[0], dht_data[1], dht_data[2], dht_data[3], dht_data[4]);
                            }
                            // Disable IRQ now
                            HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
                            state = WAIT_FOR_RESPONSE_LOW;
                        }
                    }
                }
            }
            break;
    }
}

DHT_Data DHT_GetLatestData(void) {
    return latest_dht_data;
}



void DHT_Touch(void){
    //Enable interrupt
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    DHT_Start();
    // printf("Start signal sent to DHT11\r\n");
}

void DHT_PrintLatestData(void){
    if (latest_dht_data.is_valid) {
        printf("Checksum OK\r\n");
        printf("Humidity: %d.%d%%  Temp: %d.%d°C\r\n",
               latest_dht_data.humidity_raw / 10,
               latest_dht_data.humidity_raw % 10,
               latest_dht_data.temperature_raw / 10,
               latest_dht_data.temperature_raw % 10);
    } else {
        printf("Checksum FAIL\r\n");
    }
}









//---------------------------------------------------------------------------------------




// void DHT_ReadResponse(void){
//     if (DHT_CheckResponse()) {

//     }
//     else {
//         printf("No response from DHT11.\r\n");
//     }
    
//     if (DHT_CheckResponse()) {
//       uint8_t rh_int = DHT_ReadByte();
//       uint8_t rh_dec = DHT_ReadByte();
//       uint8_t temp_int = DHT_ReadByte();
//       uint8_t temp_dec = DHT_ReadByte();
//       uint8_t checksum = DHT_ReadByte();
//       printf("Humidity: %d.%d%%  Temp: %d.%d°C\r\n", rh_int, rh_dec, temp_int, temp_dec);
//       if (checksum == (rh_int + rh_dec + temp_int + temp_dec)) {
//           printf("Humidity: %d.%d%%  Temp: %d.%d°C\r\n", rh_int, rh_dec, temp_int, temp_dec);
//       } else {
//           printf("Checksum error!\r\n");
//       }
//     } else {
//         printf("No response from DHT11.\r\n");
//     }
// }


// uint8_t DHT_CheckResponse(void) {
//     uint8_t response = 0;

//     // Wait for DHT to pull the line LOW (should happen within 100us)
//     for (uint32_t timeout = 0; timeout < 1000; timeout++) {
//         if (HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin) == GPIO_PIN_RESET) {
//             printf("0 checked\r\n");
//             break;
//         }
//         Timer_Delay_us(1);
//         if (timeout == 99) return 0; // timeout: no LOW detected
//     }


//     // Wait for DHT to release the line and go HIGH again (within 100us)
//     for (uint32_t timeout = 0; timeout < 100; timeout++) {
//         if (HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin) == GPIO_PIN_SET) {
//             printf("1 checked\r\n");
//             response = 1;
//             break;
//         }
//         Timer_Delay_us(1);
//         if (timeout == 99) return 0; // timeout: no HIGH detected
//     }
    
//     return response;
// }


// uint8_t DHT_ReadBit(void) {
//     // Wait for the line to go LOW (start of bit)
//     uint32_t timeout = 0;
//     while (HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin) == GPIO_PIN_RESET) {
//         DWT_Delay_us(1);
//         if (++timeout > 100) return 0;  // timeout waiting for bit start
//     }
//     printf("TIMEOUT: %d \r\n", timeout);

//     // Wait for 35us to reach the middle of the HIGH signal
//     DWT_Delay_us(35);

//     // Sample the bit value
//     if (HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin) == GPIO_PIN_SET) {
//         printf("1");
//         return 1;
//     } else {
//         printf("0");
//         return 0;
//     }
// }


// uint8_t DHT_ReadBit(void){
//     uint32_t timeout;
//     // Wait for the data line to go high (start of bit signal)

//     // 1. Wait for the line to go LOW (start of bit)
//     while (HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin) == GPIO_PIN_SET) {
//         Timer_Delay_us(1);
//         if (++timeout > 100) {
//             return 0;
//         }
//     }
//     // printf("TIMEOUT-----1: %d \r\n", timeout);

//     // 2. Wait for the line to go HIGH (start of bit HIGH pulse)
//     timeout = 0;
//     while (HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin) == GPIO_PIN_RESET) {
//         Timer_Delay_us(1);
//         if (++timeout > 100) {
//             printf("Timeout waiting for HIGH\r\n");
//             return 0;
//         }
//     }
//     // printf("TIMEOUT------0: %d \r\n", timeout);

//     if (timeout<28){

//         printf("0");
//         return 0;
//     }

//     printf("1");
//     return 1;

// }

// uint8_t DHT_ReadByte(void) {
//     uint8_t i, byte = 0;

//     for (i = 0; i < 8; i++) {
        
//         uint8_t current_bit = DHT_ReadBit();
//         byte |= (current_bit << (7 - i));
//     }

//     printf("\r\n");

//     return byte;
// }


// uint8_t DHT_CheckResponse(void) {
//     uint8_t response = 0;

//     // Wait for 40 microseconds
//     DWT_Delay_us(60);

//     // Check if the DHT11 pulls the line LOW
//     if (HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin) == GPIO_PIN_RESET) {
//         // Wait for 80 microseconds
//         printf("0 checked");
//         DWT_Delay_us(50);

//         // Verify that the DHT11 pulls the line HIGH
//         if (HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin) == GPIO_PIN_SET) {
//             printf("1 checked");
//             response = 1;  // DHT11 responded correctly
//         }
//     }
//     return response;
// }

// uint8_t DHT_ReadByte(void) {
//     uint8_t i, byte = 0;

//     for (i = 0; i < 8; i++) {
//         uint32_t timeout = 0;

//         // Wait for line to go LOW (~50us) indicating start of bit
//         while (HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin) == GPIO_PIN_RESET) {
//             if (++timeout > 400) return 0;
//         }

//         timeout = 0;
//         // a 1 bit has an high voltage level duration of 27uS
//         while (HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin) == GPIO_PIN_SET) {
//             if (++timeout > 200) 
//             byte |= (1 << (7 - i));
//             if (++timeout > 800) break;
//         }

//         // 0 bit has an high voltage level duration of 70uS
//         while (HAL_GPIO_ReadPin(DHT_DATA_GPIO_Port, DHT_DATA_Pin) == GPIO_PIN_SET) {
//             if (++timeout > 600) break;
//         } 
        
    
//     }
//     return byte;
// }