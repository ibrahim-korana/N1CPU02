

/*
   Fonksiyonlarda bir aksiyon oluştuğunda fonksiyon bu prosedürü çagırır.
   Bu prosedür fonksiyonun durumunu cpu1 e gönderir.
*/

static void function_Callback(void *arg, home_status_t stat)
{
   Base_Function *aa = (Base_Function *) arg; 
   
   /*
   printf("CPU1 Send ACTION >>  Name %s Reg_ID=%d Id=%d stat= %d status=%d active=%d virtual=%d \n",
       aa->genel.name,
       aa->genel.register_id, 
       aa->genel.device_id, 
       stat.stat, 
       stat.status,
       stat.active,
       aa->genel.virtual_device);
       */
       
  //statusu cpu1 e gönder 
  if (strcmp(aa->genel.name,"air")==0)
    if (((Air *)aa)->send_gateway) {
      ((Air *)aa)->send_gateway = false;
      Termostat *tmp = get_termostat_head_handle();
      while (tmp)
        {
          Base_Port *target = aa->port_head_handle;
          while (target) 
          {
            if (target->type == PORT_VIRTUAL) 
                {
                   if (strcmp(target->name,tmp->get_name())==0)
                   {
                      tmp->set_set_temp(stat.set_temp);
                   } 
                }
            target = target->next;
          } 
          tmp = tmp->next;
        }
    }
  cJSON *root = cJSON_CreateObject();
  cJSON *child = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "com", "event");
  cJSON_AddNumberToObject(root, "id", aa->genel.device_id);
    aa->get_status_json(child);
    cJSON_AddItemToObject(root, "durum", child);   
  char *dat = cJSON_PrintUnformatted(root);

  while(uart.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
  return_type_t pp = uart.Sender(dat,253);
  if (pp!=RET_OK) printf("PAKET GÖNDERİLEMEDİ. Error:%d\n",pp);
  vTaskDelay(50/portTICK_PERIOD_MS); 

  cJSON_free(dat);
  cJSON_Delete(root); 
}

static void Register_Callback(void *arg)
{}

/*
    Virtual bir fonksiyon için dışarıdan gelen mesajlar
    işlendikten sonra fonksiyon nerede ise oraya gönderilmek
    üzere command_Callback'e gelir.  
*/
static void command_Callback(void *arg, home_status_t stat)
{
    
    Base_Function *aa = (Base_Function *) arg;  
    //printf("command callback\n");
    cJSON *root = cJSON_CreateObject();
    cJSON *child = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "S_SET");
    cJSON_AddNumberToObject(root, "dev_id", aa->genel.device_id);
      aa->ConvertStatus(stat,child);
      cJSON_AddItemToObject(root, "durum", child);   
    char *dat = cJSON_PrintUnformatted(root);
    //status oluştu fonksiyon hangi cihazda ise onu bul
    device_register_t *ch = cihazlar.cihazbul(aa->genel.register_device_id); 
    if (ch!=NULL)
      {
        Global_Send(dat,aa->genel.register_device_id,ch->transmisyon);
      }
    vTaskDelay(50/portTICK_PERIOD_MS);
    cJSON_free(dat);
    cJSON_Delete(root);
    
};