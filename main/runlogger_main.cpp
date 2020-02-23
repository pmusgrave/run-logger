#include <iostream>
#include <string>
#include "driver/gpio.h"
#include "run.hpp"
#include "runlogger.hpp"
#include "sntp.h"
extern "C" {
    #include "mqtt_start.h"    
}


/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "driver/uart.h"
#include "minmea.h"
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
static const int RX_BUF_SIZE = 1024;

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      ""
#define EXAMPLE_ESP_WIFI_PASS      ""
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "runlogger";

static int s_retry_num = 0;

volatile uint8_t on_wifi = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {};
    wifi_config.sta = {};
    uint8_t ssid_init_buf[32];
    memcpy(ssid_init_buf, EXAMPLE_ESP_WIFI_SSID, sizeof(EXAMPLE_ESP_WIFI_SSID)/sizeof(char));
    memcpy(wifi_config.sta.ssid, ssid_init_buf, 32);
    uint8_t password_init_buf[64];
    memcpy(password_init_buf, EXAMPLE_ESP_WIFI_PASS, sizeof(EXAMPLE_ESP_WIFI_PASS)/sizeof(char));
    memcpy(wifi_config.sta.password, password_init_buf, 64);
        
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        on_wifi = 1;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        on_wifi = 0;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}

/******************************************************************************
*   SNTP EXAMPLE SOURCE
******************************************************************************/
/* LwIP SNTP example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <time.h>
#include <sys/time.h>
#include "esp_attr.h"
#include "esp_sleep.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;

static void obtain_time(void);
static void initialize_sntp(void);

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
void sntp_sync_time(struct timeval *tv)
{
   settimeofday(tv, NULL);
   ESP_LOGI(TAG, "Time is synchronized from custom code");
   sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
#endif

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void obtain_time(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );

    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);

    // ESP_ERROR_CHECK( example_disconnect() );
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
}
/*****************************************************************************/

#define ESP_INTR_FLAG_DEFAULT 0

RunLogger app;
esp_mqtt_client_handle_t mqtt_client;
static xQueueHandle start_evt_queue = NULL;
static xQueueHandle stop_evt_queue = NULL;
static xQueueHandle reset_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    switch(gpio_num) {
    case START_PAUSE_BUTTON:
        xQueueSendFromISR(start_evt_queue, &gpio_num, NULL);    
        break;
    case STOP_PAUSE_BUTTON:
        xQueueSendFromISR(stop_evt_queue, &gpio_num, NULL);    
        break;
    case RESET_PAUSE_BUTTON:
        xQueueSendFromISR(reset_evt_queue, &gpio_num, NULL);    
        break;
    }
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

