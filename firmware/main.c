#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"

#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "driver/gpio.h"
#include "driver/ledc.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "wifi_connect.h"
#include "pwm.h"

#define TAG "Xbox360_car"

#define ena_right  14
#define in1_right  16
#define in2_right  27
#define in3_left  2
#define in4_left  4
#define enb_left  13

#define right_channel  0
#define left_channel  1

#define PHONE

#ifdef HOME
    #define WIFI_SSID ""
    #define WIFI_PASSWORD ""
    #define IP_server ""
#elif defined PHONE
    #define WIFI_SSID ""
    #define WIFI_PASSWORD ""
    #define IP_server ""
#elif defined OFFICE
    #define WIFI_SSID ""
    #define WIFI_PASSWORD ""
    #define IP_server ""
#endif

#define PORT 8000
#define my_delay(time) vTaskDelay((time) / portTICK_PERIOD_MS)

QueueHandle_t queue;
int32_t message[4];

void naviguate(int right, int left){
    if(right == 0 && left == 0){
        // Stop all motors
        gpio_set_level(in1_right, 0);
        gpio_set_level(in2_right, 0);
        gpio_set_level(in3_left, 0);
        gpio_set_level(in4_left, 0);
    }else if (right == 1 && left == 0){
        // Turn right
        gpio_set_level(in1_right, 0);
        gpio_set_level(in2_right, 0);
        gpio_set_level(in3_left, 1);
        gpio_set_level(in4_left, 0);
    }else if (right == 0 && left == 1){
        // Turn left
        gpio_set_level(in1_right, 1);
        gpio_set_level(in2_right, 0);
        gpio_set_level(in3_left, 0);
        gpio_set_level(in4_left, 0);
    }else if (right == 1 && left == 1){
        // Move forward
        gpio_set_level(in1_right, 1);
        gpio_set_level(in2_right, 0);
        gpio_set_level(in3_left, 1);
        gpio_set_level(in4_left, 0);
    }else if (right == -1 && left == -1){
        // Move backward
        gpio_set_level(in1_right, 0);
        gpio_set_level(in2_right, 1);
        gpio_set_level(in3_left, 0);
        gpio_set_level(in4_left, 1);
    }else if (right == -1 && left == 0){
        // Reverse right turn
        gpio_set_level(in1_right, 0);
        gpio_set_level(in2_right, 1);
        gpio_set_level(in3_left, 0);
        gpio_set_level(in4_left, 0);
    }else if (right == 0 && left == -1){
        // Reverse left turn
        gpio_set_level(in1_right, 0);
        gpio_set_level(in2_right, 0);
        gpio_set_level(in3_left, 0);
        gpio_set_level(in4_left, 1);
    }else if (right == 1 && left == -1){
        // Spin clockwise
        gpio_set_level(in1_right, 1);
        gpio_set_level(in2_right, 0);
        gpio_set_level(in3_left, 0);
        gpio_set_level(in4_left, 1);
    }else if (right == -1 && left == 1){
        // Spin counter-clockwise
        gpio_set_level(in1_right, 0);
        gpio_set_level(in2_right, 1);
        gpio_set_level(in3_left, 1);
        gpio_set_level(in4_left, 0);
    }else{
        // Default: stop all motors
        gpio_set_level(in1_right, 0);
        gpio_set_level(in2_right, 0);
        gpio_set_level(in3_left, 0);
        gpio_set_level(in4_left, 0);
    }
}

void car_control_task(){
    gpio_set_direction(in1_right, GPIO_MODE_OUTPUT);
    gpio_set_direction(in2_right, GPIO_MODE_OUTPUT);
    gpio_set_direction(in3_left, GPIO_MODE_OUTPUT);
    gpio_set_direction(in4_left, GPIO_MODE_OUTPUT);     
    set_up(right_channel ,ena_right);
    set_up(left_channel ,enb_left);
    while (1) {
        if(xQueueReceive(queue, &(message), 0)){
            naviguate((int) message[0], (int) message[1]);
            analog_write(right_channel ,(int) message[2]);
            analog_write(left_channel ,(int) message[3]);
        }else{
            my_delay(50);  
        }
    }
}

void tcp_conn_task(){
    while (true){
        int my_sock = -1;
        int error = -1;
        struct sockaddr_in dest_addr = {0};
        dest_addr.sin_len = 0;
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        dest_addr.sin_addr.s_addr = inet_addr(IP_server);
        my_sock = socket(AF_INET, SOCK_STREAM, 0);

        if (my_sock < 0) {
            ESP_LOGE("TCP SOCKET", "Unable to create socket: errno %s", strerror(my_sock));
            return;   
        }
        else{
            ESP_LOGI("TCP SOCKET", "Successfully created socket with channel: %d", my_sock);
        }
        ESP_LOGI("TCP SOCKET", "Socket created, receiving from %s:%d", IP_server, PORT);

        while (true) {
            while (error != 0){
                error = connect(my_sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                if (error != 0) {
                    ESP_LOGE("TCP SOCKET", "Socket unable to connect: errno %s", strerror(errno));   
                }
                else{
                    ESP_LOGI("TCP SOCKET", "Successfully connected");
                }
                my_delay(1000);
            }
            // receiving data 
            ESP_LOGI("TCP SOCKET", "Waiting for data to recv");
            int32_t rx_buffer[4];
            int len = recv(my_sock, rx_buffer, sizeof(rx_buffer), 0);
            if (len < 0) {
                ESP_LOGE("TCP SOCKET", "Error occurred while sending data: errno %s", strerror(errno));
                my_delay(1000);
                break;
            } else {
                xQueueOverwrite(queue, rx_buffer);
                ESP_LOGI("TCP SOCKET", "Successfully received data right: %d, left: %d, speed_right: %d, speed_left: %d",(int)rx_buffer[0], (int)rx_buffer[1], (int)rx_buffer[2], (int)rx_buffer[3]);
            }
        }
    }

    vTaskDelete(NULL);
}


void app_main(void){
    queue = xQueueCreate(1, sizeof(message));
    if(queue == 0){
    	ESP_LOGE("QUEUE", "QUEUE CREATION FAILED");
    	return ;
    }
    wifi_connection(WIFI_SSID, WIFI_PASSWORD);
    xTaskCreate(tcp_conn_task, "TCP_client", 4096, NULL, 5, NULL);
    xTaskCreate(car_control_task, "car_ctrl", 4096, NULL, 2, NULL);
}

