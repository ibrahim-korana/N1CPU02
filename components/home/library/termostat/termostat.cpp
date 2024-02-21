#include "termostat.h"
#include "esp_log.h"
#include "jsontool.h"

static const char *TERMOSTAT_TAG = "TERMOSTAT";

esp_err_t Termostat::read_gateway(void)
{
    uint8_t *dt = (uint8_t *)malloc(10);
    uint8_t *cm = (uint8_t *)malloc(10);
    char *tmp = (char *)malloc(10);
    esp_err_t RET = ESP_OK;

    //printf("gateway okunuyor\n");

    //3451 komutu ile atmeganın pjon uzerinden termostat verilerini okumasını saglar.
    sprintf((char*)cm,"34%02d",id);

    //printf("Gateway %s\n",cm);

    if (gateway_write(&dev,cm,4)!=ESP_OK) RET = ESP_FAIL;

    if (RET==ESP_OK)
    {
        vTaskDelay(1000/portTICK_PERIOD_MS); 
        sprintf((char*)cm,"31%02d",id);
        memset(dt,0,10);
        memset(tmp,0,10);
        esp_err_t err = gateway_read(&dev,cm,4,(uint8_t *)dt,6);
        if (err!=ESP_OK) err = gateway_read(&dev,cm,4,(uint8_t *)dt,6);
        if (err!=ESP_OK) RET=ESP_FAIL;
        if (RET==ESP_OK)
        {
            memcpy(tmp,dt+4,2);
            if (atoi(tmp)>0)
            {
                temp = atoi(tmp);
                //printf("Okunan ISI %d\n",temp);
                vTaskDelay(50/portTICK_PERIOD_MS);
            }

            sprintf((char*)cm,"32%02d",id);
            memset(dt,0,10);
            memset(tmp,0,10);
            err= gateway_read(&dev,cm,4,(uint8_t *)dt,6); 
            if (err!=ESP_OK) err = gateway_read(&dev,cm,4,(uint8_t *)dt,6);
            if (err!=ESP_OK) RET=ESP_FAIL;
            if (RET==ESP_OK)
            {
                memcpy(tmp,dt+4,2);
                if (atoi(tmp)>0)
                {
                    set = atoi(tmp);
                    //printf("Okunan SET %d\n",set);
                }
            }
        }
        
    }
    free(tmp);
    free(cm);
    free(dt);
   // if (RET!=ESP_OK) printf("Okuma Hatalı\n");
    return RET;
}

esp_err_t Termostat::read_gateway_temp(void)
{
    uint8_t *dt = (uint8_t *)malloc(10);
    uint8_t *cm = (uint8_t *)malloc(10);
    char *tmp = (char *)malloc(10);

    //Gateway üzerindeki degerleri okur

    sprintf((char*)cm,"31%02d",id);
    memset(dt,0,10);
    memset(tmp,0,10);
    gateway_read(&dev,cm,4,(uint8_t *)dt,6);
    memcpy(tmp,dt+4,2);
    temp = atoi(tmp);
    //printf("Okunan ISI %d\n",temp);
    vTaskDelay(50/portTICK_PERIOD_MS);

    sprintf((char*)cm,"32%02d",id);
    memset(dt,0,10);
    memset(tmp,0,10);
    gateway_read(&dev,cm,4,(uint8_t *)dt,6); 
    memcpy(tmp,dt+4,2);
    set = atoi(tmp);

    //printf("Okunan SET %d\n",set);
    
    free(tmp);
    free(cm);
    free(dt);
    return ESP_OK;

}


void Termostat::set_set_temp(float s_temp)
{
   uint8_t t = (uint8_t) s_temp;
   ESP_LOGI(TERMOSTAT_TAG,"[%d] %d dereceye set ediliyor",id,t);
   uint8_t *cm = (uint8_t *)malloc(10);
   sprintf((char*)cm,"33%02d%02d",id,t);
   gateway_write(&dev,cm,6);
   free(cm);
   vTaskDelay(10000/portTICK_PERIOD_MS);
}

void Termostat::init(void)
{
    //isi ve seti iste
    read_gateway();

    esp_timer_create_args_t arg = {};
    arg.callback = &ter_tim_callback;
    //arg.name = "Ptim0";
    arg.arg = (void *) this;
    ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer));
    error = 0;

   // tim_callback((void *)this);
    //tim_start();
    //zamanlamayı başlat   
}

void Termostat::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Termostat::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_periodic(qtimer, 30000000));
}

void Termostat::ter_tim_callback(void* arg)
{   
    Termostat *ths = (Termostat *)arg;
    //ths->tim_stop();

    printf ("Timer Callback %d\n", ths->get_id());
    
    esp_err_t err = ths->read_gateway();

    uint8_t tmp = ths->get_temp();
    uint8_t set = ths->get_set();

    if (tmp==0 && set==0)
      {
          ths->inc_error();
          if (ths->get_error()<15) ESP_LOGE(TERMOSTAT_TAG,"ERROR %d %d", ths->get_id(),ths->get_error());
      } else ths->set_error(0);

    if (ths->get_error()<15)
    {

            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "com", "E_REQ");
            cJSON_AddNumberToObject(root, "dev_id", 0);
            cJSON *drm = cJSON_CreateObject();
            
            char *mm0 = (char*)malloc(10);
            sprintf(mm0, "%d.0", tmp);
            cJSON_AddRawToObject(drm, "temp", mm0);

            sprintf(mm0, "%d.0", set);
            cJSON_AddRawToObject(drm, "stemp", mm0);

            free(mm0);
            
            cJSON_AddStringToObject(drm, "irval", (char*)ths->get_name());   
            cJSON_AddItemToObject(root, "durum", drm); 
            char *dat = cJSON_PrintUnformatted(root);
            //printf("tim callback %s\n",dat);
            ths->callback(dat,ths->get_id(),TR_GATEWAY);
        
            cJSON_free(dat);
            cJSON_Delete(root);  
    }

   // ths->tim_start();
}

void Termostat::read_send(void)
{
    //tim_stop();
    //tim_start();
    read_gateway_temp();
    uint8_t tmp = get_temp();
    uint8_t set = get_set();

    if (tmp==0 && set==0)
      {
          inc_error();
          if (get_error()<15) ESP_LOGE(TERMOSTAT_TAG,"ERROR %d %d", get_id(),get_error());
          vTaskDelay(1000/portTICK_PERIOD_MS);  
      } else set_error(0);

    if (get_error()<15 && get_temp()>0)
    {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "com", "E_REQ");
            cJSON_AddNumberToObject(root, "dev_id", 0);
            cJSON *drm = cJSON_CreateObject();

            char *mm0 = (char*)malloc(10);
            sprintf(mm0, "%d.0", get_temp());
            cJSON_AddRawToObject(drm, "temp", mm0);

            sprintf(mm0, "%d.0", get_set());
            cJSON_AddRawToObject(drm, "stemp", mm0);

            free(mm0);
            
            cJSON_AddStringToObject(drm, "irval", (char*)get_name());   
            cJSON_AddItemToObject(root, "durum", drm); 
            char *dat = cJSON_PrintUnformatted(root);

            //printf("read_send %s\n",dat);

            callback(dat,get_id(),TR_GATEWAY);
        
            cJSON_free(dat);
            cJSON_Delete(root);
    }  
}