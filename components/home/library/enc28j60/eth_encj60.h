#ifndef _ETHERNET_J60_H
#define _ETHERNET_J60_H

#include "esp_system.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "esp_eth_enc28j60.h"
#include "enc28j60.h"
#include "driver/spi_master.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "freertos/event_groups.h"

#include "../lib/global/global.h"

#define ETHERNET_CONNECTED_BIT BIT0
#define ETHERNET_FAIL_BIT      BIT1


class Ethj60 {
    public:
      Ethj60() {};
      ~Ethj60() {};

      bool start(void);
      bool stop(void);
      bool init(network_config_t cnf, EventGroupHandle_t NetEv, change_callback_t cb);

      EventGroupHandle_t NetEvent;
      change_callback_t callback;

    private:
       network_config_t mConfig;     
       esp_eth_handle_t eth_handle_spi = { NULL }; 
    protected:  
        
};





#endif