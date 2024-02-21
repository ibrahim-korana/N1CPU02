#ifndef _TERMOSTAT_H
#define _TERMOSTAT_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "i2cdev.h"
#include "i2c_gateway.h"
#include "esp_timer.h"

class Termostat {
    public:
      Termostat(char *nm, uint8_t iid, rs485_callback_t cb) {
        id = iid;
        name = (char*)malloc(10);
        strcpy(name,nm);
        callback = cb;
        gateway_init_desc(&dev,0x04,(i2c_port_t)0,gpio_num_t(21),gpio_num_t(22));
        
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

      rs485_callback_t callback;
      Termostat *next = NULL;
      
              
    private:
      esp_timer_handle_t qtimer = NULL; 
    protected:  
      char *name;
      uint8_t id; 
      i2c_dev_t dev = {};   
      uint8_t temp=0;
      uint8_t set=0; 
      uint8_t error=0;
      
      
      static void ter_tim_callback(void* arg); 
      void tim_start(void);
      void tim_stop(void);  
};

#endif
