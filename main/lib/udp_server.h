


#ifndef _UDP_SERVER02_H
#define _UDP_SERVER02_H


#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_err.h"

#include "lwip/def.h"
#include "lwip/err.h"
#include "lwip/udp.h"

ESP_EVENT_DECLARE_BASE(UDP_EVENT);

enum {
    UDP_EVENT_RECV,
    UDP_EVENT_SEND,
    UDP_EVENT_BROADCAST,
};

typedef struct {
    void *remote;
    void *payload;
    size_t len;
} udp_msg_t;

typedef struct {
    int port;
    void *payload;
    size_t len;
} udp_broadcast_t;

typedef struct {
    ip_addr_t addr;
    uint16_t port;
} remote_t;

class Udp_Server {
    public:
      Udp_Server() {};
      ~Udp_Server(){};
      esp_err_t start(uint16_t prt);
      esp_err_t stop(void);

      udp_msg_t *send_receive_broadcast(const uint8_t *data, uint8_t len, esp_err_t *err);
      udp_msg_t *send_receive_broadcast(const uint8_t *data, uint8_t len, esp_err_t *err, uint16_t timeout);
      udp_msg_t *send_receive_unicast(remote_t *remote, const uint8_t *data, uint8_t len, esp_err_t *err);     
      udp_msg_t *send_receive_unicast(remote_t *remote, const uint8_t *data, uint8_t len, esp_err_t *err, uint16_t timeout);

      esp_err_t send_unicast(remote_t *remote,const uint8_t *data, uint8_t len);
      esp_err_t send_broadcast(const uint8_t *data, uint8_t len);

      char *remote_ipstr(const void *remote);
      int remote_port(const void *remote);
      udp_msg_t *copy_msg(udp_msg_t *msg);
      remote_t *create_remote(const char *ip);
      
    private:
        int port;
        struct udp_pcb *pcb = NULL;
        QueueHandle_t Recv_Queue = NULL;
        volatile bool wait_ack = false;
        esp_err_t _deinit(void);
        static void _on_send_ack(void *handler_arg, esp_event_base_t base,int32_t id, void *event_data);
        static void _on_send(void *handler_arg, esp_event_base_t base,int32_t id, void *event_data);
        static void _on_broadcast(void *handler_arg, esp_event_base_t base,int32_t id, void *event_data);
        static void _recv_proc(void *arg, struct udp_pcb *pcb, struct pbuf *pb,const ip_addr_t *addr, uint16_t port);
};


#endif