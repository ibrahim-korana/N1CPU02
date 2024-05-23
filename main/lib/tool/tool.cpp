
#include "tool.h"
#include "esp_log.h"


static const char *TOOL_TAG = "TOOL";

void test_tip(i2c_dev_t **pcf, Storage *disk )
{
  
  const char *name1="/config/test.json";
    if (disk->file_search(name1))
      {
        int fsize = disk->file_size(name1); 
        char *buf = (char *) malloc(fsize+5);
        if (buf==NULL) {ESP_LOGE("TEST", "memory not allogate"); return ;}
        FILE *fd = fopen(name1, "r");
        if (fd == NULL) {ESP_LOGE("TEST", "%s not open",name1); return ;}
        fread(buf, fsize, 1, fd);
        fclose(fd);
        
        DynamicJsonDocument doc(fsize+5);
        DeserializationError error = deserializeJson(doc, buf);
        if (error) {
          ESP_LOGE("TEST","deserializeJson() failed: %s",error.c_str());
          return;
        } 
          int test = doc["test"]; 
          int time = doc["time"]; 
          if (test==1) inout_test(pcf);
          if (test==2) test01(time,pcf);
          if (test==3) test02(time,pcf);
          if (test==4) rs485_output_test();
          if (test==5) rs485_input_test();
          if (test==6) rs485_echo_test();

        doc.clear();                       
        free(buf);
      }
}




esp_err_t pcf_init(i2c_dev_t *pp, uint8_t addr, uint8_t retry, bool log, gpio_num_t LLD)
{
   esp_err_t err = ESP_OK;  
   gpio_set_level((gpio_num_t)LLD, 1); 
   err = pcf8574_init_desc(pp, addr, (i2c_port_t)0, (gpio_num_t)21, (gpio_num_t)22); 
   if (err!=ESP_OK) ESP_LOGE(TOOL_TAG,"INIT ERROR 0x%02X %X %s",addr, err, esp_err_to_name(err));  
   int i=0;
   err = i2c_dev_probe(pp,I2C_DEV_READ);
   if (err!=ESP_OK) ESP_LOGE(TOOL_TAG,"PROP ERROR 0x%02X %X %s",addr, err, esp_err_to_name(err));  
   while(err!=ESP_OK)
         {
            gpio_set_level((gpio_num_t)LLD, 1); 
            vTaskDelay(500/portTICK_PERIOD_MS);
            err = pcf8574_init_desc(pp, addr, (i2c_port_t)0, (gpio_num_t)21, (gpio_num_t)22);
            if (i++>retry) return ESP_FAIL;
            err = i2c_dev_probe(pp,I2C_DEV_READ);
            if (log) ESP_LOGE(TOOL_TAG,"Addr 0x%02X Count %2i Error : %X %s\n",addr,i, err, esp_err_to_name(err));
         }
  gpio_set_level((gpio_num_t)LLD, 0);                      
  return err;       
}

bool out_pin_convert(uint8_t pin, uint8_t *pcfno, uint8_t *pcfpin)
{
  bool ret=false;
  switch (pin)
   {
      case 1: {*pcfno=1;*pcfpin=0;ret=true;break;}
      case 2: {*pcfno=1;*pcfpin=1;ret=true;break;}
      case 3: {*pcfno=1;*pcfpin=2;ret=true;break;}
      case 4: {*pcfno=1;*pcfpin=3;ret=true;break;}
      case 5: {*pcfno=1;*pcfpin=7;ret=true;break;}
      case 6: {*pcfno=1;*pcfpin=6;ret=true;break;} //role cekmiyor

      case 7: {*pcfno=0;*pcfpin=1;ret=true;break;}
      case 8: {*pcfno=0;*pcfpin=2;ret=true;break;}
      case 9: {*pcfno=0;*pcfpin=3;ret=true;break;}
      case 10: {*pcfno=0;*pcfpin=4;ret=true;break;}
      case 11: {*pcfno=0;*pcfpin=5;ret=true;break;}
      case 12: {*pcfno=0;*pcfpin=6;ret=true;break;} //led yanmıyor

      case 13: {*pcfno=4;*pcfpin=0;ret=true;break;} 
      case 14: {*pcfno=4;*pcfpin=1;ret=true;break;} 
      case 15: {*pcfno=4;*pcfpin=2;ret=true;break;} 


   }
  return ret;
}

