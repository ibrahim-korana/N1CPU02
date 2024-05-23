

void webwrite(void)
{
    disk.write_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0);
    disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
}

void defreset(void)
{
    network_default_config();
    global_default_config();
}

void get_uart_time(void)
{
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "com", "T_REQ");
  char *dat = cJSON_PrintUnformatted(root);
  while(uart.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
  if (uart.Sender(dat,253)!=RET_OK) printf("PAKET GÖNDERİLEMEDİ \n");
  vTaskDelay(50/portTICK_PERIOD_MS); 
  cJSON_free(dat);
  cJSON_Delete(root);
}

void CPU1_Reset(void)
{
     gpio_set_level((gpio_num_t)CPU1_RESET, 1);
     vTaskDelay(500/portTICK_PERIOD_MS);
     gpio_set_level((gpio_num_t)CPU1_RESET, 0);
     ESP_LOGE(TAG,"Bekleniyor...");
     vTaskDelay(5000/portTICK_PERIOD_MS);
     ESP_LOGE(TAG,"Reset Tamamlandı");
}

void CPU2_Reset(void)
{
  esp_restart();
}

void CPU1_Ping_Reset(uint8_t counter)
{
  ESP_LOGE(TAG,"CPU1 Cevap vermiyor %d",counter);
  if (counter>7 && GlobalConfig.reset_servisi==1) {
        ESP_LOGE(TAG,"CPU1 RESETLIYORUM");
        //uart.ping_counter = 0;
        CPU1_Reset();
    }
}


void full_send_broadcast(char *dat)
{
  while(rs485.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);

/*
  while (1)
  {
    gpio_set_level((gpio_num_t)LED, 1);
    
    vTaskDelay(100/portTICK_PERIOD_MS); 
    gpio_set_level((gpio_num_t)LED, 0);
    vTaskDelay(100/portTICK_PERIOD_MS);  
  }
*/
  rs485.Sender(dat,255);
  if (NetworkConfig.espnow==0) broadcast.Send(dat);
  if (NetworkConfig.espnow==1) EspNOW_Broadcast(dat);
}

void broadcast_I_REQ(void)
{
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "com", "I_REQ");
  char *dat = cJSON_PrintUnformatted(root);
  full_send_broadcast(dat);
  cJSON_free(dat);
  cJSON_Delete(root);
}

void Global_Send(char *data,uint8_t sender, transmisyon_t transmisyon)
{
  if (transmisyon==TR_UDP) tcpserver.Send(data,sender);
  if (transmisyon==TR_SERIAL) {
        while(rs485.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
        rs485.Sender(data,sender);
                              }
  if (transmisyon==TR_ESPNOW) EspNOW_Send(data,sender);
}

void response_task(void *arg)
{
  response_par_t *par = (response_par_t *)arg;
  char *mm=(char*)calloc(1,strlen(par->data)+1);
  strcpy(mm,par->data);
  Global_Send(mm,par->sender,par->trn);
  free(mm);
  vTaskDelete(NULL);
}

void info(const char*komut, char* data)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", komut);
    cJSON_AddNumberToObject(root, "id", GlobalConfig.device_id);  
    cJSON_AddStringToObject(root, "mac", (char*)NetworkConfig.mac);
    cJSON_AddStringToObject(root, "ip", Addr.to_string(NetworkConfig.home_ip));
    cJSON_AddNumberToObject(root, "fcount", function_count());  
    char *dat = cJSON_PrintUnformatted(root);
    //full_send_broadcast(dat);
    strcpy(data,dat);
    cJSON_free(dat);
    cJSON_Delete(root); 
}

#define DEV_NUM 10
//-----------------------------------------
static void send_intro(void *arg)
{
    //printf("Before %u\n" ,esp_get_free_heap_size());
    vTaskDelay(100/portTICK_PERIOD_MS);
    uint8_t fs = function_count();
    uint8_t ps = (fs/DEV_NUM)+(((fs%DEV_NUM)>0)?1:0);
    uint8_t cp = 1;
    Base_Function *target = get_function_head_handle();

  while(cp<=ps) 
  {  
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "intro");
    cJSON_AddNumberToObject(root, "tpak",ps);
    cJSON_AddNumberToObject(root, "cpak",cp);
    cJSON_AddStringToObject(root, "dev", (char*)GlobalConfig.device_name);
    cJSON *array;
    array = cJSON_AddArrayToObject(root, "func");
    cJSON *object;
    uint8_t rr = 0;
    while (target)
      {
        object = cJSON_CreateObject();
        cJSON_AddStringToObject(object, "name", (char*)target->genel.name);
        cJSON_AddNumberToObject(object, "id", target->genel.device_id);
        uint8_t ll = 0;
        if (target->genel.register_device_id>0) ll=100;
        ll += target->prg_loc;
        cJSON_AddNumberToObject(object, "loc", ll);
        cJSON_AddItemToArray(array, object);
        target=target->next;
        if (rr++>DEV_NUM) break;
      }
    char *dat = cJSON_PrintUnformatted(root); 
    printf("INTRO SIZE %d:%d %d %s\n",ps,cp,strlen(dat),dat);
    cp++;
    //strcpy(data,dat);
    while(uart.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
    uart.Sender(dat,253);
    cJSON_free(dat);
    cJSON_Delete(root); 
    vTaskDelay(300/portTICK_PERIOD_MS);
  } 
    //printf("After %u\n" ,esp_get_free_heap_size()); 
    vTaskDelete(NULL);
}

