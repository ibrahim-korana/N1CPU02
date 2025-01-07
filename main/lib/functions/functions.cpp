#include "functions.h"
#include "esp_log.h"
#include "ArduinoJson.h"
#include "classes.h"
#include "lamp.h"
#include "priz.h"
#include "kontaktor.h"
#include "onoff.h"
#include "curtain.h"
#include "air.h"
#include "security.h"
#include "water.h"
#include "mwater.h"
#include "gas.h"
#include "tmrole.h"
#include "termostat.h"
#include "asansor.h"
#include "piston.h"
#include "kapi.h"
#include "garage.h"

#include "../tool/tool.h"

const char *FUNTAG = "FUNCTIONS";

Base_Function *function_head_handle = NULL;
Termostat *termostat_head_handle = NULL;
prg_location_t *locations = NULL;
uint8_t Function_Counter = 0;

prg_location_t *get_location_head_handle(void)
{
  return locations;
} 

Termostat *get_termostat_head_handle(void)
{
  return termostat_head_handle;
} 

Base_Function *get_function_head_handle(void)
{
  return function_head_handle;
}

void set_function_head_handle(Base_Function * bas) 
{
   function_head_handle=bas;
}

#define PORT_DEBUG true

void add_port(JsonArray port, Base_Function *cls, i2c_dev_t **pcf, bool debug=false)
{

   for (JsonObject prt : port) {
      int _p_port = prt["pin"]; // 1, 1
      const char* _p_name = prt["name"]; // "anahtar", "role"
      const char* _p_type = prt["type"]; // "PORT_INPORT", ...
      int _p_rev = prt["reverse"];
      int _p_pcf = prt["pcf"];

      port_type_t tt = port_type_convert((char *)_p_type); 
      Base_Port *aa = new Base_Port();
      assert(aa!=NULL);
      strcpy(aa->name,_p_name);
      aa->Klemens = _p_port;
      if (tt==PORT_OUTPORT) {
                    /*pcf üzerinden out lar*/
                    if (_p_pcf>0) {
                        uint8_t pcfno=0,pcfpin=0;
                        if (out_pin_convert(_p_port,&pcfno,&pcfpin))
                          {  
                              out_config_t *cfg = (out_config_t *)malloc(sizeof(out_config_t));

                             // printf("OUT PORT CFG %d %d %s\n",pcfno, pcfpin, _p_name);

                              cfg->type = OUT_TYPE_EXPANDER;
                              uint8_t al=0;
                              if (_p_rev==1) {al=1;}
                              cfg->pcf8574_out_config = {
                                        .pin_num = pcfpin,
                                        .device = pcf[pcfno],
                                        .reverse = al
                                    };
                              out_handle_t cikis = iot_out_create(cfg); 
                              aa->set_outport(cikis);  
                          }
                  } else {
                    /*gpio üzerinden yapılan outlar*/
                    out_config_t *cfg = (out_config_t *)malloc(sizeof(out_config_t));
                    cfg->type = OUT_TYPE_GPIO;
                    cfg->gpio_out_config = {
                        .gpio_num = _p_port,
                        }; 
                    out_handle_t cikis = iot_out_create(cfg); 
                    aa->set_outport(cikis);  
                  }
                  aa->set_port_type(tt,(void*)cls);  
                  cls->add_port(aa,PORT_DEBUG);
              }

      if (tt!=PORT_OUTPORT) {
        if (_p_pcf>0) {
          //port pcf üzerinde
          uint8_t pcfno=0,pcfpin=0;
          if (in_pin_convert(_p_port,&pcfno,&pcfpin))
            {
              //pin numarası doğru
              button_config_t *cfg = (button_config_t *)malloc(sizeof(button_config_t));
              cfg->type = BUTTON_TYPE_EXPANDER;
              cfg->pcf8574_button_config = {
                                .pin_num = pcfpin,
                                .device = pcf[pcfno],
                                .active_level = 0,};
              button_handle_t giris = iot_button_create(cfg);
              aa->set_inport(giris); 
              aa->set_port_type(tt,(void*)cls);  
              cls->add_port(aa,PORT_DEBUG); 
            }
                      } else {
                        if (tt!=PORT_VIRTUAL)
                        {
                        //port gpio üzerinde
                        uint8_t lev = 0;
                        if (_p_rev==1) lev=1;
                        button_config_t *cfg = (button_config_t *)malloc(sizeof(button_config_t));
                        cfg->type = BUTTON_TYPE_GPIO;
                        cfg->gpio_button_config = {
                                  .gpio_num = _p_port,
                                  .active_level = lev,
                              };
                        button_handle_t giris = iot_button_create(cfg);
                        aa->set_inport(giris); 
                        aa->set_port_type(tt,(void*)cls);  
                        cls->add_port(aa,PORT_DEBUG);   
                        } else {
                          //port Virtualdır
                          aa->virtual_port = true;
                          aa->type = tt; 
                          cls->add_port(aa,PORT_DEBUG); 
                        }
                      }

      }        
  } //for
}

