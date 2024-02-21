#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys/socket.h"
#include "netdb.h"
#include "errno.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"

#include "../core/core.h"
#include "../cihazlar/cihazlar.h"
#include "../iptool/iptool.h"

#define MAX_SOCKET 30
#define INVALID_SOCK (-1)
#define YIELD_TO_ALL_MS 50
#define BUFFER_SIZE 512

class TcpServer {
    public:
      bool Start(   home_network_config_t *cnf, 
                      home_global_config_t *hcnf, 
                      uint16_t tcpport,
                      transmisyon_callback_t cb,
                      Cihazlar *chz
                    );
       
      esp_err_t Send(char *data, uint8_t id);
      esp_err_t PortSend(char *data, int port);
      bool is_busy(void) {return false;}


      TcpServer() {
        port = (char *)malloc(7);
        adres = (char *)malloc(17);
        server_fatal_error = false;
      };
      ~TcpServer(){
        free(port);
        free(adres);
      };

      
      
      //özel tanımlar
      //---------------------------

      SemaphoreHandle_t server_ready= NULL;
      IPAddr addr = IPAddr();  
      bool server_fatal_error;
      char *port;
      char *adres;  
    
      const char *TAGTCP="TCP_PORT";
    
        Cihazlar *cihaz = NULL;
      
      home_network_config_t *net_config;
      home_global_config_t *dev_config;
      transmisyon_callback_t callback=NULL;
      uint16_t TcpPort = -1;

     // tcpip_adapter_ip_info_t ip_info;

    private:
      esp_err_t _Send(char *data, int port);
      static void tcp_server_task(void *arg); 
      static void log_socket_error(const char *tag, const int sock, const int err, const char *message);
      static int try_receive(const char *tag, const int sock, uint8_t * data, size_t max_len);
      static int socket_send(const char *tag, const int sock, const char * data, const size_t len);
      static void get_clients_address(struct sockaddr_storage *source_addr, char *address_str);
};   

#endif