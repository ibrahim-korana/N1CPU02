#ifndef _PISTON_H
#define _PISTON_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"

typedef enum {
    PIS_CLOSED = 0,
    PIS_HALF_CLOSED, 
    PIS_OPENED,
    PIS_PROCESS,
    PIS_UNKNOWN, 
} pis_status_t;

typedef enum {
    PIS_MOTOR_STOP = 0,
    PIS_MOTOR_DOWN, 
    PIS_MOTOR_UP,
} pis_motor_status_t;

typedef enum {
    PISBTN_UNKNOWN = 0,
    PISBTN_SHORT, 
    PISBTN_LONG,
} pis_btn_stat_t;

class Piston : public Base_Function {
    public:
      Piston(int id, function_callback_t cb, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"pis");
        function_callback = cb;
        statustype = 4;
        disk = dsk;
        local_port_callback = &func_callback;
        disk.read_status(&status,genel.device_id);
        if (!status.first) {             
              memset(&status,0,sizeof(status));
              status.stat     = false;
              status.active   = true;
              status.first    = true;
              status.status   = 0;
              write_status();
        }      
      };
      ~Piston() {};

      void set_status(home_status_t stat);
      void remote_set_status(home_status_t stat, bool callback_call=true);
      
      //Statusu json olarak dondurur
      void get_status_json(cJSON* obj) override;
      //değişebilir portları dondurur
      bool get_port_json(cJSON* obj) override;
      void init(void);

      void ConvertStatus(home_status_t stt, cJSON *obj);
      void fire(bool stat); 
      void senaryo(char *par);         

      uint8_t get_sure(void) {return sure;}
      void set_sure(uint8_t sr) {sure=sr;}   
      void motor_action(bool callback=true);


      pis_status_t temp_status = PIS_UNKNOWN;
      pis_status_t current_status = PIS_OPENED;
      pis_motor_status_t Motor = PIS_MOTOR_STOP; 
      pis_btn_stat_t buton_durumu = PISBTN_UNKNOWN;
      const char* convert_motor_status(uint8_t mot);

      bool eskitip=false;

      void tim_stop();
      void tim_start();
      void bek_stop();
      void bek_start();

      void role_change(void);
      void anahtar_change(void);

    private:
      esp_timer_handle_t qtimer;
      esp_timer_handle_t bektimer;
      uint8_t sure = 60;
    protected:  
       static void func_callback(void *arg, port_action_t action);  
       static void cur_tim_callback(void* arg);            
};

#endif