void *add_function(uint8_t a_id, const char *a_name, const char *au_name, uint8_t loc, function_callback_t fun_cb, Storage dsk)
{
    void *bb0 = NULL;
    //---------------------  
      if (strcmp(a_name,"lamp")==0) bb0 = (Lamp *)new Lamp(a_id, fun_cb, dsk);
      if (strcmp(a_name,"priz")==0) bb0 = (Priz *) new Priz(a_id, fun_cb, dsk);
      if (strcmp(a_name,"cont")==0) bb0 = (Contactor *) new Contactor(a_id, fun_cb, dsk);    
      if (strcmp(a_name,"onoff")==0) bb0 = (Onoff *) new Onoff(a_id, fun_cb, dsk); 
      if (strcmp(a_name,"cur")==0) bb0 = (Curtain *) new Curtain(a_id,fun_cb, dsk);  
      if (strcmp(a_name,"air")==0) bb0 = (Air *) new Air(a_id,fun_cb, dsk); 
      if (strcmp(a_name,"sec")==0) bb0 = (Security *) new Security(a_id,fun_cb, dsk);
      if (strcmp(a_name,"water")==0) bb0 = (Water *) new Water(a_id,fun_cb, dsk);
      if (strcmp(a_name,"mwater")==0) bb0 = (MWater *) new MWater(a_id,fun_cb, dsk);
      if (strcmp(a_name,"gas")==0) bb0 = (Gas *) new Gas(a_id,fun_cb, dsk);
      if (strcmp(a_name,"tmrelay")==0) bb0 = (TmRelay *) new TmRelay(a_id,fun_cb, dsk);
      if (strcmp(a_name,"elev")==0) bb0 = (Asansor *) new Asansor(a_id,fun_cb, dsk);
      if (strcmp(a_name,"pis")==0) bb0 = (Piston *) new Piston(a_id,fun_cb, dsk);
      if (strcmp(a_name,"door")==0) bb0 = (Door *) new Door(a_id,fun_cb, dsk);
      if (strcmp(a_name,"gara")==0) bb0 = (Garage *) new Garage(a_id,fun_cb, dsk);
    //---------------------
    if (bb0!=NULL)    
      {
        ((Base_Function *)bb0)->genel.device_id = ++Function_Counter;
        strcpy(((Base_Function *)bb0)->genel.uname, au_name);
        //------ fonksiyonu listeye ekle -------------
        ((Base_Function *)bb0)->next = function_head_handle;
        ((Base_Function *)bb0)->prg_loc = loc;

        function_head_handle = (Base_Function *)bb0;
      }
    return bb0;  
}


Base_Function *find_register(uint8_t sender, uint8_t rid)
{
    Base_Function *target = function_head_handle;
    while(target)
      {
        Base_Function *cc = (Base_Function *)target;
        if (cc->genel.register_device_id == sender && cc->genel.register_id==rid) return cc;
        target=cc->next;
      }
    return NULL;
}

Base_Function *function_find(uint8_t id)
{
    Base_Function *target = function_head_handle;
    while(target)
      {
        Base_Function *cc = (Base_Function *)target;
        if (cc->genel.device_id == id) return cc;
        target=cc->next;
      }
    return NULL;
}

