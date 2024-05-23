
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
            if (st.stat==true && strcmp(target->name,"ON")==0) target->set_status(true);
            if (st.stat==false && strcmp(target->name,"OFF")==0) target->set_status(true);
          }
      target = target->next;
  }
  status.status = 1;
  write_status();
  if (function_callback!=NULL) function_callback((void *)this, get_status());
}

//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void Door::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        //stat.status = 1;
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
    if (status.status!=stat.status) chg=true;
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
    if (stat)
      {
        //Yangın alarmı alınırsa kapı kilidi açılır.
        status.stat = false;
        set_status(status);
      }
}

void Door::senaryo(char *par)
{
    if(strcmp(par,"on")==0)
      {
        status.stat = false; 
        set_status(status);
      }
    if(strcmp(par,"off")==0)
      {
        status.stat = true; 
        set_status(status);
      }    
}

void Door::tim_callback(void* arg)
{   
    Door *aa = (Door *)arg;
    home_status_t ss = aa->get_status();
    Base_Port *target = aa->port_head_handle;
    while (target) {
        if (target->type == PORT_OUTPORT) 
            {
                if (ss.stat==true && strcmp(target->name,"ON")==0) target->set_status(false);
                if (ss.stat==false && strcmp(target->name,"OFF")==0) target->set_status(false);
            }
        target = target->next;
    }
    ss.status=0;
    aa->local_set_status(ss,true);
    if (aa->function_callback!=NULL) {aa->function_callback(arg, ss);}
}

void Door::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Door::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
      {
         ESP_ERROR_CHECK(esp_timer_start_once(qtimer, timer * 1000000));
      }
}

void Door::door_event(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch(event_id)
    {
        case HOME_DOOR_OPEN : {
            ESP_LOGI(ASANSOR_TAG,"Door OPEN");           
            Door *aa = (Door *)event_handler_arg; 
            home_status_t ss = aa->get_status();
            ss.stat = true;
            aa->set_status(ss);           
            break;
        }
        case HOME_DOOR_CLOSE : {
            ESP_LOGI(ASANSOR_TAG,"Door CLOSE");          
            Door *aa = (Door *)event_handler_arg; 
            home_status_t ss = aa->get_status();
            ss.stat = false;
            aa->set_status(ss);          
            break;
        }
    } 
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
        //Kapı aşağıdaki eventleri dinler 
        esp_event_handler_register(HOME_EVENTS, HOME_DOOR_OPEN, door_event, (void *)this);
        esp_event_handler_register(HOME_EVENTS, HOME_DOOR_CLOSE, door_event, (void *)this);
    }
}

