

void rs485_callback(char *data, uint8_t sender, transmisyon_t transmisyon)
{
  // printf("RS485 %s\n",data);
   coap_callback(data,sender,transmisyon,NULL, false);
}