uint8_t function_remote_re_register(
                                     function_reg_t *old,
                                     function_callback_t fun_cb,
                                     function_callback_t com_cb,
                                     Storage dsk
                                   )
{
  Base_Function *qq = find_register(old->register_device_id,old->register_id);
  if (qq!=NULL) {
                  //printf("OLD LOC Bu zaten var %d\n",old->prg_loc);
                  qq->prg_loc = old->prg_loc;
                  return qq->genel.device_id;
                }

  
  void *bb0 = add_function(old->device_id, old->name, old->auname, old->prg_loc, fun_cb, dsk);             
  if (bb0!=NULL)
  {
    ((Base_Function *)bb0)->register_callback = NULL;
    ((Base_Function *)bb0)->command_callback = com_cb;
    ((Base_Function *)bb0)->genel.register_id = old->register_id;
    ((Base_Function *)bb0)->genel.register_device_id = old->register_device_id;
    ((Base_Function *)bb0)->genel.virtual_device = true;
    ((Base_Function *)bb0)->genel.location = old->transmisyon;
    ((Base_Function *)bb0)->prg_loc = old->prg_loc;
    //((Base_Function *)bb0)->genel.active = old->active;
    //printf("OLD LOC Bu yeni ekleniyor %d\n",((Base_Function *)bb0)->prg_loc);
    return ((Base_Function *)bb0)->genel.device_id;
  }   
       
  return 255; 
}

uint8_t functions_remote_register(const char *name, 
                                  const char *au_name,
                                  uint8_t sender, 
                                  uint8_t rid, 
                                  uint8_t prg_lc,
                                  transmisyon_t yer,
                                  function_callback_t fun_cb,
                                  function_callback_t com_cb,
                                  Storage dsk
                                  ) 
{
  Base_Function *qq = find_register(sender,rid);
  if (qq!=NULL) {
                    qq->prg_loc = prg_lc;
                    function_reg_t reg = {};
                    strcpy(reg.name,name);
                    strcpy(reg.auname,au_name);
                    reg.device_id = qq->genel.device_id;
                    reg.register_id = rid;
                    reg.register_device_id = sender;
                    reg.transmisyon = yer;
                    reg.prg_loc = prg_lc;
                    dsk.write_file("/config/function.bin",&reg,sizeof(function_reg_t),reg.device_id);
                   return qq->genel.device_id;
                }
  void *bb0 = add_function(0, name, au_name,prg_lc, fun_cb, dsk);
  if (bb0!=NULL)    
    {
      // printf("Once %d ile \n",((Base_Function *)bb0)->genel.device_id);

      //printf("PRGLOC %d\n",prg_lc);

      ((Base_Function *)bb0)->register_callback = NULL;
      ((Base_Function *)bb0)->command_callback = com_cb;
      ((Base_Function *)bb0)->genel.location = yer; 
      ((Base_Function *)bb0)->timer = 0;
      ((Base_Function *)bb0)->global = 0;
      ((Base_Function *)bb0)->genel.virtual_device = true;
      ((Base_Function *)bb0)->genel.register_id = rid;
      ((Base_Function *)bb0)->genel.register_device_id = sender;
      ((Base_Function *)bb0)->prg_loc = prg_lc;
       
      function_reg_t reg = {};
      strcpy(reg.name,name);
      strcpy(reg.auname,au_name);
      reg.device_id = ((Base_Function *)bb0)->genel.device_id;
      reg.register_id = rid;
      reg.register_device_id = sender;
      reg.transmisyon = yer;
      reg.prg_loc = prg_lc;

      //printf("%d ile kaydedildi\n",reg.device_id);
      ((Base_Function *)bb0)->genel.device_id = reg.device_id;
     
      dsk.write_file("/config/function.bin",&reg,sizeof(function_reg_t),reg.device_id);

      //function_reg_t reg0 = {};
      //dsk.read_file("/config/function.bin",&reg0,sizeof(function_reg_t),reg.device_id);
      //printf("KAYITTAN OKUNAN %d %s\n",reg0.device_id,reg0.auname);
       
      return ((Base_Function *)bb0)->genel.device_id;
    }
    return 255;
}

