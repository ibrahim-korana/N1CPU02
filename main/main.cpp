#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_ota_ops.h"

#define ESP_INTR_FLAG_DEFAULT 0

static const char *TAG = "ANAKUTU_CPU2";
#define GLOBAL_FILE "/config/global.bin"
#define NETWORK_FILE "/config/network.bin"

#include "geneltanim.h" 

#include "core.h"
#include "storage.h"
#include "iptool.h"
#include "lib/tool/tool.h"
#include "ice_pcf8574.h"
#include "classes.h"
#include "rs485.h"
#include "uart.h"
#include "cihazlar.h"
#include "ESP32Time.h"
#include "jsontool.h"
#include "wifi_now.h"
#include "network.h"
#include "tcpserver.h"
#include "broadcast.h"
#include "http.c"
#include "air.h"
#include "esp_timer.h"

#include "esp_http_client.h"
#include "esp_https_ota.h"


#define LED 0
#define CPU1_RESET 23
#define I2CINT GPIO_NUM_33
#define BUTTON1 34
#define WATER 27

#define HASH_LEN 32

#define CPU2_ID 254
#define CPU1_ID 253

#define TCP_PORT 5005
#define UDP_PORT 7700

/*
        TANIMLAR
*/


//void config(void);

Storage disk = Storage();
IPAddr Addr = IPAddr();

bool ATMEGA = true;
i2c_dev_t pcf0 = {};
i2c_dev_t pcf1 = {};
i2c_dev_t pcf2 = {};
i2c_dev_t pcf3 = {};
i2c_dev_t pcf4 = {};
i2c_dev_t *pcf[5];


home_network_config_t NetworkConfig = {};
home_global_config_t GlobalConfig = {};


UART_config_t uart_cfg={};
RS485_config_t rs485_cfg={};
ESP32Time rtc(10800);
UART uart = UART();
RS485 rs485 = RS485();
Cihazlar cihazlar = Cihazlar();
Network wifi = Network();
bool netstatus = false;

TcpServer tcpserver = TcpServer();
UdpBroadcast broadcast = UdpBroadcast();
QueueHandle_t ter_que = NULL;


//----------- Pre-Define Function -----------
void config(void);
void uart_callback(char *data, uint8_t sender, transmisyon_t transmisyon, void *response);
void rs485_callback(char *data, uint8_t sender, transmisyon_t transmisyon);
void coap_callback(char *data, uint8_t sender, transmisyon_t transmisyon, void *response, bool responsevar);
void network_default_config(void);
void global_default_config(void);
static void function_Callback(void *arg, home_status_t stat);
static void Register_Callback(void *arg);
static void command_Callback(void *arg, home_status_t stat);
void register_all(void);
void get_uart_time(void);
void broadcast_I_REQ(void);
void Reset_Broadcast(void);
void info(const char*komut, char* data);
void Global_Send(char *data,uint8_t sender, transmisyon_t transmisyon);
#ifdef ATMEGA_CONTROL
  static void IRAM_ATTR I2c_IntHandler(void* arg);
  static void read_task(void* arg);
#endif  
uint8_t function_count(void);
Base_Function *function_find(uint8_t id);
Base_Function *get_function_head_handle(void);
prg_location_t *get_location_head_handle(void);
void function_list(void);
//globaltools.cpp içinde
static void send_intro(void *arg);
void send_sintro(char *data);
void send_status(uint8_t id, uint8_t sender);
bool event_action(cJSON *rcv);
bool sevent_action(cJSON *rcv, char *data);
bool spevent_action(cJSON *rcv, char *data);
void send_location(char *response);
void refresh_device(void);
esp_err_t device_control(void);
void out_print(void);
void register_all(void);
void Remore_Reset(void);
void Reset_Broadcast(void);
uint8_t functions_remote_register(const char *name, 
                                  const char *auname,
                                  uint8_t sender, 
                                  uint8_t rid, 
                                  uint8_t prg_lc,
                                  transmisyon_t yer,
                                  function_callback_t fun_cb,
                                  function_callback_t com_cb,
                                  Storage dsk
                                  ) ;
void get_sha256_of_partitions(void);
void ota_task(void *param);

//----------------------------------


SemaphoreHandle_t register_ready;
SemaphoreHandle_t status_ready;
SemaphoreHandle_t IACK_ready;
SemaphoreHandle_t REGOK_ready;


#include "globaltools.cpp"
#include "coapcallback.cpp"
#include "uartcallback.cpp"
#include "broadcastcallback.cpp"
#include "rs485callback.cpp"
#include "event.cpp"

#include "lib/functions/functions.h"
#include "functioncallback.cpp"
#include "config.cpp"
#include "ota.cpp"


#ifdef ATMEGA_CONTROL
void I2c_IntHandler(void* arg)
{
    uint16_t aa=0;
    BaseType_t Tok;
    Tok = pdFALSE;
    gpio_intr_disable(I2CINT);
    xQueueSendFromISR(ter_que, &aa, &Tok);	
    if( Tok ) portYIELD_FROM_ISR ();
}

