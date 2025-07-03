#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "../lib/nrf24l01/nrf24l01.h"
#include <stdio.h>
#include "espressif/esp8266/uart_register.h"

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;
    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void task_blink(void* ignore)
{
    gpio16_output_conf();
    while(true) {
    	gpio16_output_set(0);
        vTaskDelay(1000/portTICK_RATE_MS);
        printf("Blink\n");
    	gpio16_output_set(1);
        vTaskDelay(1000/portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

void task_nrf24_receive(void* ignore)
{
    printf("Setting up nRF24L01 as receiver...\n");
    nrf24_device(RECEIVER, RESET);
    printf("nRF24L01 setup complete.\n");
    nrf24_prx_static_payload_width(32, 1);
    uint8_t payload[32];
    while (true) {
        printf("Attempting to receive...\n");
        uint8_t result = nrf24_receive(payload, 32);
        printf("Receive result: %d\n", result);
        if (result == OPERATION_DONE) {
            printf("Received: ");
            for (int i = 0; i < 32; i++) {
                printf("%02X ", payload[i]);
            }
            printf("\n");
        } else if (result == OPERATION_ERROR) {
            printf("Error in receiving\n");
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    // Set UART0 baud rate to 115200
    WRITE_PERI_REG(UART_CLKDIV(0), UART_CLK_FREQ / 115200);
    
    // Test print to verify baud rate
    printf("Start of program\n");
    
    xTaskCreate(&task_blink, "startup", 2048, NULL, 1, NULL);
    xTaskCreate(&task_nrf24_receive, "nrf24_rx", 2048, NULL, 1, NULL);
}