//-----------------------------------------
void send_sintro(char *data)
{
    cJSON *root;
	  root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "sintro");
    cJSON *array;
    array = cJSON_AddArrayToObject(root, "dev");
    cJSON *object;
    Base_Function *target = get_function_head_handle();
    while (target)
      {
        object = cJSON_CreateObject();
        bool ekle = target->get_port_json(object);
        if (ekle)
              {
                 cJSON *obb = cJSON_CreateObject();
                 cJSON_AddNumberToObject(obb, "id", target->genel.device_id);
                 cJSON_AddItemToObject(obb,"sdev",object);
                 cJSON_AddItemToArray(array, obb); 
              } else cJSON_Delete(object);  
        target=target->next;
      }
    char *dat = cJSON_PrintUnformatted(root); 
    //printf("%s\n",dat);
    strcpy(data,dat);
    cJSON_free(dat);
    cJSON_Delete(root);  
}
//-----------------------------------------
void send_status(uint8_t id, uint8_t sender)
{

    Base_Function *target = get_function_head_handle();
    while (target)
    {     
      if (target->genel.device_id==id || id==0)
        {
          printf("Status id %d %d\n",target->genel.device_id,id);
          cJSON *root = cJSON_CreateObject();
          cJSON_AddStringToObject(root, "com", "status");
          cJSON_AddNumberToObject(root, "id", target->genel.device_id);
          cJSON *drm = cJSON_CreateObject();
          target->get_status_json(drm);
          cJSON_AddItemToObject(root, "durum", drm);
          char *dat = cJSON_PrintUnformatted(root); 

            while(uart.is_busy()) vTaskDelay(30/portTICK_PERIOD_MS);
            uart.Sender(dat,sender);
            printf("%d %s\n",sender,dat);
            
          cJSON_free(dat);
          cJSON_Delete(root);  
        }    
      target=target->next;
    }
}
//-----------------------------------------
bool event_action(cJSON *rcv)
{
  uint8_t id=0;
  JSON_getint(rcv,"id", &id);
  if (id==200) {
    char *nm = (char *)malloc(15);
    JSON_getstring(cJSON_GetObjectItem(rcv,"durum"),"ircom",nm,14);
    Base_Function *target=get_function_head_handle();
    while(target)
      {
        home_status_t stat={};
        json_to_status(rcv, &stat, target->get_status());
        target->set_sensor(nm,stat);
        target=target->next;
      }
    free(nm);
    id=0;
  }
  if (id<1) return false; 

  Base_Function *aa = function_find(id);
  if (aa!=NULL)
  {
      home_status_t stat={};
      json_to_status(rcv, &stat, aa->get_status());
      aa->set_status(stat);
  }
  return true;
}

bool sevent_action(cJSON *rcv, char *data)
{
    uint8_t id=0;
    JSON_getint(rcv,"id", &id);
    if (id<1) return false; 
    Base_Function *aa = function_find(id);
    if (aa!=NULL)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "sevent");
        cJSON_AddNumberToObject(root, "id", aa->genel.device_id);
        aa->get_port_status_json(root);
        char *dat = cJSON_PrintUnformatted(root); 
        strcpy(data,dat);
        cJSON_free(dat);
        cJSON_Delete(root);  
    }
  return true;
}

bool spevent_action(cJSON *rcv, char *data)
{
    uint8_t id=0;
    JSON_getint(rcv,"id", &id);
    if (id<1) return false; 
    Base_Function *aa = function_find(id);
    if (aa!=NULL)
    {
       uint8_t pn = 0;
       bool st = false;
       JSON_getint(rcv,"port",&pn);
       JSON_getbool(rcv,"usract",&st);
       Base_Port *qq = aa->port_head_handle;
       while(qq)
         {
           if (qq->id==pn) qq->set_user_active(st); 
           qq = qq->next;
         }         
    }
  return true;
}

void send_location(char *response)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "loc");
    cJSON *array;
    array = cJSON_AddArrayToObject(root, "locs");
    cJSON *object;
    prg_location_t *target = get_location_head_handle();
    while (target)
      {
        object = cJSON_CreateObject();
        cJSON_AddStringToObject(object, "name", (char*)target->name);
        cJSON_AddNumberToObject(object, "page", target->page);
        cJSON_AddItemToArray(array, object);
        target=(prg_location_t *)target->next;
      }
    char *dat = cJSON_PrintUnformatted(root); 
    printf("LOC %s\n",dat);
    strcpy(response,dat);
    cJSON_free(dat);
    cJSON_Delete(root);    
}