void read_task(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(ter_que, &io_num, portMAX_DELAY)) {
            //ESP_LOGW(TAG,"Termostat Interrupt");
            Termostat *tmp = get_termostat_head_handle();
            while(tmp)
            {
              //tmp->read_gateway();
              tmp->read_send();
              vTaskDelay(200/portTICK_PERIOD_MS); 
              tmp =tmp->next;
            }
            
            vTaskDelay(100/portTICK_PERIOD_MS); 
            gpio_intr_enable(I2CINT);
        }
    }
}
#endif

static void termostat__poll(void *arg)
{
  Termostat *tmp = get_termostat_head_handle();
  //ESP_LOGI(TAG,"Termostat Polling");
  while(tmp)
    {
      //printf("ID %d\n",tmp->get_id());
      tmp->read_gateway();
      vTaskDelay(200/portTICK_PERIOD_MS); 
      tmp =tmp->next;
    }
}


static void late_process(void *arg)
{
    ESP_LOGW(TAG,"Late Process start..");
    get_uart_time();

    if (GlobalConfig.start_value!=3)
      {
       // Net.wifi_update_clients(); 


        //remote cihazlar hazırlandı ancak statusları belli degil
        //statuslarını okuman lazım
        //full_status_read();        
        ESP_LOGW(TAG,"Late Process END");
      } ;//else refresh_device();

    cihazlar.init(&rs485); 
    cihazlar.start(true);  

   
      esp_timer_handle_t ztimer = NULL;
      esp_timer_create_args_t arg1 = {};
      arg1.callback = &termostat__poll;
      arg1.name = "tpoll";
      ESP_ERROR_CHECK(esp_timer_create(&arg1, &ztimer));
      ESP_ERROR_CHECK(esp_timer_start_periodic(ztimer, 30000000));
      termostat__poll(NULL);
    
}


extern "C" void app_main()
{
    esp_log_level_set("gpio", ESP_LOG_NONE);
    esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("wifi_init", ESP_LOG_NONE);
    esp_log_level_set("system_api", ESP_LOG_NONE);
    esp_log_level_set("phy_init", ESP_LOG_NONE);
    //esp_log_level_set("i2cdev", ESP_LOG_NONE);

    
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    
    #ifdef ATMEGA_CONTROL
      ter_que = xQueueCreate(10, sizeof(uint32_t));
      xTaskCreate(read_task, "tsk00", 4096, NULL, 10, NULL);
    #endif
    

    config();

    //inout_test(pcf);


      //açılıştan sonra gecikmeli olması gereken işlemler.
      //Wifi server yeni açılıyor. clientların baglanıp
      //ip almaları gerekiyor. Bu nedenle clientlarla
      //yapılacak işlemlerin biraz gecikmesi gerekir.
      esp_timer_handle_t ztimer = NULL;
      esp_timer_create_args_t arg1 = {};
      arg1.callback = &late_process;
      arg1.name = "ztim1";
      ESP_ERROR_CHECK(esp_timer_create(&arg1, &ztimer));
      int tmm= 10000000;
      if (GlobalConfig.start_value==3) tmm=4000000;
      ESP_ERROR_CHECK(esp_timer_start_once(ztimer, tmm));
      ESP_LOGW(TAG,"10sn Late Process waiting..");

   //rs485_output_test();

   // test01(50, pcf);
    //test02(50, pcf);
        

    REGOK_ready = xSemaphoreCreateBinary();
    assert(REGOK_ready);

    broadcast_I_REQ();

    //uint8_t _aa=0;
    //xQueueSend(ter_que, &_aa, ( TickType_t ) 10 );
    #ifdef ATMEGA_CONTROL
       gpio_intr_enable(I2CINT);	
    #endif   

    while(true) 
    {

      // if (gpio_get_level((gpio_num_t)33)==0)
      //   {
            //printf("33 DOWN\n");
      //   }

       if (gpio_get_level((gpio_num_t)BUTTON1)==0)
         {
            uint64_t startMicros = esp_timer_get_time();
            uint64_t currentMicros = startMicros;
            bool LL = true;
            while (gpio_get_level((gpio_num_t)BUTTON1)==0)
              {
                  //CPU1_Reset(); 
                  
                  currentMicros = esp_timer_get_time()-startMicros;
                  if ((currentMicros%200000)==0) {gpio_set_level((gpio_num_t)LED, LL);LL=!LL;} 
                  if ((currentMicros)>5000000) {
                      GlobalConfig.start_value=1; 
                      disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0); 
                      esp_restart();   
                                                }   
                  vTaskDelay(10/portTICK_PERIOD_MS); 
                               
              }
         }
      

      vTaskDelay(10/portTICK_PERIOD_MS);
    }
  
  
}

