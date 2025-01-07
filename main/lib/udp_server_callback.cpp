

void uart_sender_task(void *arg)
{
    //printf("task\n");
    char *txt = (char*)arg;
    //printf("task 1 %s\n",txt);
    char *txt0;
    asprintf(&txt0,"%s",txt);
    //printf("task 2\n");
    strcpy(txt0,txt);
    //printf("task 3 %s\n",txt0);
    while(uart.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
    uart.Sender(txt0,RS485_MASTER);
    //printf("task 4\n");
    vTaskDelay(100/portTICK_PERIOD_MS);
    //printf("task 5\n");
    free(txt0);
    //printf("task end\n");
    vTaskDelete(NULL);
}


static void on_udp_recv(void *handler_arg, esp_event_base_t base, int32_t id,
                    void *event_data)
{
    /* if (event_data == NULL) {
        return;
    }
    udp_msg_t *recv = (udp_msg_t *)event_data;
    char *txt = (char*)calloc(1,recv->len+1);
    memcpy(txt,recv->payload,recv->len);
    
    ESP_LOGI(TAG, "Received %d bytes from %s:%d", recv->len,
             udp_server.remote_ipstr(recv->remote), udp_server.remote_port(recv->remote));
   // ESP_LOG_BUFFER_HEXDUMP(TAG, recv->payload, recv->len, ESP_LOG_INFO);
    ESP_LOGI(TAG," << %s",txt);

    xTaskCreate(&uart_sender_task, "aaa_task", 4096, txt, tskIDLE_PRIORITY, NULL);
    vTaskDelay(50/portTICK_PERIOD_MS);

    free(txt);
 */
}