void refresh_device(void)
{
  /*
     cihazları SIL
     remote fonksiyonları sil
     ip kaydı için sinfo broadcast mesajı yolla
     cihazlar kaydedildikten sonra (bekle)
     kaydedilen her cihaz için re_register çalıştırarak fonksiyon kaydı yap
  */
 if(GlobalConfig.start_value!=3)
    {
      cihazlar.cihaz_bosalt();function_reg_t reg = {};
            reg.device_id = 0;
            for (int i=0;i<MAX_DEVICE;i++)
              {
                  disk.write_file("/config/function.bin",&reg,sizeof(function_reg_t),i);
              } 
      //Diskteki fonksiyonlar siliniyor
      function_reg_t reg0 = {};
      reg.device_id = 0;
      for (int i=0;i<MAX_DEVICE;i++)
        {
            disk.write_file("/config/function.bin",&reg0,sizeof(function_reg_t),i);
        } 


      //Hafızadaki fonksiyonları silmek oldukça zor bu nedenle özel bir reboot yapacagız
          ESP_LOGE(TAG,"SPESIAL REBOOT....");
          GlobalConfig.start_value = 3;
          disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);   
          esp_restart();  
    } else {
      GlobalConfig.start_value = 0;
      disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0); 
    }
  //Burada cihazlar ve fonksiyonlar başlangıç konumunda
  //açılışta sinfo gitti cihazlar kayıt oldu. Her ihtimale karşı idleri kontrol et;

  ESP_LOGE(TAG,"DEVICE CONTROL....");
  ESP_ERROR_CHECK(device_control());  
  ESP_LOGE(TAG,"END DEVICE CONTROL PROCESS");
}

esp_err_t device_control(void)
{
  uint8_t say = 1,rep = 0;
  ESP_LOGW(TAG,"Device Control");
  while(say>0)
  {
      device_register_t *target = cihazlar.get_handle();
      IACK_ready = xSemaphoreCreateBinary();
      assert(IACK_ready);
      
      while(target)
      {
        if (target->device_id<1 && target->socket>0)
        {
          cJSON* root = cJSON_CreateObject();
          cJSON_AddStringToObject(root, "com", "I_REQ");
          cJSON_AddNumberToObject(root, "id",254);  
          char *dat = cJSON_PrintUnformatted(root);
          tcpserver.PortSend(dat,target->socket);
          cJSON_free(dat);
          cJSON_Delete(root); 
          xSemaphoreTake(IACK_ready, 5000/portTICK_PERIOD_MS); 
        }
        target = (device_register_t *)target->next;
      }
      vSemaphoreDelete(IACK_ready);
      IACK_ready =NULL;
      vTaskDelay(500/portTICK_PERIOD_MS);  
      say=0;
      target = cihazlar.get_handle();
      while(target)
      {
        if (target->device_id<1 && target->socket>0) say++;
        target = (device_register_t *)target->next;
      }
      if (rep++>5) return ESP_FAIL;
  }
  return ESP_OK;
}

void out_print(void)
{
   iot_button_list();
   iot_out_list();
}

void register_all_task(void *arg)
{
  //disk siliniyor
      ESP_LOGW(TAG,"Fonksiyonlar diskten diliniyor..");
      disk.function_file_format();

      //baglı cihazlara register emri gidiyor
      cihazlar.cihaz_list();

      device_register_t *target = cihazlar.get_handle();
      cJSON *root = cJSON_CreateObject();
      cJSON_AddStringToObject(root, "com", "R_REG");
      char *dat = cJSON_PrintUnformatted(root);

      while (target)
        {
          
          printf("DEVICE %d %d\n",target->device_id,target->transmisyon);   
          
          if (target->device_id>0 && target->device_id<200)
          {   
            Global_Send(dat,target->device_id,target->transmisyon);
            ESP_LOGW(TAG,"%d Registration WAITING..",target->device_id);
            if (xSemaphoreTake(REGOK_ready,10000/portTICK_PERIOD_MS)==pdTRUE)
            {
                 ESP_LOGW(TAG,"%d Registration completed",target->device_id);
            } else {
                 ESP_LOGE(TAG,"%d Registration ERROR",target->device_id);
            }

          }
          target = (device_register_t *)target->next;
        }
      cJSON_free(dat);
      cJSON_Delete(root);   

  vTaskDelete(NULL);
}

void register_all(void)
{
      xTaskCreate(register_all_task, "regall", 4096, NULL, 5, NULL); 
      
}

void Remore_Reset(void)
{
  //tüm remore cihazlar resetlensin
  Reset_Broadcast();
}

void Reset_Broadcast(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "reset");
    char *dat = cJSON_PrintUnformatted(root);
    full_send_broadcast(dat);
    cJSON_free(dat);
    cJSON_Delete(root); 
}