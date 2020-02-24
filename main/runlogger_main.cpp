#include <iostream>
#include <string>
#include <cmath>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi.hpp"
#include "sntp_time_sync.hpp"
#include "run.hpp"
#include "runlogger.hpp"
#include "sntp.h"
#include "minmea.h"
extern "C" {
    #include "mqtt_start.h"    
}

#define METERS_TO_MILES_FACTOR (0.00062137)
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
static const int RX_BUF_SIZE = 1024;
static const char *TAG = "runlogger";
extern volatile uint8_t on_wifi;
extern RTC_DATA_ATTR int boot_count;

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
                        + std::to_string(data.get_distance() * METERS_TO_MILES_FACTOR)
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
                + std::to_string(data.get_distance() * METERS_TO_MILES_FACTOR)
                + ",\n\t"
                + "\"duration\": "
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

static void parse_gps_from_uart_task(void* arg) {
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
                    double lat;
                    double lon;
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
                        lat = minmea_tocoord(&frame.latitude);
                        lon = minmea_tocoord(&frame.longitude);
                        if(!std::isnan(lat) && !std::isnan(lon)){
                            app.handle_new_gps_coord(lat,lon);    
                        }
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
        }
        /* Not sure how much to delay yet. There's a tradeoff between sampling
        rate and percent error in the distance measurement due to the accuracy
        limitations of the GPS system. For instance, if you sample repeatedly
        while the device is in the same position, GPS tolerance will make the
        distance calculation continue to accrue roughly 10m per sample, in the
        worst case. Sampling less frequently reduces this error, but it also 
        reduces the overall measuring accuracy of your running route.*/
        vTaskDelay(1000);
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


    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();


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
    xTaskCreate(parse_gps_from_uart_task, "parse_gps_from_uart_task", 2048, NULL, 10, NULL);
}