bool in_pin_convert(uint8_t pin, uint8_t *pcfno, uint8_t *pcfpin)
{
  bool ret=false;
  if (pin>=1 && pin<=8) {
                          *pcfno=2;
                          *pcfpin=pin-1;
                          ret=true;
                        }
  if (pin==9) {*pcfno=0; *pcfpin=7; ret=true; }    
  if (pin==10) {*pcfno=0; *pcfpin=0; ret=true; }                 
  if (pin==11) {*pcfno=1; *pcfpin=4; ret=true; }
  if (pin==12) {*pcfno=1; *pcfpin=5; ret=true; }  

  if (pin==13) {*pcfno=4; *pcfpin=3; ret=true; }  
  if (pin==14) {*pcfno=4; *pcfpin=4; ret=true; }  
  if (pin==15) {*pcfno=4; *pcfpin=5; ret=true; }  
  if (pin==16) {*pcfno=4; *pcfpin=6; ret=true; }  
  if (pin==17) {*pcfno=4; *pcfpin=7; ret=true; }  

  return ret;
}

void inout_test(i2c_dev_t **pcf)
{
  #define BUTTON1 34
  #define WATER 27

    uint16_t  a_delay=800;
    uint16_t a_delay1=500;
    ESP_LOGW(TOOL_TAG,"\n\nIN/OUT TEST BASLADI\n\n");
    esp_log_level_set("ANAKUTU_CPU2", ESP_LOG_NONE);


    for (int q=0;q<2;q++)
    {
      if (q==0) {a_delay=100;a_delay1=50;}
      if (q==1) {a_delay=500;a_delay1=500;}
      if (q==2) {a_delay=800;a_delay1=500;}
      if (q==3) {a_delay=1500;a_delay1=500;}
      uint8_t say = 13;
      #ifdef PCF_4
         say=16;
      #endif   

      for (int j=1;j<say;j++)
        {  
          uint8_t pc=0, pn =0;
          out_pin_convert(j,&pc,&pn);
          pcf8574_pin_write(pcf[pc],pn,0);
          if ((j%2)==0)
            {
                ESP_LOGI(TOOL_TAG,"%d --> %02d <-- PCF %d pin=%d level=0",q,j,pc,pn);
            } else {
                ESP_LOGW(TOOL_TAG,"%d %02d PCF %d pin=%d level=0",q,j,pc,pn);
            }    
          vTaskDelay(a_delay/portTICK_PERIOD_MS);
          pcf8574_pin_write(pcf[pc],pn,1);
          //ESP_LOGI(TOOL_TAG,"%02d PCF %d pin=%d level=1",j, pc,pn);
          vTaskDelay(a_delay1/portTICK_PERIOD_MS);
        } 
    }

    ESP_LOGE(TOOL_TAG,"GIRISLERI GND'ye çekerek test ediniz. Cikis icin RESET\n");
    uint8_t val0=0, val1 = 0, val2=0, val3=0, val00 = 0xff, val11=0xff, val22 = 0xff, val33=0xff;
    bool rep = true;
    while (rep)
      {  
        
        pcf8574_port_read(pcf[0], &val0);
        pcf8574_port_read(pcf[1], &val1);
        pcf8574_port_read(pcf[2], &val2);
        #ifdef PCF_4
          pcf8574_port_read(pcf[4], &val3);
        #endif

        if (val0!=val00) {
                 val00 = val0;
                 if (val0!=0xFF) ESP_LOGI(TOOL_TAG,"PCF0 = %02X Flag " BIN_PATTERN,val0,BYTE_TO_BIN(val0));
                         }
        if (val1!=val11) {
                 val11 = val1;
                 if (val1!=0xFF) ESP_LOGI(TOOL_TAG,"PCF1 = %02X Flag " BIN_PATTERN,val1,BYTE_TO_BIN(val1));
                         }  
        if (val2!=val22) {
                 val22 = val2;
                 if (val2!=0xFF) ESP_LOGI(TOOL_TAG,"PCF2 = %02X Flag " BIN_PATTERN,val2,BYTE_TO_BIN(val2));
                         }    
        #ifdef PCF_4
          if (val3!=val33) {
                 val33 = val3;
                 if (val3!=0xFF) ESP_LOGI(TOOL_TAG,"PCF3 = %02X Flag " BIN_PATTERN,val3,BYTE_TO_BIN(val3));
                          }  
        #endif                                      
        //if (gpio_get_level((gpio_num_t)WATER)==1) {ESP_LOGI(TOOL_TAG,"WATER UP");vTaskDelay(1000/portTICK_PERIOD_MS);}
        if (gpio_get_level((gpio_num_t)BUTTON1)==0) {
                              ESP_LOGI(TOOL_TAG,"BUTTON 1 UP");
                              vTaskDelay(1000/portTICK_PERIOD_MS);
                              }
                              //rs485_output_test();

        vTaskDelay(50/portTICK_PERIOD_MS);
      }   
      
}


