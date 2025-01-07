

#include "air.h"

typedef struct {
  uint8_t sender;
  char *data;
  transmisyon_t trn;
} regok_par_t;

void reg_ok_task(void *arg)
{
  regok_par_t *par = (regok_par_t *)arg;
  char *mm=(char*)calloc(1,strlen(par->data)+1);
  strcpy(mm,par->data);
  Global_Send(mm,par->sender,par->trn);
  free(mm);
  vTaskDelete(NULL);
}

void Gonder(void *arg)
{
  char *dat = (char *)arg;
  char *data;
  if (asprintf(&data,"%s",dat)>-1)
  {
   // printf("Giden %s\n",data);

    while(uart.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
    return_type_t pp = uart.Sender(data,253);
    if (pp!=RET_OK) printf("PAKET GÖNDERİLEMEDİ. Error:%d\n",pp);
    vTaskDelay(50/portTICK_PERIOD_MS);
    udp_server.send_broadcast((uint8_t *)data,strlen(data)); 
    free(data);
  } 
  vTaskDelete(NULL);
}


void coap_callback(char *data, uint8_t sender, transmisyon_t transmisyon, void *response, bool responsevar)
{
    /*
         RS485 transmisyonundan veya COAP (Udp) transmisyonundan gelen herşey
         bu callback'e gelir. Gelen tüm mesajlar alt cihazlardaki fonksiyonların
         aksiyonlarıdır. Bu nedenle burada değerlendirilerek işlem gerektirenler
         gerçekleştirilir veya fonksiyonların remote_set_status fonksiyonuna 
         gönderilirler.
    */

    if (sender<60)
      ESP_LOGI(TAG,"GELEN << sender: [%d] %s",sender, data);
    
    cJSON *rcv = cJSON_Parse(data);

    if (rcv==NULL) return; 
    char *command = (char *)calloc(1,20);
    if (JSON_item_control(rcv,"com")) JSON_getstring(rcv,"com", command,19); 
    if (JSON_item_control(rcv,"command")) JSON_getstring(rcv,"command", command,19); 


  if (strcmp(command,"sinfo_ack")==0) { 
        char *mac =(char*)malloc(16);
        char *ip =(char*)malloc(16);
        uint8_t fcount = 0, iid=0;
        JSON_getstring(rcv,"mac", mac,15); 
        JSON_getstring(rcv,"ip", ip,15); 
        JSON_getint(rcv,"fcount",&fcount);
        JSON_getint(rcv,"id",&iid);
        device_register_t *Bag=cihazlar.cihazbul(mac);
        if (Bag!=NULL) {
                          
                          Bag->device_id = iid; 
                          Bag->ip = Addr.to_int(ip);
                          Bag->function_count = fcount;
                          Bag->transmisyon = transmisyon;
                       } else {
                          Bag=cihazlar.cihazbul(iid); 
                          if (Bag==NULL) {
                            Bag = cihazlar.cihaz_ekle(iid,transmisyon);
                            Bag->device_id = iid; 
                            Bag->ip = Addr.to_int(ip);
                            Bag->function_count = fcount;
                          }
                          
                       }
        free(mac);
        free(ip);
      }


    /*
       Register sırasında bekleyen semaphoru serbest bırakır.
    */
    if (strcmp(command,"R_REG_OK")==0) {
                  xSemaphoreGive(REGOK_ready);
                                        }

    /*
       Ip Request (Device Register)
       I_REQ komutunun karşılığıdır. I_REQ broadcast olarak gider. Bunun karşılıgında
       hattaki tüm cihazlar id ve ip lerini I_ACK ile bildirirler. I_ACK cevaplarının
       toplamı cihaz tablosunu oluşturur. Bu tablo, device_register_t struct yapısında,
       diskte device.bin dosyasında ve hafızada sıralı olarak tutulur.   
    */
    if (strcmp(command,"I_ACK")==0)
      {
          uint8_t iid = 0, fcount=0 ;
          char *mac = (char *)malloc(20);
          char *ip = (char *)malloc(20);
          JSON_getint(rcv,"id",&iid);
          JSON_getstring(rcv,"mac",mac,19);
          JSON_getstring(rcv,"ip",ip,19);
          JSON_getint(rcv,"fcount",&fcount);
          device_register_t *Bag = cihazlar.cihazbul(mac);
          if (Bag!=NULL) {
                          Bag->device_id = iid; 
                          Bag->ip = Addr.to_int(ip);
                          Bag->function_count = fcount;
                       } else {
                          Bag=cihazlar.cihazbul(iid); 
                          if (Bag==NULL) {
                            Bag = cihazlar.cihaz_ekle(iid,transmisyon);
                            Bag->device_id = iid; 
                            Bag->ip = Addr.to_int(ip);
                            Bag->function_count = fcount;
                          } else {
                            Bag->ip = Addr.to_int(ip);
                            Bag->function_count = fcount;
                          }
                       }
          free(mac);mac=NULL;
          free(ip);ip=NULL;
          if (IACK_ready!=NULL) xSemaphoreGive(IACK_ready);

      }
          
  
    /*
          Register Request
          R_REG komutunun karşılığıdır. R_REG komutunu alan cihaz üzerinde tanımlı fonksiyonları
          register etmek için R_REQ komutunu gönderir. R_REQ alan server fonksiyonu virtual
          olarak tanımlayarak R_ACK ile tanım bilgilerini gönderecektir. R_REQ komutu
          R_ACK alınıncaya kadar aralıklarla tekrar tekrar gönderilir. 
    */
    if (strcmp(command,"R_REQ")==0)
      {
        uint8_t id000 = 0;
        uint8_t loc = 0;
        bool err=false;
        char *nm = (char*)malloc(30);
        char *aunm = (char*)malloc(30);
        if (!JSON_getint(rcv,"req_id",&id000)) err=true;
        if (!JSON_getstring(rcv,"name",nm,29)) err=true;
        if (!JSON_getstring(rcv,"auname",aunm,29)) strcpy(aunm,"No_AuName");
        JSON_getint(rcv,"loc",&loc);

        if (!err)
        {

          uint8_t ww = functions_remote_register(
                                  nm,aunm,
                                  sender,id000,
                                  loc, //prg_location olacak
                                  transmisyon,
                                  &function_Callback,
                                  &command_Callback,
                                  disk
                              );
          
          ESP_LOGI(TAG,"Sender : [%d] %d => %d Registered",sender, id000,ww);
          cJSON *root = cJSON_CreateObject();
          cJSON_AddStringToObject(root, "com", "R_ACK");
          cJSON_AddNumberToObject(root, "req_id",id000 );
          cJSON_AddNumberToObject(root, "ack_id",ww );
          char *dat = cJSON_PrintUnformatted(root);

          regok_par_t rr = {};
          rr.data = dat;
          rr.sender = sender;
          rr.trn = transmisyon;
          //strcpy((char *)response,dat);
          //Global_Send(dat,sender,transmisyon);
          xTaskCreate(reg_ok_task,"rtask",2048,&rr,5,NULL);
          vTaskDelay(100/portTICK_PERIOD_MS);
          cJSON_free(dat);
          cJSON_Delete(root);

          if (JSON_item_control(rcv,"durum")) 
          {
              home_status_t stt = {};
              Base_Function *bf = function_find(ww);
              json_to_status(rcv,&stt,bf->get_status());              
              if (bf!=NULL) bf->remote_set_status(stt);
          }                
        }
        free(nm);
        free(aunm);
      }


    /*
        Event Request
        Bağlı cihazlarda herhangi bir aksiyon olduğunda cihaz aksiyonu bildirmek için
        E_REQ komutunu gönderir. Server, komut ile gönderilen durumu virtual fonksiyona
        bildirerek CPU 1 e gönderilmek üzere aksiyon oluşturmasını sağlar.
    */
    if (strcmp(command,"E_REQ")==0)
    {
      uint8_t id0 = 0;
      JSON_getint(rcv,"id",&id0);
      if (JSON_item_control(rcv,"durum")) 
      {
        home_status_t stt = {};
        if (id0>0)
        {
          Base_Function *bf = function_find(id0);        
          if (bf!=NULL) {
            json_to_status(rcv,&stt,bf->get_status()); 
            bf->remote_set_status(stt);
          }
        } else {
           //eger id 0 ise statusu tum sensorlere gönder.
           Base_Function *bf = get_function_head_handle();
           while (bf)
              {
                json_to_status(rcv,&stt,bf->get_status()); 
                //ESP_LOGI(TAG,"send %s %d ",bf->genel.name, bf->genel.device_id);                
                bf->set_sensor((char*)stt.irval,stt);
                bf = bf->next;
              }
        }
      }
    }   

  if (strcmp(command,"V_STAT")==0)
    {
      uint8_t id0 = 0;
      char *nm = (char*)malloc(30);
      JSON_getint(rcv,"id",&id0);
      JSON_getstring(rcv,"name",nm,29);
      if (id0==0)
      {
        Base_Function *bf = get_function_head_handle();
        while (bf)
        {
          if (bf->find_port(nm))
          {
            //printf("V_STAT %s %d %s\n",nm,bf->genel.device_id,bf->genel.name);
            if(strcmp(bf->genel.name,"lamp")==0)
            {
              cJSON *root1 = cJSON_CreateObject();
              cJSON_AddStringToObject(root1, "com", "stat");
              cJSON_AddNumberToObject(root1, "dev_id", bf->genel.device_id);
              cJSON_AddStringToObject(root1, "name", nm);
              cJSON_AddNumberToObject(root1,"stat",bf->get_status().stat);
              char *dat1 = cJSON_PrintUnformatted(root1);
              xTaskCreate(Gonder,"gondertask",2048,dat1,5,NULL);
              vTaskDelay(100/portTICK_PERIOD_MS);
              free(dat1);
              free(root1);
              break;
            }
            if(strcmp(bf->genel.name,"air")==0)
            {
              cJSON *root1 = cJSON_CreateObject();
              cJSON_AddStringToObject(root1, "com", "stat");
              cJSON_AddNumberToObject(root1, "dev_id", bf->genel.device_id);
              cJSON_AddStringToObject(root1, "name", nm);
              Air *bbb = (Air *)bf;
              bbb->durum_to_status();
              cJSON_AddNumberToObject(root1,"temp",bbb->get_status().temp);
              cJSON_AddNumberToObject(root1,"stemp",bbb->get_status().set_temp);
              cJSON_AddNumberToObject(root1,"coun",bbb->get_status().counter);

              char *dat1 = cJSON_PrintUnformatted(root1);
              xTaskCreate(Gonder,"gondertask",2048,dat1,5,NULL);
              vTaskDelay(100/portTICK_PERIOD_MS);
              free(dat1);
              free(root1);
              break;
            }
          }
          bf = bf->next;
        }
      }
      free(nm);
    }  


    if (strcmp(command,"S_ACK")==0)
    {
      uint8_t id0 = 0;
      JSON_getint(rcv,"id",&id0);
      if (JSON_item_control(rcv,"durum")) 
      {
        home_status_t stt = {};
        Base_Function *bf = function_find(id0);       
        if (bf!=NULL) {
          json_to_status(rcv,&stt,bf->get_status()); 
          bf->remote_set_status(stt);
        }
      }
      if (status_ready!=NULL) xSemaphoreGive(status_ready);
      if (STATUS_ready!=NULL) xSemaphoreGive(STATUS_ready);
    }   

    free(command);
    cJSON_Delete(rcv);    
}  
