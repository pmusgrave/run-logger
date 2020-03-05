#include "unity.h"
#include <iostream>
#include <ctime>
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <sys/time.h>
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"
#include "wifi.hpp"
#include "sntp_time_sync.hpp"

static const char *TAG = "runlogger";

void initialize_wifi() {
	//Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
}

void sync_time() {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    else {
        // add 500 ms error to the current system time.
        // Only to demonstrate a work of adjusting method!
        {
            ESP_LOGI(TAG, "Add a error for test adjtime");
            struct timeval tv_now;
            gettimeofday(&tv_now, NULL);
            int64_t cpu_time = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
            int64_t error_time = cpu_time + 500 * 1000L;
            struct timeval tv_error = { .tv_sec = error_time / 1000000L, .tv_usec = error_time % 1000000L };
            settimeofday(&tv_error, NULL);
        }

        ESP_LOGI(TAG, "Time was set, now just adjusting it. Use SMOOTH SYNC method.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
#endif

    char strftime_buf[64];

    // Set timezone to Eastern Standard Time and print local time
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in New York is: %s", strftime_buf);

    if (sntp_get_sync_mode() == SNTP_SYNC_MODE_SMOOTH) {
        struct timeval outdelta;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS) {
            adjtime(NULL, &outdelta);
            ESP_LOGI(TAG, "Waiting for adjusting time ... outdelta = %li sec: %li ms: %li us",
                        (long)outdelta.tv_sec,
                        outdelta.tv_usec/1000,
                        outdelta.tv_usec%1000);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
}


#define START_PAUSE_BUTTON 14
#define ESP_INTR_FLAG_DEFAULT 0

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

int32_t get_diff_ms(struct timeval *a, struct timeval *b) {
    struct timeval result;
    result.tv_sec = a->tv_sec - b->tv_sec;
    result.tv_usec = a->tv_usec - b->tv_usec;
    if (a->tv_usec < b->tv_usec) {
        result.tv_sec -= 1;
        result.tv_usec += 1000000;
    }
    return (result.tv_sec * 1000 + (result.tv_usec / 1000));
}

static void handle_start_pause_button(void* arg) {
    struct timeval last_press;
    struct timeval now;
    gettimeofday(&last_press, NULL);
    uint32_t io_num;
    int button_state = 1;
    int32_t diff;

    while (1) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            gettimeofday(&now, NULL);
            diff = get_diff_ms(&now, &last_press);
            
            button_state = gpio_get_level((gpio_num_t)io_num);
            if ( (button_state == 0) && ( diff > 1000 ) ) {
                printf("GPIO[%d] intr, val: %d\n", (gpio_num_t)io_num, button_state);
                // gpio_set_level((gpio_num_t)13, 1);
                last_press = now;
            }
            else {
                printf("%d\n", diff);
            }
            vTaskDelay(1);
        }
    }
}

extern "C" void app_main(void) {
    initialize_wifi();
    sync_time();

    std::cout << "Initializing I/O" << std::endl;
    // configure start/pause button
    gpio_config_t io_conf;
    io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL<<START_PAUSE_BUTTON);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = (gpio_pullup_t)1;
    gpio_config(&io_conf);

    gpio_pad_select_gpio((gpio_num_t)13);
    gpio_set_direction((gpio_num_t)13, GPIO_MODE_OUTPUT);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(handle_start_pause_button, "handle_start_pause_button", 2048, NULL, 10, NULL);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add((gpio_num_t)START_PAUSE_BUTTON, gpio_isr_handler, (void*) (gpio_num_t)START_PAUSE_BUTTON);

    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();    
}
