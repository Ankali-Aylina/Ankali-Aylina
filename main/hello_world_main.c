/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"

#define CHIP_NAME "ESP32"

static const char *TAG = "MAIN APP";

void app_main(void)
{
    printf("Hello world!\n");

    /* 打印芯片信息 */
    esp_chip_info_t chip_info;  //芯片信息结构体
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CHIP_NAME,
            chip_info.cores,    //CPU核数
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",    //CHIP_FEATURE_X功能标志的位掩码  芯片拥有蓝牙经典
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : ""); //CHIP_FEATURE_X功能标志的位掩码  芯片具有蓝牙LE

    printf("silicon revision %d, ", chip_info.revision);    //芯片修订版号

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");   //CHIP_FEATURE_X功能标志的位掩码  芯片具有嵌入式闪存

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size()); //可用的最小可用堆

    ESP_LOGI(TAG, "system init V1.1");

    while(1){
        printf("system run ...\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