static void blink(void* arg) {
    while(1) {
        if (app.get_state() == PAUSED) {
            gpio_set_level((gpio_num_t)LED, 0);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            gpio_set_level((gpio_num_t)LED, 1);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        else {
            gpio_set_level((gpio_num_t)LED, 0);
        }
        vTaskDelay(1);
    }
}

static void start_button_task(void* arg) {
    struct timeval last_press;
    struct timeval now;
    gettimeofday(&last_press, NULL);
    uint32_t io_num;
    int button_state = 1;
    int32_t diff;

    //testing
    Run data;
    ///
    while (1) {
        if(xQueueReceive(start_evt_queue, &io_num, portMAX_DELAY)) {
            if (io_num == START_PAUSE_BUTTON) {
                gettimeofday(&now, NULL);
                diff = get_diff_ms(&now, &last_press);
                button_state = gpio_get_level((gpio_num_t)io_num);
                if ( (button_state == 0) && ( diff > 1000 ) ) {
                    std::cout << "Time: "
                        << app.current_run.get_duration().tv_sec
                        << "."
                        << app.current_run.get_duration().tv_usec 
                        << " seconds"
                        << std::endl;
                    app.handle_start_pause_button();
                    last_press = now;


                    // TESTING
                    ESP_LOGI(TAG, "Synchronizing data...");
                    data = app.current_run;
                    struct tm date_local_time = data.get_start_date();
                    char buffer[100];
                    sprintf(buffer, "%04d-%02d-%02d", 
                        date_local_time.tm_year + 1900,
                        date_local_time.tm_mon + 1,
                        date_local_time.tm_mday);
                    std::string date(buffer);
                    std::string output_message = 
                        std::string("{\n\t")
                        + "\"date\":"
                        + " \"" + date + "\""
                        + ",\n\t"
                        + "\"distance\": "
                        + std::to_string(data.get_distance())
                        + ",\n\t"
                        + "\"duration\": "
                        + std::to_string(data.get_duration().tv_sec)
                        + "."
                        + std::to_string(data.get_duration().tv_sec)
                        + "\n}";
                    
                    ESP_LOGI(TAG, "Publishing run data via MQTT...");
                    esp_mqtt_client_publish(mqtt_client, "run", output_message.c_str(), 0, 0, 0);
                    ////
                }
            }
        }
        vTaskDelay(1);
    }
}

static void stop_button_task(void* arg) {
    struct timeval last_press;
    struct timeval now;
    gettimeofday(&last_press, NULL);
    uint32_t io_num;
    int button_state = 1;
    int32_t diff;

    while (1) {
        if(xQueueReceive(stop_evt_queue, &io_num, portMAX_DELAY)) {
            gettimeofday(&now, NULL);
            diff = get_diff_ms(&now, &last_press);
            button_state = gpio_get_level((gpio_num_t)io_num);
            if ( (button_state == 0) && ( diff > 1000 ) ) {
                app.handle_stop_button();
                last_press = now;
            }
        }
        taskYIELD();
    }
}

static void reset_button_task(void* arg) {
    struct timeval last_press;
    struct timeval now;
    gettimeofday(&last_press, NULL);
    uint32_t io_num;
    int button_state = 1;
    int32_t diff;

    while (1) {
        if(xQueueReceive(reset_evt_queue, &io_num, portMAX_DELAY)) {
            gettimeofday(&now, NULL);
            diff = get_diff_ms(&now, &last_press);
            button_state = gpio_get_level((gpio_num_t)io_num);
            if ( (button_state == 0) && ( diff > 1000 ) ) {
                app.handle_reset_button();
                last_press = now;
            }
        }
        taskYIELD();
    }
}

static void synchronize_log(void* pvParameter) {
    Run data;
    while (1) {
        if (on_wifi && app.log.size() > 0) {
            ESP_LOGI(TAG, "Synchronizing data...");
            data = app.log.back();
            std::string output_message = 
                std::string("{\n\t")
                + "distance: "
                + std::to_string(data.get_distance())
                + ",\n\t"
                + "duration: "
                + std::to_string(data.get_duration().tv_sec)
                + "."
                + std::to_string(data.get_duration().tv_sec)
                + "\n}";
            
            ESP_LOGI(TAG, "Publishing run data via MQTT...");
            esp_mqtt_client_publish(mqtt_client, "run", output_message.c_str(), 0, 0, 0);
            app.log.pop_back();
        }

        taskYIELD();
    }
}

static void uart_rx_task(void* arg) {
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    char* line = (char*) malloc(RX_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            // printf("%s\n", data);
            // ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }

        line = strtok((char*)data,"\r\n");
        while (line != NULL) {
            switch (minmea_sentence_id((char*)(line), false)) {
                case MINMEA_SENTENCE_RMC: {
                    struct minmea_sentence_rmc frame;
                    if (minmea_parse_rmc(&frame, (const char*)line)) {
                        printf("$xxRMC: raw coordinates and speed: (%d/%d,%d/%d) %d/%d\n",
                                frame.latitude.value, frame.latitude.scale,
                                frame.longitude.value, frame.longitude.scale,
                                frame.speed.value, frame.speed.scale);
                        printf("$xxRMC fixed-point coordinates and speed scaled to three decimal places: (%d,%d) %d\n",
                                minmea_rescale(&frame.latitude, 1000),
                                minmea_rescale(&frame.longitude, 1000),
                                minmea_rescale(&frame.speed, 1000));
                        printf("$xxRMC floating point degree coordinates and speed: (%f,%f) %f\n",
                                minmea_tocoord(&frame.latitude),
                                minmea_tocoord(&frame.longitude),
                                minmea_tofloat(&frame.speed));
                    }
                    else {
                        printf("$xxRMC sentence is not parsed\n");
                    }
                } break;

                case MINMEA_SENTENCE_GGA: {
                    struct minmea_sentence_gga frame;
                    if (minmea_parse_gga(&frame, (const char*)line)) {
                        printf("$xxGGA: fix quality: %d\n", frame.fix_quality);
                    }
                    else {
                        printf("$xxGGA sentence is not parsed\n");
                    }
                } break;

                case MINMEA_SENTENCE_GST: {
                    struct minmea_sentence_gst frame;
                    if (minmea_parse_gst(&frame, (const char*)line)) {
                        printf("$xxGST: raw latitude,longitude and altitude error deviation: (%d/%d,%d/%d,%d/%d)\n",
                                frame.latitude_error_deviation.value, frame.latitude_error_deviation.scale,
                                frame.longitude_error_deviation.value, frame.longitude_error_deviation.scale,
                                frame.altitude_error_deviation.value, frame.altitude_error_deviation.scale);
                        printf("$xxGST fixed point latitude,longitude and altitude error deviation"
                               " scaled to one decimal place: (%d,%d,%d)\n",
                                minmea_rescale(&frame.latitude_error_deviation, 10),
                                minmea_rescale(&frame.longitude_error_deviation, 10),
                                minmea_rescale(&frame.altitude_error_deviation, 10));
                        printf("$xxGST floating point degree latitude, longitude and altitude error deviation: (%f,%f,%f)",
                                minmea_tofloat(&frame.latitude_error_deviation),
                                minmea_tofloat(&frame.longitude_error_deviation),
                                minmea_tofloat(&frame.altitude_error_deviation));
                    }
                    else {
                        printf("$xxGST sentence is not parsed\n");
                    }
                } break;

                case MINMEA_SENTENCE_GSV: {
                    struct minmea_sentence_gsv frame;
                    if (minmea_parse_gsv(&frame, (const char*)line)) {
                        printf("$xxGSV: message %d of %d\n", frame.msg_nr, frame.total_msgs);
                        printf("$xxGSV: sattelites in view: %d\n", frame.total_sats);
                        for (int i = 0; i < 4; i++)
                            printf("$xxGSV: sat nr %d, elevation: %d, azimuth: %d, snr: %d dbm\n",
                                frame.sats[i].nr,
                                frame.sats[i].elevation,
                                frame.sats[i].azimuth,
                                frame.sats[i].snr);
                    }
                    else {
                        printf("$xxGSV sentence is not parsed\n");
                    }
                } break;

                case MINMEA_SENTENCE_VTG: {
                   struct minmea_sentence_vtg frame;
                   if (minmea_parse_vtg(&frame, (const char*)line)) {
                        printf("$xxVTG: true track degrees = %f\n",
                               minmea_tofloat(&frame.true_track_degrees));
                        printf("        magnetic track degrees = %f\n",
                               minmea_tofloat(&frame.magnetic_track_degrees));
                        printf("        speed knots = %f\n",
                                minmea_tofloat(&frame.speed_knots));
                        printf("        speed kph = %f\n",
                                minmea_tofloat(&frame.speed_kph));
                   }
                   else {
                        printf("$xxVTG sentence is not parsed\n");
                   }
                } break;

                case MINMEA_SENTENCE_ZDA: {
                    struct minmea_sentence_zda frame;
                    if (minmea_parse_zda(&frame, (const char*)line)) {
                        printf("$xxZDA: %d:%d:%d %02d.%02d.%d UTC%+03d:%02d\n",
                               frame.time.hours,
                               frame.time.minutes,
                               frame.time.seconds,
                               frame.date.day,
                               frame.date.month,
                               frame.date.year,
                               frame.hour_offset,
                               frame.minute_offset);
                    }
                    else {
                        printf("$xxZDA sentence is not parsed\n");
                    }
                } break;

                case MINMEA_INVALID: {
                    printf("$xxxxx sentence is not valid\n");
                } break;

                default: {
                    printf("$xxxxx sentence is not parsed\n");
                } break;
            }

            line = strtok(NULL,"\r\n");
            
            vTaskDelay(50);
        }
    }
    free(data);
}



extern "C" void app_main(void)
{

    std::cout << "Initializing UART" << std::endl;
    const int uart_num = UART_NUM_1;
    uart_config_t uart_config = {
        9600,
        UART_DATA_8_BITS,
        UART_PARITY_DISABLE,
        UART_STOP_BITS_1,
        UART_HW_FLOWCTRL_DISABLE,
        0
    };
    uart_driver_install(uart_num, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);


    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();


    /**************************************************************************
    *   SNTP EXAMPLE MAIN
    **************************************************************************/
    ++boot_count;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

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

    //const int deep_sleep_sec = 10;
    //ESP_LOGI(TAG, "Entering deep sleep for %d seconds", deep_sleep_sec);
    //esp_deep_sleep(1000000LL * deep_sleep_sec);
    /*************************************************************************/

    mqtt_start(&mqtt_client);

    app.init_io();

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add((gpio_num_t)START_PAUSE_BUTTON, gpio_isr_handler, (void*) (gpio_num_t)START_PAUSE_BUTTON);
    start_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    stop_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    reset_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(blink, "blink", 2048, NULL, 10, NULL);
    xTaskCreate(start_button_task, "start_button", 8192, NULL, 10, NULL);
    xTaskCreate(stop_button_task, "stop_button", 2048, NULL, 10, NULL);
    xTaskCreate(reset_button_task, "reset_button", 2048, NULL, 10, NULL);
    xTaskCreate(synchronize_log, "synchronize_log", 2048, NULL, 10, NULL);
    xTaskCreate(uart_rx_task, "uart_rx_task", 2048, NULL, 10, NULL);

    // Initialize task to synchronize logged data
    // TaskHandle_t xHandle = NULL;
    // xTaskCreate( synchronize_log, "SYNCHRONIZE_LOG", 2048, NULL, tskIDLE_PRIORITY, &xHandle );
    // configASSERT( xHandle );
}
