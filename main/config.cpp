
void global_default_config(void)
{
     GlobalConfig.home_default = 1; 
     strcpy((char*)GlobalConfig.device_name, "Main_CPU2");
     strcpy((char*)GlobalConfig.mqtt_server,"");
     strcpy((char*)GlobalConfig.license, "");
     GlobalConfig.mqtt_keepalive = 0; 
     GlobalConfig.start_value = 0;
     GlobalConfig.device_id = 254;
     GlobalConfig.http_start = 1;
     GlobalConfig.tcp_start = 1;
     GlobalConfig.reset_servisi = 0;
     disk.file_control(GLOBAL_FILE);
     disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
}

void network_default_config(void)
{
     NetworkConfig.home_default = 1; 
     NetworkConfig.wifi_type = HOME_WIFI_AP;
     NetworkConfig.ipstat = DYNAMIC_IP;
     strcpy((char*)NetworkConfig.wifi_ssid, "ICE_Device");
     strcpy((char*)NetworkConfig.wifi_pass, "IcedeviCE");
     strcpy((char*)NetworkConfig.ip,"192.168.7.1");
     strcpy((char*)NetworkConfig.netmask,"255.255.255.0");
     strcpy((char*)NetworkConfig.gateway,"192.168.7.1");
     NetworkConfig.espnow=0;
     strcpy((char*)NetworkConfig.update_server,"icemqtt.com.tr");
     NetworkConfig.upgrade = 0;

     //NetworkConfig.wifi_type = HOME_WIFI_STA;
     //strcpy((char *)NetworkConfig.wifi_ssid,(char *)"IMS_YAZILIM");
     //strcpy((char *)NetworkConfig.wifi_pass,(char *)"mer6514a4c");

     disk.file_control(NETWORK_FILE);
     disk.write_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0);
}

