#include "piston.h"
#include "esp_log.h"

static const char *PISTON_TAG = "PISTON";

//Bu fonksiyon inporttan tetiklenir
void Piston::func_callback(void *arg, port_action_t action)
{   
    if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port;
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
        button_event_t evt = iot_button_get_event(btn);
        Piston *fnc = (Piston *) arg;
        home_status_t stat = fnc->get_status();
        bool change = false;

        //printf("Event %d\n",evt);

        if (evt==BUTTON_LONG_PRESS_START) 
          {
              fnc->buton_durumu = PISBTN_LONG;
          }
        if (evt==BUTTON_PRESS_DOWN)
          {
            if (fnc->Motor!=PIS_MOTOR_STOP)
                {
                //motor calışıyor. motoru ve timerı durdur
                fnc->Motor=PIS_MOTOR_STOP;
                fnc->tim_stop();
                stat.status = (int)PIS_HALF_CLOSED;
                change=true;
                fnc->temp_status = PIS_UNKNOWN;
                fnc->motor_action();
                } else {
                //motor stop durumda. motoru çalıştırıp zamanı başlat
                fnc->temp_status = (pis_status_t) fnc->status.status;
                if (strcmp(prt->name,"UP")==0) fnc->Motor=PIS_MOTOR_UP;
                if (strcmp(prt->name,"DOWN")==0) fnc->Motor=PIS_MOTOR_DOWN;
                stat.status = (int) PIS_PROCESS;
                change=true;
                fnc->tim_stop();
                fnc->tim_start();
                fnc->motor_action();
                }
          }
          if (evt==BUTTON_PRESS_UP) 
          {
            if (fnc->buton_durumu==PISBTN_LONG || fnc->eskitip) {
                fnc->temp_status = PIS_UNKNOWN;
                fnc->buton_durumu = PISBTN_UNKNOWN;
                //motoru ve timer ı durdur
                stat.status = (int) PIS_HALF_CLOSED;
                change=true;
                fnc->Motor=PIS_MOTOR_STOP;
                fnc->tim_stop();
                fnc->motor_action();
            }; 
          }
       if (change) fnc->local_set_status(stat,true);
     }
}