void test01(uint16_t a_delay2, i2c_dev_t **pcf)
{
   while (true)
   {
     for (int j=1;j<13;j++)
        {  
          uint8_t pc=0, pn =0;
          out_pin_convert(j,&pc,&pn);
          pcf8574_pin_write(pcf[pc],pn,0);
          vTaskDelay(a_delay2/portTICK_PERIOD_MS);
          pcf8574_pin_write(pcf[pc],pn,1);
          vTaskDelay(1/portTICK_PERIOD_MS);
        } 
   }    
}

void test02(uint16_t a_delay2, i2c_dev_t **pcf)
{
   uint8_t aa = 0x00;
   while (true)
   {
      pcf8574_port_write(pcf[0],aa);
      pcf8574_port_write(pcf[1],aa);
      #ifdef PCF_4
         pcf8574_port_write(pcf[4],aa);
      #endif   
      vTaskDelay(a_delay2/portTICK_PERIOD_MS);
      if (aa==0x00) aa=0xFF; else aa=0x00;
   }    
}


#include "rs485.h"

void rs485_output_test(void)
{
bool rep = true;
uint16_t counter = 0;

RS485_config_t rs485_cfg={};
rs485_cfg.uart_num = 1;
rs485_cfg.dev_num  = 253;
rs485_cfg.rx_pin   = 25;
rs485_cfg.tx_pin   = 26;
rs485_cfg.oe_pin   = 13;
rs485_cfg.baud     = 460800;

uart_driver_delete((uart_port_t)rs485_cfg.uart_num);

        uart_config_t uart_config = {};
        uart_config.baud_rate = 115200;
        uart_config.data_bits = UART_DATA_8_BITS;
        uart_config.parity = UART_PARITY_DISABLE;
        uart_config.stop_bits = UART_STOP_BITS_1;
        uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        uart_config.rx_flow_ctrl_thresh = 122;

ESP_ERROR_CHECK(uart_driver_install((uart_port_t)rs485_cfg.uart_num, BUF_SIZE * 2, 0, 0, NULL, 0));
ESP_ERROR_CHECK(uart_param_config((uart_port_t)rs485_cfg.uart_num, &uart_config));
ESP_ERROR_CHECK(uart_set_pin((uart_port_t)rs485_cfg.uart_num, rs485_cfg.tx_pin, rs485_cfg.rx_pin, rs485_cfg.oe_pin, UART_PIN_NO_CHANGE));
ESP_ERROR_CHECK(uart_set_mode((uart_port_t)rs485_cfg.uart_num, UART_MODE_RS485_HALF_DUPLEX));
ESP_ERROR_CHECK(uart_set_rx_timeout((uart_port_t)rs485_cfg.uart_num, 3));
//uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
//echo_send(rscfg->uart_num, "Start RS485 Output test.\r\n", 24);
char * bff = (char *)malloc(10);
uint8_t count = 0;
while (rep)
{ 
  sprintf(bff,"%02d",counter++);
  if (counter>98) counter=0; 
  //echo_send(rscfg->uart_num, "A", 1);
  uart_write_bytes((uart_port_t)rs485_cfg.uart_num, bff, strlen(bff));
  uart_wait_tx_done((uart_port_t)rs485_cfg.uart_num, 10);

    printf("%s ", bff);
    fflush(stdout);

    if(count++>10) {
      count=0;
      printf("\n>> ");fflush(stdout);
    }

  vTaskDelay(5/portTICK_PERIOD_MS);

        
      }
}

