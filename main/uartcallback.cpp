
typedef struct {
  uint8_t id;
  uint8_t sender;
} send0_param_t;

void send_param_task(void *arg)
{
  send0_param_t *prm = (send0_param_t *)arg;
 // printf("status send task\n");
  send_status(prm->id,prm->sender);
  vTaskDelete(NULL);
}


/*
    CPU1 den gelen tüm mesajlar bu fonksiyona gelecek
*/
void uart_callback(char *data)
{
    uint8_t sender = 253;
    ESP_LOGI(TAG,"CPU1 GELEN << sender: [%d] %s",sender, data);
    vTaskDelay(50/portTICK_PERIOD_MS);

    char *response = (char *)malloc(1024);
    memset(response,0,1024);

    cJSON *rcv = cJSON_Parse(data);
    if (rcv==NULL) return; 
    char *command = (char *)calloc(1,20); 
    JSON_getstring(rcv,"com", command,19); 

    if (strcmp(command,"reset")==0) CPU1_Reset();

    if (strcmp(command,"T_ACK")==0) {
       uint32_t tm0 = 0;
       uint8_t res_sys = 0;
       JSON_getlong(rcv,"time",&tm0);
       JSON_getint(rcv,"res_sys",&res_sys); 
       GlobalConfig.reset_servisi = res_sys;
       //printf("TM=%u\n",tm0); 
       rtc.setTime((unsigned long)tm0,0);
       
       //
       char *tt = (char *)malloc(32);
       rtc.getTimeDateTR(tt);
       ESP_LOGW("UART_CB","DATE/TIME %s",tt);
       free(tt);    
    }
  
    if (strcmp(command,"intro")==0) xTaskCreate(send_intro, "intro", 4096, NULL, 5, NULL);
    if (strcmp(command,"COM_names")==0) send_names();
    if (strcmp(command,"sintro")==0) send_sintro((char *)response);
    if (strcmp(command,"status")==0) {
                uint8_t id=0;
                JSON_getint(rcv,"id",&id);
                send0_param_t pp = {};
                pp.id = id;
                pp.sender = sender;
                xTaskCreate(send_param_task, "spt", 4096, &pp, 5, NULL); 
                                    }
                              
    if (strcmp(command,"event")==0) event_action(rcv);
    if (strcmp(command,"sevent")==0) sevent_action(rcv, (char *)response);
    if (strcmp(command,"spevent")==0) spevent_action(rcv, (char *)response);
    if (strcmp(command,"loc")==0) send_location((char *)response);


    if (strcmp(command,"COM_idcall")==0) refresh_device();//button1_up_cb(NULL,NULL);//full_status_read();//
    if (strcmp(command,"COM_rereg")==0) register_all(); 
    if (strcmp(command,"COM_funlist")==0) function_list();
    if (strcmp(command,"COM_devlist")==0) cihazlar.cihaz_list();
    if (strcmp(command,"COM_timeupd")==0) get_uart_time();
    if (strcmp(command,"COM_outlist")==0) out_print();
    
    if (strcmp(command,"COM_wifilist")==0) {
             //Net.wifi_update_clients(true);
             broadcast_I_REQ();
             } //full_status_read(); //Net.wifi_update_clients();
    if (strcmp(command,"COM_devcontrol")==0) device_control();
    if (strcmp(command,"COM_CPU1reset")==0) CPU1_Reset();
    if (strcmp(command,"COM_CPU2reset")==0) CPU2_Reset();
    
    if (strcmp(command,"COM_Remotereset")==0) Remore_Reset();
    if (strcmp(command,"COM_Test")==0) {
              GlobalConfig.start_value = 1;
              disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
              CPU2_Reset();
              }

    if (strcmp(command,"COM_485Test")==0) {
              GlobalConfig.start_value = 2;
              disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
              CPU2_Reset();
              }
              
    if (strcmp(command,"T_SEN")==0)
      {
          char *nm = (char *)calloc(1,20); 
          JSON_getstring(rcv,"uname", nm,19); 
          Base_Function *hand = get_function_head_handle();
          while(hand)
           {
             if (strcmp(hand->genel.uname,nm)==0 || strcmp(nm,"RESET")==0) {
                char *par = (char *)calloc(1,50); 
                JSON_getstring(rcv,"ack", par,49); 
                hand->senaryo(par);
                free(par);
             }
             hand = hand->next;
           }
          free(nm);
      }

    if (strcmp(command,"all")==0)
      {
          char *par0 = (char *)calloc(1,20); 
          uint8_t par1=0;
          bool ok=false;
          if (JSON_getstring(rcv,"dev", par0,19))
            if (JSON_getint(rcv,"ack", &par1)) ok=true;
          if (ok) 
          { 
              Base_Function *hand = get_function_head_handle();
              while(hand)
              {
                if (strcmp(hand->genel.name,par0)==0)
                {
                  if (strcmp(par0,"lamp")==0 ) {
                    home_status_t sst = hand->get_status();
                    if (par1==0) sst.stat = false;
                    if (par1==1) sst.stat = true;
                    hand->set_status(sst);
                  }
                  if (strcmp(par0,"cur")==0) {
                    home_status_t sst = hand->get_status();
                    if (par1==0) sst.status = 0;
                    if (par1==1) sst.status = 2;
                    hand->set_status(sst);
                  }
                }
                hand = hand->next;
              }
          }
          free(par0);
      }

    if (strcmp(command,"E_REQ")==0)
    {
      uint8_t id0 = 0;
      JSON_getint(rcv,"id",&id0);
      if (JSON_item_control(rcv,"durum")) 
      {
        home_status_t stt = {};
         ESP_LOGI(TAG,"E_REQ geldi id=%d",id0);
        if (id0>0)
        {
          Base_Function *bf = function_find(id0);        
          if (bf!=NULL) {
            //printf("%d stat\n",id0);
            json_to_status(rcv,&stt,bf->get_status()); 
            bf->remote_set_status(stt);
          }
        } else {
           //eger id 0 ise statusu tum sensorlere gönder.
           Base_Function *bf = get_function_head_handle();
           while (bf)
              {
                json_to_status(rcv,&stt,bf->get_status()); 
                //ESP_LOGI(TAG,"send %s %d ",bf->genel.name, bf->genel.device_id);                
                bf->set_sensor((char*)stt.irval,stt);
                bf = bf->next;
              }
        }
      }
    }  

    free(command);  
    cJSON_Delete(rcv);  

    if (strlen(response)>2)
    {
      while(uart.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
      uart.Sender(response,sender,true);    
      vTaskDelay(50/portTICK_PERIOD_MS); 
      //uart.send_fin(sender);
      vTaskDelay(50/portTICK_PERIOD_MS); 
    }
    
    
   free(response);
}

//
void udp_rec_task(void *arg)
{

    char *tx = (char *)arg;
    char *data;
    if(asprintf(&data,"%s",tx)>-1)
    {
      coap_callback(data, 1, TR_UDP, NULL, false);
      free(data);
    }  
//    printf("UDP Task EXIT\n");  
  vTaskDelete(NULL);
}


static void on_udp_recv(void *handler_arg, esp_event_base_t base, int32_t id,
                    void *event_data)
{
     if (event_data == NULL) {
        return;
    }
    udp_msg_t *recv = (udp_msg_t *)event_data;
    char *txt = (char*)calloc(1,recv->len+1);
    memcpy(txt,recv->payload,recv->len);
    
    ESP_LOGI(TAG, "Received %d bytes from %s:%d", recv->len,
             udp_server.remote_ipstr(recv->remote), udp_server.remote_port(recv->remote));
   // ESP_LOG_BUFFER_HEXDUMP(TAG, recv->payload, recv->len, ESP_LOG_INFO);
    ESP_LOGI(TAG," << %s",txt);
   
    //uart_callback(txt);

    xTaskCreate(&udp_rec_task, "aaa_task", 4096, txt, tskIDLE_PRIORITY, NULL);
    vTaskDelay(50/portTICK_PERIOD_MS);

    free(txt);
 
}