
#include "esp_log.h"
#include "esp_http_server.h"


const char* get_id_string(esp_event_base_t base, int32_t id) 
{
    const char* event = "TASK_ITERATION_EVENT";

    if (base==ESP_HTTP_SERVER_EVENT)
    {
      if (id==HTTP_SERVER_EVENT_ERROR ) event = "HTTP_SERVER_EVENT_ERROR";
      if (id==HTTP_SERVER_EVENT_START ) event = "HTTP_SERVER_EVENT_START";
      if (id==HTTP_SERVER_EVENT_ON_CONNECTED  ) event = "HTTP_SERVER_EVENT_ON_CONNECTED";
      if (id==HTTP_SERVER_EVENT_ON_HEADER ) event = "HTTP_SERVER_EVENT_ON_HEADER";
      if (id==HTTP_SERVER_EVENT_HEADERS_SENT ) event = "HTTP_SERVER_EVENT_HEADERS_SENT";
      if (id==HTTP_SERVER_EVENT_ON_DATA ) event = "HTTP_SERVER_EVENT_ON_DATA";
      if (id==HTTP_SERVER_EVENT_SENT_DATA ) event = "HTTP_SERVER_EVENT_SENT_DATA";
      if (id==HTTP_SERVER_EVENT_DISCONNECTED ) event = "HTTP_SERVER_EVENT_DISCONNECTED";
      if (id==HTTP_SERVER_EVENT_STOP ) event = "HTTP_SERVER_EVENT_STOP";
    }

    if (base==IP_EVENT)
      {
        if (id==IP_EVENT_STA_GOT_IP ) event = "IP_EVENT_STA_GOT_IP";
        if (id==IP_EVENT_STA_LOST_IP  ) event = "IP_EVENT_STA_LOST_IP ";
        if (id==IP_EVENT_AP_STAIPASSIGNED  ) event = "IP_EVENT_AP_STAIPASSIGNED ";
        if (id==IP_EVENT_GOT_IP6) event = "IP_EVENT_GOT_IP6";
        if (id==IP_EVENT_ETH_GOT_IP ) event = "IP_EVENT_ETH_GOT_IP";
        if (id==IP_EVENT_ETH_LOST_IP ) event = "IP_EVENT_ETH_LOST_IP";

      }
      
    if (base==WIFI_EVENT)
      {
        if (id==WIFI_EVENT_WIFI_READY) event = "WIFI_EVENT_WIFI_READY";
        if (id==WIFI_EVENT_SCAN_DONE) event = "WIFI_EVENT_SCAN_DONE";
        if (id==WIFI_EVENT_STA_START) event = "WIFI_EVENT_STA_START";
        if (id==WIFI_EVENT_STA_STOP ) event = "WIFI_EVENT_STA_STOP ";
        if (id==WIFI_EVENT_STA_CONNECTED ) event = "WIFI_EVENT_STA_CONNECTED ";
        if (id==WIFI_EVENT_STA_DISCONNECTED ) event = "WIFI_EVENT_STA_DISCONNECTED ";
        if (id==WIFI_EVENT_STA_AUTHMODE_CHANGE ) event = "WIFI_EVENT_STA_AUTHMODE_CHANGE ";
        if (id==WIFI_EVENT_AP_START ) event = "WIFI_EVENT_AP_START ";
        if (id==WIFI_EVENT_AP_STOP ) event = "WIFI_EVENT_AP_STOP ";
        if (id==WIFI_EVENT_AP_STACONNECTED ) event = "WIFI_EVENT_AP_STACONNECTED ";
        if (id==WIFI_EVENT_AP_STADISCONNECTED  ) event = "WIFI_EVENT_AP_STADISCONNECTED  ";
        if (id==WIFI_EVENT_AP_PROBEREQRECVED  ) event = "WIFI_EVENT_AP_PROBEREQRECVED  ";
      }
      
    

    return event;
}
void all_event(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
   // ESP_LOGW(TAG, "%lu %s:%s", id , base, get_id_string(base, id));
    //if (GlobalConfig.comminication==TR_UDP)
    {
        if (base==IP_EVENT)
          {
            if (id==IP_EVENT_STA_GOT_IP || id==IP_EVENT_ETH_GOT_IP)
              {
                // Net.wifi_update_clients();
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                NetworkConfig.home_ip = event->ip_info.ip.addr;
                NetworkConfig.home_netmask = event->ip_info.netmask.addr;
                NetworkConfig.home_gateway = event->ip_info.gw.addr;
                NetworkConfig.home_broadcast = (uint32_t)(NetworkConfig.home_ip) | (uint32_t)0xFF000000UL;

                strcpy((char *)NetworkConfig.ip,Addr.to_string(NetworkConfig.home_ip));
                strcpy((char *)NetworkConfig.netmask,Addr.to_string(NetworkConfig.home_netmask));
                strcpy((char *)NetworkConfig.gateway,Addr.to_string(NetworkConfig.home_gateway));
                //tcpclient.wait = false;
                if (id==IP_EVENT_ETH_GOT_IP) {
                  ESP_LOGI(TAG, "Ethernet GOT IP %s", NetworkConfig.ip);
                  //Eth.set_connect_bit();
                }
                if (id==IP_EVENT_STA_GOT_IP) wifi.set_connection_bit();
              }
            if (id==IP_EVENT_AP_STAIPASSIGNED)
             {
                //Station ip aldı
                printf("CLIENTA IP ATANDI\n");
             }  
          }
          
        
        if (base==WIFI_EVENT)
          {
            if (id == WIFI_EVENT_AP_STACONNECTED) {
                wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
                char *mm = (char *)malloc(20);
                sprintf(mm,"%02X%02X%02X%02X%02X%02X",MAC2STR(event->mac));
                cihazlar.cihaz_ekle(mm,TR_UDP);
                free(mm);
            }   

            if (id == WIFI_EVENT_AP_STADISCONNECTED) {
                wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
                char *mm = (char *)malloc(20);
                sprintf(mm,"%02X%02X%02X%02X%02X%02X",MAC2STR(event->mac));
                cihazlar.cihaz_sil(mm);
                free(mm);
            } 

            if (id ==IP_EVENT_AP_STAIPASSIGNED) {
                printf("IP ATANDI\n");
            }

            if (id==WIFI_EVENT_STA_DISCONNECTED)
              {
                 // tcpclient.wait = true;
                  if (wifi.retry < WIFI_MAXIMUM_RETRY) {
                	  wifi.Station_connect();
                      wifi.retry++;
                      ESP_LOGW(TAG, "Tekrar Baglanıyor %d",WIFI_MAXIMUM_RETRY-wifi.retry);
                                                      } else {
                      ESP_LOGE(TAG,"Wifi Başlatılamadı..");
                      ESP_ERROR_CHECK(ESP_FAIL);
                                                             }
              }
            
            if (id==WIFI_EVENT_STA_START)
              {
                wifi.retry=0;
                ESP_LOGW(TAG, "Wifi Network Connecting..");
                wifi.Station_connect();
              }
            
          }      
    }
}



