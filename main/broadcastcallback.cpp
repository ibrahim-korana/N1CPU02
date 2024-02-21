

void broadcast_callback(char *data, uint8_t sender, transmisyon_t transmisyon)
{
    //char *data = (char *) arg;
    char *mm= (char*)malloc(20);
    transmisyon_string(transmisyon,mm);
    ESP_LOGI(TAG,"%s broadcast <<  %s", mm,data);
    free(mm);

  cJSON *rcv = cJSON_Parse((char*)data);
  if (rcv==NULL) return; 
  char *command = (char *)malloc(20);
  JSON_getstring(rcv,"com", command, 19); 
  
  
  if (strcmp(command,"sinfo")==0)
  {
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
                          Bag = cihazlar.cihaz_ekle(mac,transmisyon);
                          Bag->device_id = iid; 
                          Bag->ip = Addr.to_int(ip);
                          Bag->function_count = fcount;
                       }
        free(mac);
        free(ip);
        char *buf =(char*)malloc(200);
        info("sinfo_ack",buf);
        if (transmisyon==TR_SERIAL) {rs485.Sender(buf,Bag->device_id);}
        if (transmisyon==TR_ESPNOW) {EspNOW_Send(buf,Bag->device_id);}
        if (transmisyon==TR_UDP) {tcpserver.Send(buf,Bag->device_id);}
        free(buf);

  } 
  
  free(command); 
  cJSON_Delete(rcv);  
}
