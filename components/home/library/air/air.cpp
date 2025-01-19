#include "air.h"
#include "esp_log.h"

static const char *AIR_TAG = "AIR";

//Bu fonksiyon inporttan tetiklenir
void Air::func_callback(void *arg, port_action_t action)
{ }

//#define ADEBUG 

void Air::temp_action(bool send)
{
    bool change = false, role = false;
    status_to_durum(); 

    #ifdef ADEBUG
        ESP_LOGI(AIR_TAG,"    Type     %s",(run_stat.mod==0)?"Isıtma" : "Sogutma"); ;
        ESP_LOGI(AIR_TAG,"    Min/Max  %d/%d",run_stat.min_temp,run_stat.max_temp); 
        ESP_LOGI(AIR_TAG,"    Temp/Set %.0f/%.0f",durum.temp,durum.set_temp);
        ESP_LOGI(AIR_TAG,"    Auto     %s",(durum.stat.st.auto_manuel==1)?"Manuel":"Auto");
        ESP_LOGI(AIR_TAG,"    On/Off   %s",(durum.stat.st.start_stop==1)?"Run":"Stop");
        if (durum.stat.st.auto_manuel==1)
           ESP_LOGI(AIR_TAG,"    Anahtar  %s",(durum.stat.st.motor_aktif_pasif==1)?"Aktif":"Pasif");        
    #endif   

    if (durum.stat.st.auto_manuel==0) //Auto Mod
    {
        if (run_stat.mod == 0)
        {
            //Isıtma Modu
            if (durum.temp>=durum.set_temp) {
                                        role=false;
                                        change=true;
                                    } else {
                                            role = true;
                                            change = true;
                                            }
        }
        if (run_stat.mod==1)
        {
            //Sogutma Modu
            if (durum.temp>0)
            {
                if (durum.temp<=durum.set_temp) {
                                            role=false;
                                            change=true;
                                        } else {
                                                role = true;
                                                change = true;
                                                }
            }
        }
    }
    if (durum.stat.st.auto_manuel==1) //Manuel mod
    {
        role = durum.stat.st.motor_aktif_pasif;
        if (durum.temp<run_stat.min_temp || durum.temp>run_stat.max_temp) role=false;
        change = true;
    }
    if (durum.stat.st.start_stop==0)
    {
        role = false;
        change=true;
    }                                  
    if (change)
    {                                
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) durum.stat.st.role_durumu = target->set_status(role);
            target = target->next;
        }
    } 

    #ifdef ADEBUG
     ESP_LOGI(AIR_TAG,"    Motor    %s",(durum.stat.st.role_durumu==1)?"Çalışıyor":"Durdu");
    #endif

    if (send) if (function_callback!=NULL) {
        durum_to_status();
        function_callback((void *)this, get_status());
    }
}


void Air::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        //status.stat = stat.stat;
        status.counter = stat.counter;
        status.temp = stat.temp;
        status.set_temp = stat.set_temp;
        bool chg = (stat.set_temp!=durum.set_temp);
        status_to_durum();
        if (chg) {
           status.set_temp = stat.set_temp;
            //burada set alt tarafa bildirilecek
            send_gateway=true; 
        }
        write_status();
        temp_action(false);
        if (function_callback!=NULL) function_callback((void *)this, get_status());
    } else {
        if (command_callback!=NULL) command_callback((void *)this, stat);
    }
}

void Air::set_sensor(char *name, home_status_t stat)
{
    if (!genel.virtual_device)
    {
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_VIRTUAL) 
                {
                    if (strcmp(target->name,name)==0) {
                        //printf("VIR PORT set %.2f=%.2f %.2f=%.2f \n",stat.temp,status.temp, stat.set_temp,status.set_temp); 
                        home_status_t ss = get_status();
                        ss.temp = stat.temp;
                        ss.set_temp = stat.set_temp;
                        set_status(ss);  
                    } 
                }
            target = target->next;
        } 
    }
}


//Eger mevcut durumdan fark var ise statusu ayarlar ve/veya callback çağrılır
//durum degişimi portları etkilemez. bu fonksiyon daha çok remote cihaz 
//durum değişimleri için kullanılır.
void Air::remote_set_status(home_status_t stat, bool callback_call) {
    
    bool chg = false;
    if (status.active!=stat.active) chg=true;
    if (status.stat!=stat.stat) chg=true;
    if (status.counter!=stat.counter) chg=true;
    if (status.set_temp!=stat.set_temp) chg=true;
    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(AIR_TAG,"%d Status Changed",genel.device_id);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      } 
           
}

void Air::ConvertStatus(home_status_t stt, cJSON* obj)
{
   
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    durum_to_status();   
    cJSON_AddNumberToObject(obj, "coun",status.counter);
    uint16_t jj = (run_stat.max_temp<<8) | (run_stat.min_temp);
    cJSON_AddNumberToObject(obj, "color",jj);
    cJSON_AddNumberToObject(obj, "status", stt.status);
    char *mm;;
    asprintf(&mm,"%2.02f",stt.temp); 
    cJSON_AddNumberToObject(obj, "temp", atof(mm));
    free(mm);
    char *mm1;
    asprintf(&mm1,"%2.02f",stt.set_temp);
    cJSON_AddNumberToObject(obj, "stemp", atof(mm1));
    free(mm1);
}

