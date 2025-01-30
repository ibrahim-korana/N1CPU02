#ifndef _TERMOSTAT_H
#define _TERMOSTAT_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "i2cdev.h"
#include "i2c_gateway.h"
#include "esp_timer.h"
#include "rs485.h"

class Termostat {
    public:
      Termostat(char *nm, uint8_t iid, rs485_callback_t cb, transmisyon_t trn, RS485 *r485) {
        id = iid;
        name = (char*)malloc(10);
        trns = trn;
        rs485 = r485;
        strcpy(name,nm);
        callback = cb;
        gateway_init_desc(&dev,0x04,(i2c_port_t)0,gpio_num_t(21),gpio_num_t(22));
        Read_Sem = xSemaphoreCreateBinary();
        
      };
      ~Termostat() {
        free(name);       
      };

      void init(void);
      uint8_t get_temp(void) {return temp;}
      uint8_t get_set(void) {return set;}
      uint8_t get_id(void) {return id;}
      char *get_name(void) {return name;}
      esp_err_t read_gateway(void);
      esp_err_t read_gateway_temp(void);
      void set_set_temp(float s_temp);
      void read_send(void);

      uint8_t get_error(void) {return error;}
      void set_error(uint8_t e) {error=e;}
      uint8_t inc_error(void) {error++; return error;}
      void local_send(void);

      rs485_callback_t callback;
      Termostat *next = NULL;
      
      
              
    private:
      esp_timer_handle_t qtimer = NULL; 
    protected:  
      char *name;
      uint8_t id; 
      transmisyon_t trns=TR_PJON;
      RS485 *rs485;
      i2c_dev_t dev = {};   
      volatile uint8_t temp=0;
      volatile uint8_t set=0; 
      uint8_t error=0;
      uint8_t ttemp=0;
      uint8_t tset=0; 
      SemaphoreHandle_t Read_Sem;
      volatile bool Read_S = true;
      
      
      
      static void ter_tim_callback(void* arg); 
      void tim_start(void);
      void tim_stop(void);  
      static void at_read_task(void *arg);

};

#endif