void rs485_input_test(void)
{
bool rep = true;
uint16_t counter = 0;

RS485_config_t rs485_cfg={};
rs485_cfg.uart_num = 1;
rs485_cfg.dev_num  = 253;
rs485_cfg.rx_pin   = 25;
rs485_cfg.tx_pin   = 26;
rs485_cfg.oe_pin   = 13;
rs485_cfg.baud     = 460800;

uart_driver_delete((uart_port_t)rs485_cfg.uart_num);

        uart_config_t uart_config = {};
        uart_config.baud_rate = 115200;
        uart_config.data_bits = UART_DATA_8_BITS;
        uart_config.parity = UART_PARITY_DISABLE;
        uart_config.stop_bits = UART_STOP_BITS_1;
        uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        uart_config.rx_flow_ctrl_thresh = 122;

ESP_ERROR_CHECK(uart_driver_install((uart_port_t)rs485_cfg.uart_num, BUF_SIZE * 2, 0, 0, NULL, 0));
ESP_ERROR_CHECK(uart_param_config((uart_port_t)rs485_cfg.uart_num, &uart_config));
ESP_ERROR_CHECK(uart_set_pin((uart_port_t)rs485_cfg.uart_num, rs485_cfg.tx_pin, rs485_cfg.rx_pin, rs485_cfg.oe_pin, UART_PIN_NO_CHANGE));
ESP_ERROR_CHECK(uart_set_mode((uart_port_t)rs485_cfg.uart_num, UART_MODE_RS485_HALF_DUPLEX));
ESP_ERROR_CHECK(uart_set_rx_timeout((uart_port_t)rs485_cfg.uart_num, 3));

char * bff = (char *)calloc(1,10);
uint8_t count = 0;

while (true)
{ 
  int len = uart_read_bytes((uart_port_t)rs485_cfg.uart_num, bff, 2, (100 / portTICK_PERIOD_MS));
  if (len > 0) {
    printf("%s ", bff);
    fflush(stdout);

    if(count++>10) {
      count=0;
      printf("\n>> ");fflush(stdout);
    }
  } else ESP_ERROR_CHECK(uart_wait_tx_done((uart_port_t)rs485_cfg.uart_num, 10));

}

}


static void echo_send( uart_port_t port, uint8_t count)
{
    char* data = (char*) calloc(1,6);
    sprintf(data,"%02d",count);
    if (uart_write_bytes(port, data, 2) != 2) {
        ESP_LOGE(TOOL_TAG, "Göndermede kritik hata...");
        abort();
    }
    printf("[%02X-", count); fflush(stdout); 
    free(data);
}

#define ECHO_PORT UART_NUM_1
#define ECHO_BAUD_RATE 115200
#define ECHO_TX 26
#define ECHO_RX 25
#define ECHO_OE 13
#define ECHO_READ_TOUT (3)


void rs485_echo_task(void *arg)
{
     uart_port_t uart_num = ECHO_PORT;
    uart_config_t uart_config = {
        .baud_rate = ECHO_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, ECHO_TX, ECHO_RX, ECHO_OE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(uart_num, UART_MODE_RS485_HALF_DUPLEX));
    ESP_ERROR_CHECK(uart_set_rx_timeout(uart_num, ECHO_READ_TOUT));
    char* data = (char*) calloc(1,6);
    uint8_t count=0, ii=0, yy=0;
    bool ww = false;

    echo_send(uart_num, count);
    while (1) {
        int len = uart_read_bytes(uart_num, data, 2, (100 / portTICK_PERIOD_MS));
        if (len > 0) {
          uint8_t rr = atoi(data);
          printf("%02x] ", rr); fflush(stdout); 
          count=atoi(data)+1;
          if (((++ii)%10)==0) {printf("\n");ii=0;}
          if (((++yy)%50)==0) {ww=!ww;yy=0;gpio_set_level(GPIO_NUM_0,ww);}
          
          
          echo_send(uart_num, count);
                     } else {
                      ESP_ERROR_CHECK(uart_wait_tx_done(uart_num, 10));
                     }
    }
  vTaskDelete(NULL);
}

void rs485_echo_test(void)
{
    xTaskCreate(rs485_echo_task, "echo_task", 4096, NULL, 5, NULL);
    uint8_t ee = 0;
    while(1) {    
      if (gpio_get_level(GPIO_NUM_34)==0)
        {
           echo_send(ECHO_PORT, ee);
           vTaskDelay(500/portTICK_PERIOD_MS);
        }       
      vTaskDelay(100/portTICK_PERIOD_MS);

    } 


} 
void init_spi(void)
{
  
}