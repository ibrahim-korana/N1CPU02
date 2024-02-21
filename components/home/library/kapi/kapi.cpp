
#include "kapi.h"
#include "esp_log.h"

static const char *ASANSOR_TAG = "DOOR";

//Bu fonksiyon inporttan tetiklenir
void Door::func_callback(void *arg, port_action_t action)
{   
   
}

void Door::status_change(home_status_t st)
{
  Base_Port *target = port_head_handle;
  while (target) {
      if (target->type == PORT_OUTPORT) 
          {
            if (st.counter==1 && strcmp(target->name,"ON")==0) target->set_status(true);
            if (st.counter==2 && strcmp(target->name,"OFF")==0) target->set_status(true);
            if (st.counter==0) {
                target->set_status(false);
            }
          }
      target = target->next;
  }
  if (status.status) status.status=0;
  write_status();
  if (function_callback!=NULL) function_callback((void *)this, get_status());
}

//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void Door::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        stat.status = 1;
        local_set_status(stat);
        status_change(stat);        
        tim_start();
    } else {
        if (command_callback!=NULL) command_callback((void *)this, stat);
    }
}

//Eger mevcut durumdan fark var ise statusu ayarlar ve/veya callback çağrılır
//durum degişimi portları etkilemez. bu fonksiyon daha çok remote cihaz 
//durum değişimleri için kullanılır.
void Door::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.stat!=stat.stat) chg=true;
    if (status.active!=stat.active) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(ASANSOR_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}

void Door::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "status", stt.status);
}

void Door::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool Door::get_port_json(cJSON* obj)
{
  return false;
}

//yangın bildirisi aldığında ne yapacak
void Door::fire(bool stat)
{
}

void Door::senaryo(char *par)
{
   
}

void Door::tim_callback(void* arg)
{   
    Door *aa = (Door *)arg;
    home_status_t ss = aa->get_status();
    ss.counter = 0;
    aa->status_change(ss);
}

void Door::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Door::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_once(qtimer, timer * 1000000));
}
void Door::init(void)
{
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &tim_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        if (timer<2) timer=2;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
    }
}