void Piston::tim_stop(void){
  if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Piston::bek_start(void){
  ESP_ERROR_CHECK(esp_timer_start_once(bektimer, 1000000));
}


void Piston::bek_stop(void){
  if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Piston::tim_start(void){
  if (status.counter==0 || status.counter>60) status.counter=10;
  ESP_ERROR_CHECK(esp_timer_start_once(qtimer, status.counter * 1700000));
}

const char* Piston::convert_motor_status(uint8_t mot)
{
  if (mot==PIS_MOTOR_STOP) return "STOP";
  if (mot==PIS_MOTOR_UP) return "UP";
  if (mot==PIS_MOTOR_DOWN) return "DOWN";
  return "UNKNOWN";
}
void Piston::motor_action(bool callback) 

{  /*
    0 stop
    1 up
    2 down
    */
  Base_Port *target = port_head_handle;
  Base_Port *target0 = port_head_handle;
  while (target) { 
    if (Motor==PIS_MOTOR_STOP) 
    {
        if (target->type==PORT_OUTPORT) target->set_status(false);
    }
    if (Motor==PIS_MOTOR_UP) 
    {
        while (target0) {
            if (target0->type==PORT_OUTPORT && strcmp(target0->name,"MOTOR")==0) {
              target0->set_status(true);
              vTaskDelay(2000/portTICK_PERIOD_MS);
            }
            target0 = target0->next;
        } 
        if (target->type==PORT_OUTPORT && strcmp(target->name,"UP")==0) target->set_status(true);
    }
    if (Motor==PIS_MOTOR_DOWN) 
    {
        while (target0) {
            if (target0->type==PORT_OUTPORT && strcmp(target0->name,"MOTOR")==0) {
              target0->set_status(true);
              vTaskDelay(2000/portTICK_PERIOD_MS);
            }
            target0 = target0->next;
        } 
        if (target->type==PORT_OUTPORT && strcmp(target->name,"DOWN")==0) target->set_status(true);
        //if (target->type==PORT_OUTPORT && strcmp(target->name,"MOTOR")==0) target->set_status(true);
    }  
    target = target->next;
  }

  ESP_LOGI(PISTON_TAG,"Motor Status : %s",convert_motor_status(Motor));
  if(function_callback!=NULL && callback) function_callback((void *)this,status);
}

void Piston::cur_tim_callback(void* arg)
{
    Piston *mthis = (Piston *)arg;
    //Base_Function *base = (Base_Function *)arg;
    home_status_t st = mthis->get_status();
    ESP_LOGI(PISTON_TAG,"Bekleme zamanı doldu");
    if (mthis->temp_status!=PIS_UNKNOWN)
    {   
        ESP_LOGI(PISTON_TAG,"MOTOR Durduruluyor..");
        st.status = (int) mthis->temp_status;
        mthis->local_set_status(st,true);   
        mthis->temp_status = PIS_UNKNOWN;
        mthis->Motor= PIS_MOTOR_STOP;
        mthis->motor_action(); 
    }
}

void Piston::init(void)
{
  
  if (!genel.virtual_device)
  {
    status.stat = true;
    status.status = PIS_CLOSED;
    esp_timer_create_args_t arg = {};
    arg.callback = &cur_tim_callback;
    arg.name = "tim0";
    arg.arg = (void *) this;
    ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
    
    //esp_timer_create_args_t arg0 = {};
    //arg0.callback = &mot_tim_callback;
    //arg0.name = "tim1";
    //arg0.arg = (void *) this; 
    //ESP_ERROR_CHECK(esp_timer_create(&arg0, &bektimer));

    Motor= PIS_MOTOR_STOP;
    motor_action(false);
    if (status.counter==0 || status.counter>60) status.counter=10;
  }
}

void Piston::set_status(home_status_t stat)
{
  if (!genel.virtual_device)
    {
      local_set_status(stat,true);
      if (temp_status == PIS_UNKNOWN) {
          if (status.status==0 || status.status==2)
            {     tim_stop();
                  tim_start();
                  temp_status = (pis_status_t) status.status;
                  status.status = (int) PIS_PROCESS;
                  if (temp_status == PIS_CLOSED) Motor = PIS_MOTOR_DOWN;
                  if (temp_status == PIS_OPENED) Motor = PIS_MOTOR_UP;
                  motor_action(false);
            }
      } else {
          status.status = (int)PIS_HALF_CLOSED;
          tim_stop();
          temp_status = PIS_UNKNOWN;
          Motor= PIS_MOTOR_STOP;
          motor_action(false);
      }
      write_status();
      if (function_callback!=NULL) function_callback((void *)this, get_status());
    } else {
        if (command_callback!=NULL) command_callback((void *)this, stat);
    }
}

void Piston::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.status!=stat.status) chg=true;
    if (status.active!=genel.active) chg=true;
    if (status.counter!=stat.counter) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(PISTON_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}

void Piston::ConvertStatus(home_status_t stt, cJSON *obj)
{
    cJSON_AddNumberToObject(obj, "status", stt.status);
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "coun", stt.counter);
}

void Piston::get_status_json(cJSON* obj)
{
    return ConvertStatus(status, obj);
}

bool Piston::get_port_json(cJSON* obj)
{
  return false;
}

void Piston::fire(bool stat)
{
    
}

void Piston::senaryo(char *par)
{
    if(strcmp(par,"on")==0)
      {
        status.status = PIS_OPENED;
        set_status(status);
      }
    if(strcmp(par,"off")==0)
      {
        status.status = PIS_CLOSED;
        set_status(status);
      }  
}


void Piston::role_change(void)
{
  Base_Port *target = port_head_handle;
  while (target) 
  { 
    Base_Port *pp = (Base_Port *) target;
    if (pp->type == PORT_OUTPORT) 
    {
      if (strcmp(pp->name,"UP")==0)
      {
        strcpy(pp->name,"DOWN");
      } else strcpy(pp->name,"UP");
    }
    target = target->next;
  } 
}

void Piston::anahtar_change(void)
{
  Base_Port *target = port_head_handle;
  while (target) 
  { 
    Base_Port *pp = (Base_Port *) target;
    if (pp->type == PORT_INPORT) 
    {
      if (strcmp(pp->name,"UP")==0)
      {
        strcpy(pp->name,"DOWN");
      } else strcpy(pp->name,"UP");
    }
    target = target->next;
  }  
}