void config(void)
{
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<LED) | (1ULL<<CPU1_RESET);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE; 

    gpio_config(&io_conf);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<BUTTON1);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; 
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<WATER);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE; 
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<I2CINT);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; 
    gpio_config(&io_conf);

    //------------------- Pcf init ------------------

      vTaskDelay(1000/portTICK_PERIOD_MS);
      
      gpio_set_level((gpio_num_t)LED, 1);
      esp_err_t err0 = ESP_OK, err1 = ESP_OK, err2 = ESP_OK;
      err0 = pcf_init(&pcf0, 0x20, 5, true, (gpio_num_t)LED) ;
      err1 = pcf_init(&pcf1, 0x21, 5, true,(gpio_num_t)LED) ;
      err2 = pcf_init(&pcf2, 0x22, 5, true,(gpio_num_t)LED) ;
      gpio_set_level((gpio_num_t)LED, 0);
      ESP_ERROR_CHECK (err0);
      ESP_ERROR_CHECK (err1);
      ESP_ERROR_CHECK (err2);
      
      //#undef PCF_4

      #ifdef PCF_4
          ESP_ERROR_CHECK (pcf_init(&pcf4, 0x23, 10, true,(gpio_num_t)LED)) ;
          ESP_LOGW(TAG, "EXTENDED PCF AKTIF");
      #else
          //&pcf4 = NULL; 
          ESP_LOGW(TAG, "EXTENDED PCF PASIF");    
      #endif
      
      #ifdef ATMEGA_CONTROL
        ATMEGA=true;
        if (gateway_init_desc(&pcf3, 0x04, 0, (gpio_num_t)21, (gpio_num_t)22)!=ESP_OK) ATMEGA=false;
        ESP_LOGW(TAG, "ATMEGA AKTIF");
      #else
        //pcf3 = NULL; 
        ESP_LOGW(TAG, "ATMEGA PASIF"); 
      #endif  
     
      pcf[0] = &pcf0;
      pcf[1] = &pcf1;
      pcf[2] = &pcf2;
      pcf[3] = &pcf3;
      pcf[4] = &pcf4;

    #ifdef ATMEGA_CONTROL
      gpio_set_intr_type(I2CINT, GPIO_INTR_NEGEDGE);
      gpio_isr_handler_add(I2CINT, I2c_IntHandler, NULL);
      gpio_intr_disable(I2CINT);
    #endif  
        
    gpio_set_level((gpio_num_t)LED, 0);
    gpio_set_level((gpio_num_t)CPU1_RESET, 0);

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK( nvs_flash_erase() );
            ret = nvs_flash_init();
        }
     ESP_ERROR_CHECK( ret );

     //if(esp_event_loop_create_default()!=ESP_OK) {ESP_LOGE(TAG,"esp_event_loop_create_default ERROR "); }
	ESP_ERROR_CHECK(esp_event_handler_instance_register(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, all_event, NULL, NULL));

   get_sha256_of_partitions();

    //------------------- Pcf init ------------------
      
      gpio_set_level((gpio_num_t)LED, 1);
      
     
    //---------STORAGE ------------
    ESP_ERROR_CHECK (!disk.init());
    //disk.format();

    disk.read_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig), 0);
    if (NetworkConfig.home_default==0 ) {
        //Network ayarları diskte kayıtlı değil. Kaydet.
         network_default_config();
         disk.file_control(NETWORK_FILE);
         disk.read_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0);
         if (NetworkConfig.home_default==0 ) ESP_LOGW(TAG, "Network Initilalize File ERROR !...");
    }
    
    disk.read_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig), 0);
    if (GlobalConfig.home_default==0 ) {
        //Global ayarlar diskte kayıtlı değil. Kaydet.
         global_default_config();
         disk.read_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
         if (GlobalConfig.home_default==0 ) printf( "\n\nGlobal Initilalize File ERROR !...\n\n");
    }
    disk.list("/config", "*.*");

    if (GlobalConfig.start_value==1) {GlobalConfig.start_value=0; 
                                      disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0); 
                                      test_tip(pcf, &disk); 
                                      } 
   
    //-------------------------------------
    uart_cfg.uart_num = 2;
    uart_cfg.dev_num  = CPU2_ID;
    uart_cfg.rx_pin   = 17;
    uart_cfg.tx_pin   = 16;
    uart_cfg.atx_pin   = 5;
    uart_cfg.arx_pin   = 19;
    uart_cfg.int_pin   = 18;
    uart_cfg.baud      = 921600;
    //int 19

    rs485_cfg.uart_num = 1;
    rs485_cfg.dev_num  = CPU2_ID;
    rs485_cfg.rx_pin   = 25;
    rs485_cfg.tx_pin   = 26;
    rs485_cfg.oe_pin   = 13;
    rs485_cfg.baud     = 460800;

    uart.initialize(&uart_cfg, &uart_callback);
    uart.ping_start(CPU1_ID,&CPU1_Ping_Reset, LED);
    

    rs485.initialize(&rs485_cfg, &rs485_callback, &broadcast_callback, (gpio_num_t)LED);

    //----------  NETWORK ------------ 
    uint8_t mc[6] = {0};
    if (NetworkConfig.espnow==0) 
        ESP_ERROR_CHECK(esp_read_mac(mc, ESP_MAC_WIFI_SOFTAP));
    else    
        ESP_ERROR_CHECK(esp_read_mac(mc, ESP_MAC_WIFI_STA));
    
    sprintf((char*)NetworkConfig.mac, "%02X%02X%02X%02X%02X%02X", mc[0], mc[1], mc[2],mc[3], mc[4], mc[5]);

    if (NetworkConfig.espnow==1)
    {       
        ESP_ERROR_CHECK(esp_netif_init());
        EspNOW_set_cihazlar(&cihazlar);        
        EspNOW_set_callback(&rs485_callback);
        EspNOW_set_broadcast_callback(&broadcast_callback);
        EspNOW_init(GlobalConfig.device_id);       
    }
    if (NetworkConfig.espnow==0 && NetworkConfig.wifi_type<4)
    {
          wifi.init(&NetworkConfig);
          wifi.cihaz = &cihazlar;
          if (NetworkConfig.wifi_type==HOME_WIFI_AP)
               {
                    esp_err_t rt = wifi.Ap_Start();
                    if (rt==ESP_OK) netstatus = true;
               }
          if (NetworkConfig.wifi_type==HOME_WIFI_STA)
               {
                    esp_err_t rt = wifi.Station_Start();
                    if (rt==ESP_OK) netstatus = true;
               }
    }
    //-------------------------------- 

    //fonksiyonları oku
    Read_functions(&function_Callback, //fonksiyon eventleri için callback 
                   &Register_Callback, //register istekleri için callback
                   disk,
                   pcf);

    //function_list();               

     if (GlobalConfig.start_value!=3)
    {
        uint8_t jj = 0;
        for (int i=0;i<MAX_DEVICE;i++)
        {
          function_reg_t reg = {};
          disk.read_file("/config/function.bin",&reg,sizeof(function_reg_t),i);
          if (reg.device_id>0) {
                  ++jj;
                  function_remote_re_register(&reg,&function_Callback, &command_Callback, disk);
                    }
        }  
        ESP_LOGI(TAG,"%d remote function eklendi",jj);                        
        function_list();
    }
   
  
    read_gateway(disk, rs485_callback, &rs485);
     
   
   read_locations(disk);
   list_locations();

     if (NetworkConfig.espnow==0 && NetworkConfig.wifi_type<4 && netstatus==true )
     {
          ESP_LOGI(TAG, "TCP SERVER START");
          tcpserver.Start(&NetworkConfig,&GlobalConfig,TCP_PORT, &coap_callback,&cihazlar);
          ESP_LOGI(TAG, "BROADCAST SERVER START");
          broadcast.Start(&NetworkConfig,&GlobalConfig,UDP_PORT, &broadcast_callback);
          
          if (GlobalConfig.http_start==1)
          {
               ESP_LOGI(TAG, "WEB START");
               ESP_ERROR_CHECK(start_rest_server("/config",&NetworkConfig, &GlobalConfig, &webwrite, &defreset)); 
          }

          if (netconfig->upgrade==1)
          {
             ESP_LOGI(TAG,"Firmware Upgrade");
             xTaskCreate(&ota_task, "ota_task", 8192, &NetworkConfig, 5, NULL);
          }         
     }

    char *Current_Date_Time = (char *)malloc(50);
      rtc.getTimeDate(Current_Date_Time);
      const esp_app_desc_t *desc = esp_app_get_description();                            
    ESP_LOGI(TAG,"         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    ESP_LOGI(TAG,"         |         ICE MAIN BOX       |");
    ESP_LOGI(TAG,"         |             CPU_2          |");
    ESP_LOGI(TAG,"         |         Version 1.0        |");
    ESP_LOGI(TAG,"         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    ESP_LOGI(TAG,"         RS485 DEVICE ID : %d" ,CPU2_ID);
    ESP_LOGI(TAG,"               Date/Time : %s" ,Current_Date_Time);
    ESP_LOGI(TAG,"             App Version : %s" ,desc->version);
    ESP_LOGI(TAG,"                App Name : %s" ,desc->project_name);    
    ESP_LOGI(TAG,"         WIFI            : %s" ,(netstatus)?"OK":"Wifi YOK");
    ESP_LOGI(TAG,"         ESP_NOW         : %d" ,NetworkConfig.espnow);
    ESP_LOGI(TAG,"         IP              : %s" ,Addr.to_string(NetworkConfig.home_ip));
    ESP_LOGI(TAG,"         NETMASK         : %s" ,Addr.to_string(NetworkConfig.home_netmask));
    ESP_LOGI(TAG,"         GATEWAY         : %s" ,Addr.to_string(NetworkConfig.home_gateway));
    ESP_LOGI(TAG,"         BROADCAST       : %s" ,Addr.to_string(NetworkConfig.home_broadcast));
    ESP_LOGI(TAG,"         MAC             : %s" ,(char*)NetworkConfig.mac);
    ESP_LOGI(TAG,"         Free heap       : %lu" ,esp_get_free_heap_size());
    free(Current_Date_Time);
  
}