void *Read_functions(function_callback_t fun_cb, 
                     register_callback_t reg_cb,
                     Storage dsk,
                     i2c_dev_t **pcf
                     )
{
    //printf("BEFORE Free heap: %u\n", esp_get_free_heap_size());
    const char *name1="/config/config.json";
    if (dsk.file_search(name1))
      {
        int fsize = dsk.file_size(name1); 
        /*
        if (fsize>2048) {
          ESP_LOGE(FUNTAG, "config.json 2048 byte dan büyük");
          ESP_ERROR_CHECK(true);
        }
        */
       ESP_LOGI("TAG"," File SIZE    : %u" , fsize);
        char *buf = (char *) malloc(fsize+5);
        if (buf==NULL) {ESP_LOGE(FUNTAG, "memory not allogate"); return NULL;}
        FILE *fd = fopen(name1, "r");
        if (fd == NULL) {ESP_LOGE(FUNTAG, "%s not open",name1); return NULL;}
        fread(buf, fsize, 1, fd);
        fclose(fd);
        
        DynamicJsonDocument doc(fsize+5);
        DeserializationError error = deserializeJson(doc, buf);
       

        if (error) {
          ESP_LOGE(FUNTAG,"deserializeJson() failed: %s",error.c_str());
          return NULL;
        }

        for (JsonObject function : doc["functions"].as<JsonArray>()) {
          const char* a_name = function["name"];
          const char* au_name = function["uname"];
          int a_id = function["id"]; 
          int a_loc = function["loc"]; 
          int a_tim = function["timer"]; 
          int a_glo = function["global"];
          //const char* function_hardware_location = function["hardware"]["location"]; 

          void *bb0 = add_function(a_id, a_name, au_name, a_loc, fun_cb, dsk);
          if (bb0!=NULL)    
              {
                ((Base_Function *)bb0)->register_callback = reg_cb;
                ((Base_Function *)bb0)->timer = a_tim;
                ((Base_Function *)bb0)->global = a_glo;
                ((Base_Function *)bb0)->genel.location = TR_LOCAL;
                add_port(function["hardware"]["port"].as<JsonArray>(),((Base_Function *)bb0),pcf);
               
                //if (strcmp(a_name,"air")==0) ((Base_Function *)bb0)->list_port();
              }  
        }
      
        doc.clear();                       
        free(buf);
        uint8_t kk=0; 
        Base_Function *target = function_head_handle;
        while(target)
          {
            ((Base_Function *)target)->init();
            ((Base_Function *)target)->set_active(true);
            kk++;
            target=((Base_Function *)target)->next;
          }
        ESP_LOGI(FUNTAG,"config.json dosyasından %d fonksiyon eklendi ve başlatıldı.",kk);           
      }
      
    //printf("AFTER Free heap: %u\n", esp_get_free_heap_size());
    return NULL;   
}

uint8_t function_count(void)
{
  uint8_t cnt=0;
  Base_Function *target = function_head_handle;
  while(target)
      {
          cnt++;
          target=target->next;
      }
  return cnt;
}

