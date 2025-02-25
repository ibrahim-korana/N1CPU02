#ifndef _SECURITY_H
#define _SECURITY_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"

typedef enum {
    SEC_CLOSED = 0,
    SEC_PROCESS,
    SEC_OPENED,
    SEC_UNKNOWN, 
    SEC_ALARM,
    SEC_STOP,
} security_status_t;

//ESP_EVENT_DECLARE_BASE(SECURITY_EVENTS);

class Security : public Base_Function {
    public:
      Security(int id, function_callback_t cb, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"sec");
        function_callback = cb;
        statustype = 1;
        disk = dsk;
        disk.read_status(&status,genel.device_id);
        local_port_callback = &func_callback;
        if (!status.first) {             
              memset(&status,0,sizeof(status));
              status.stat     = false;
              status.active   = true;
              status.first    = true;
              status.status   = 0;
              daire = 0;
              blok = 0;
              write_status();
        }        
      };
      ~Security() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);
      void remote_set_status(home_status_t stat, bool callback_call=true);
           
      void get_status_json(cJSON* obj) override;
      bool get_port_json(cJSON* obj) override;
      void get_port_status_json(cJSON* obj) override;
      void init(void) override;

      void alarm_close(void);
      void alarm_open(void);
      void alarm_stop(void);
      void tim_stop(void);
      void tim_start(void);  
      void cikis_port_active(void);
      void set_daire(uint8_t dai, uint8_t blk) {
        daire = dai;
        blok = blk;
      }


      void fire(bool stat);
      void ConvertStatus(home_status_t stt, cJSON* obj);
        
    private:
      esp_timer_handle_t qtimer = NULL;
      uint8_t daire;
      uint8_t blok;
      
    protected:  
      static void func_callback(void *arg, port_action_t action); 
      static void tim_callback(void* arg);     
         
};

#endif