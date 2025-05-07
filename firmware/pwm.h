
#ifndef PWM_H
#define PWM_H

#include "esp_event.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"


void set_up(int rigel, int pin);
void analog_write(int channel, int duty_cycle);

#endif