void Air::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool Air::get_port_json(cJSON* obj)
{
  return false;
}

//yangın bildirisi aldığında ne yapacak
void Air::fire(bool stat)
{
    if (stat) {
        main_temp_status = status;
        //ısıtma yangın ihbarında kapatılır
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    status.status = target->set_status(false);
                }
            target = target->next;
        }
    } else {
       status = main_temp_status;
       Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    target->set_status(status.status);
                }
            target = target->next;
        }      
    }
}

void Air::senaryo(char *par)
{
    cJSON *rcv = cJSON_Parse(par);
    if (rcv!=NULL)
    {
        JSON_getfloat(rcv,"temp",&(status.temp));
        JSON_getbool(rcv,"stat",&(status.stat));
        set_status(status);
        cJSON_Delete(rcv);  
    } 
}

void Air::init(void)
{
    if (!genel.virtual_device)
    {
        run_stat.min_temp = (global & 0b00011111);
        if (run_stat.min_temp==0) run_stat.min_temp=18;
        run_stat.mod = ((global & 0b10000000) == 0b10000000);
        
        run_stat.max_temp=(timer & 0b01111111);
        if (run_stat.max_temp<=run_stat.min_temp) run_stat.max_temp = 30;
        run_stat.user = ((timer & 0b10000000) == 0b10000000);

        uint16_t jj = (run_stat.max_temp<<8) | (run_stat.min_temp);
        status.color = jj;

        ESP_LOGW(AIR_TAG,"TERMOSTAT BILGILERI");
        ESP_LOGW(AIR_TAG,"-------------------");
        ESP_LOGW(AIR_TAG,"     Mod    : %s",(run_stat.mod==1)?"SOGUTMA":"ISITMA"); //1 Sogutma
        ESP_LOGW(AIR_TAG,"     Min/Max: %d/%d",run_stat.min_temp,run_stat.max_temp);   
        ESP_LOGW(AIR_TAG,"     User   : %s",(run_stat.user==1)?"SABİT":"Değiştirebilir");   

        status_to_durum(); 
        temp_action(true);
    }
}

void Air::status_to_durum(void)
{
    if (run_stat.user==0)
    {
        durum.stat.st.isitma_sogutma = run_stat.mod;
        durum.stat.int_st = status.counter;
    } else {
        durum.stat.st.auto_manuel = 0;
        durum.stat.st.isitma_sogutma = run_stat.mod;
        durum.stat.st.start_stop = 1;
    }
    durum.temp = status.temp;
    durum.set_temp = status.set_temp;
}

void Air::durum_to_status(void)
{
    status.counter = durum.stat.int_st;
    status.temp = durum.temp;
    status.set_temp = durum.set_temp;    
}

/*
      Air objesi termostat olarak çalışır. 
      Isıtma ve sogutma olmak üzere 2 ayrı algoritma ile kendisine baglanan çıkış rölesini kontrol eder. 
      
      Obje ister sogutma isterse ısıtma olarak tanımlanmış olsun, Auto ve Manuel olmak üzere 2 ayrı modda sahiptir.
      Auto modda sensorden gelen ısı degeri ve set degeri dikkate alınarak çıkış rölesi açılıp kapatılırken 
      manuel modda kullanıcıdan gelen aç/kapat emirlerine göre açılıp kapatılır. 

      Objeye en az 1 çıkış rölesi tanımlanmak zorundadır. Olası tanım:

      {
            "name":"air",
            "uname":"Isıtma",
            "id": 1,
            "loc": 0,     
            "timer": 0,
            "global":0,
            "hardware": {
                "port": [
                    {"pin": 0, "pcf":0, "name":"SN04", "type":"PORT_VIRTUAL"},
                    {"pin": 4, "pcf":1, "name":"KLM role", "type":"PORT_OUTPORT"}
                ]
            }
      }

      Global degerinin ilk 5 biti MINIMUM ısıtma degerini belitler.
                           8. bit 0 ise ISITMA sisteminin 1 ise SOGUTMA sisteminin,
                           
      minimum ısıtma veya sogutma degeri 0 verilirse 18 olur.

      timerın ilk 7 biti maximum ısıtma degerini tanımlar. 0 veya minimum degerden düşük verilirse 30 olarak alınır. 
      timer son biti 1 ise sistemlerin user tarafından degiştirilememesini sağlar.
     
      ölçülen ısı minimum/maximum degerler dışına çıkarsa sistem durdurulur.
      
      Objeye Virtual bir sensor tanımlanmak zorundadır. Bu sensor mevcut ısı, set ve manuel bilgilerini 
      gönderecektir. 

      Air objesi oda bilgilerini dikkate almaz. 

*/