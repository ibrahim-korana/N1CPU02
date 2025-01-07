#ifndef _AIR_H
#define _AIR_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "jsontool.h"

typedef struct {
  union {
    struct {
      uint8_t isitma_sogutma:1; //0 isitma 1 sogutma
      uint8_t auto_manuel:1; //0 auto 1 manuel
      uint8_t motor_aktif_pasif:1; //0 pasif 1 aktif; manulede roleyi aç kapat 
      uint8_t start_stop:1; //1 start 0 stop;
      uint8_t role_durumu:1; //1 start 0 stop rölenin aktifteki durumu
      uint8_t gece_gunduz:1; //1 gece 0 gündüz
      uint8_t bos:2;
          } st;
	    uint8_t int_st;
  };
} air_status_t;

typedef struct {
   air_status_t stat;
   float temp;
   float set_temp;     
} air_t;


class Air : public Base_Function {
    public:
      Air(int id, function_callback_t cb, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"air");
        function_callback = cb;
        statustype = 1;
        disk = dsk;
        disk.read_status(&status,genel.device_id);
        //printf("BASLANGIC COUNTER %02X first %d\n",status.counter,status.first);
        local_port_callback = &func_callback;        
        if (!status.first) {             
              memset(&status,0,sizeof(status));
              status.stat     = true;
              status.set_temp = 25;
              status.temp     = 26;
              status.active   = true;
              status.first    = true;
              status.status   = 0;
                durum.stat.st.isitma_sogutma = !((global & 0x01)==0x01); //1 sogutma 
                durum.stat.st.auto_manuel = 0; //Auto
                durum.stat.st.gece_gunduz = 1; //Gunduz;
                durum.stat.st.motor_aktif_pasif = 0; //pasif;
                durum.stat.st.start_stop = 1; //Start
                durum.stat.st.role_durumu = 0; //pasif
                durum.stat.st.bos = 0;
                status.counter  = durum.stat.int_st;
              //printf("BASLANGIC %02X SS %d\n",status.counter, durum.stat.st.start_stop);  
              write_status();
        }    
        genel.active = status.active;     
      };
      ~Air() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);      
      void remote_set_status(home_status_t stat, bool callback_call=true);
      void set_sensor(char *name, home_status_t stat);
           
      void get_status_json(cJSON* obj) override;
      bool get_port_json(cJSON* obj) override;
      void init(void) override;

      void fire(bool stat);
      void senaryo(char *par);
      //void set_start(bool drm); 
      void ConvertStatus(home_status_t stt, cJSON* obj);
      bool send_gateway=false;

      void status_to_durum(void);
      void durum_to_status(void);
        
    private:
      

    protected:  
      air_t durum = {};
      uint8_t min_temp = 18;
      uint8_t max_temp = 30;
      uint8_t isitma_sogutma = 0; //1 sogutma 0 ısıtma

      

      static void func_callback(void *arg, port_action_t action);  
      void temp_action(bool send=true);    
};

#endif