void function_list(void)
{
    Base_Function *target = function_head_handle;
    uint8_t cnt =0;
    ESP_LOGI(FUNTAG,"     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    ESP_LOGI(FUNTAG,"     DEV REG CIH NAME           STR            ACT LOC");
    ESP_LOGI(FUNTAG,"     ID  ID  ID  ");
    ESP_LOGI(FUNTAG,"     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"); 
    while(target)
      {
        Base_Function *cc = (Base_Function *)target;
        home_status_t st = cc->get_status();
        ESP_LOGI(FUNTAG,"     %3d %3d %3d %-15s %-15s %-3d %-3d", 
              cc->genel.device_id,
              cc->genel.register_id,
              cc->genel.register_device_id,
              cc->genel.name, 
              cc->genel.uname, 
              st.active,
              cc->prg_loc
              );
          cc->list_port();    
          target=cc->next;
          cnt ++;
      }
    ESP_LOGI(FUNTAG,"     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"); 
    ESP_LOGI(FUNTAG,"     Toplam Fonksiyon Sayisi : %d",cnt); 
}

void add_locations(prg_location_t *lc )
{
  lc->next = (prg_location_t *)locations;
  locations = lc;
}

void list_locations(void)
{
  prg_location_t *target = locations;
    ESP_LOGI(FUNTAG,"     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    ESP_LOGI(FUNTAG,"     LOCATIONS");
    ESP_LOGI(FUNTAG,"     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"); 
    while(target)
      {
        ESP_LOGI(FUNTAG,"     %3d %-20s", 
              target->page,
              target->name
              );
          target=(prg_location_t *)target->next;
      }
    ESP_LOGI(FUNTAG,"     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");  
}

void *read_locations(Storage dsk)
{
    const char *name1="/config/location.json";
    if (dsk.file_search(name1))
      {
        int fsize = dsk.file_size(name1); 
        char *buf = (char *) malloc(fsize+5);
        if (buf==NULL) {ESP_LOGE(FUNTAG, "memory not allogate"); return NULL;}
        FILE *fd = fopen(name1, "r");
        if (fd == NULL) {ESP_LOGE(FUNTAG, "%s not open",name1); return NULL;}
        fread(buf, fsize, 1, fd);
        fclose(fd);
        DynamicJsonDocument doc(3000);
        DeserializationError error = deserializeJson(doc, buf);

        if (error) {
          ESP_LOGE(FUNTAG,"deserializeJson() failed: %s",error.c_str());
          return NULL;
        }

        for (JsonObject function : doc["locations"].as<JsonArray>()) {
          const char* a_name = function["name"];
          int a_page = function["page"]; 
         
          prg_location_t *bb0 = new prg_location();
          strcpy(bb0->name,a_name);
          bb0->page = a_page;
          add_locations(bb0);
        }
      
        doc.clear();                       
        free(buf);
      }
   return NULL;   
}


void *read_gateway(Storage dsk, rs485_callback_t cb, RS485 *rs)
{
    const char *name1="/config/gateway.json";
    if (dsk.file_search(name1))
      {
        int fsize = dsk.file_size(name1); 
        char *buf = (char *) malloc(fsize+5);
        if (buf==NULL) {ESP_LOGE(FUNTAG, "memory not allogate"); return NULL;}
        FILE *fd = fopen(name1, "r");
        if (fd == NULL) {ESP_LOGE(FUNTAG, "%s not open",name1); return NULL;}
        fread(buf, fsize, 1, fd);
        fclose(fd);
        DynamicJsonDocument doc(3000);
        DeserializationError error = deserializeJson(doc, buf);

        if (error) {
          ESP_LOGE(FUNTAG,"deserializeJson() failed: %s",error.c_str());
          return NULL;
        }

        for (JsonObject function : doc["gateways"].as<JsonArray>()) {
          const char* a_name = function["name"];
          uint8_t a_id = function["id"]; 
          const char* a_trns = function["trns"];
          transmisyon_t tt = TR_PJON;
          if (strcmp(a_trns,"PJON")==0) tt = TR_PJON;
          if (strcmp(a_trns,"RS485")==0) tt = TR_SERIAL;

          Termostat *term = new Termostat((char*)a_name,a_id, cb, tt, rs);
          term->init(); //Bu satır Termostatı başlatır
          term->next = (Termostat *)termostat_head_handle;
          termostat_head_handle = term;
         
          ESP_LOGI(FUNTAG,"%s Termostat Eklendi.",a_name);
        }
      
        doc.clear();                       
        free(buf);
      }
   return NULL;   
}

Base_Function *function_find_name(const char *name)
{
    Base_Function *target = function_head_handle;
    while(target)
      {
        Base_Function *cc = (Base_Function *)target;
        if (strcmp(cc->genel.name,name) == 0) return cc;
        target=cc->next;
      }
    return NULL;
}

void add_daire_security(uint8_t daire, uint8_t blok)
{
    Security *sc = (Security *)function_find_name("sec");
    if (sc!=NULL) {
      sc->set_daire(daire,blok);
    }

}