
#include "udp_server.h"

const char *UDP_SRV_TAG = "Udp_Server";

#define TIMEOUT 3000

void Udp_Server::_on_send(void *handler_arg, esp_event_base_t base,int32_t id, void *event_data)
{
    Udp_Server *self = (Udp_Server *)handler_arg;
    if (self->pcb == NULL || event_data == NULL) {
        return;
    }

    udp_msg_t *msg = (udp_msg_t *)event_data;
    remote_t *target = (remote_t *)msg->remote;
    struct pbuf *send_pb = pbuf_alloc(PBUF_TRANSPORT, msg->len, PBUF_RAM);
    memcpy(send_pb->payload, msg->payload, msg->len);
    ESP_LOGI(UDP_SRV_TAG, "Send %d bytes to %s:%d", send_pb->len,
             ipaddr_ntoa(&target->addr), target->port);
    
    udp_sendto(self->pcb, send_pb, &target->addr, target->port);
    pbuf_free(send_pb);

    free(msg->remote);
    free(msg->payload);
}

void Udp_Server::_on_broadcast(void *handler_arg, esp_event_base_t base,int32_t id, void *event_data)
{
    Udp_Server *self = (Udp_Server *)handler_arg;
    if (self->pcb == NULL || event_data == NULL) {
        return;
    }

    udp_broadcast_t *msg = (udp_broadcast_t *)event_data;
    struct pbuf *send_pb = pbuf_alloc(PBUF_TRANSPORT, msg->len, PBUF_RAM);
    memcpy(send_pb->payload, msg->payload, msg->len);
   // free(msg->payload);
    ESP_LOGI(UDP_SRV_TAG, "Send Broadcast %d bytes to port %d", send_pb->len, msg->port);
    udp_sendto(self->pcb, send_pb, IP_ADDR_BROADCAST, msg->port);
    pbuf_free(send_pb);   
}

udp_msg_t *Udp_Server::copy_msg(udp_msg_t *msg)
{
    udp_msg_t *ms=( udp_msg_t *)calloc(1,sizeof(udp_msg_t));
    ms->len = msg->len;
    ms->remote = malloc(sizeof(remote_t));
    ms->payload = malloc(ms->len);
    memcpy(ms->remote, msg->remote, sizeof(remote_t));
    memcpy(ms->payload, msg->payload, ms->len);
    return ms;
}


void Udp_Server::_recv_proc(void *arg, struct udp_pcb *pcb, struct pbuf *pb,const ip_addr_t *addr, uint16_t port)
{
    Udp_Server *ths = (Udp_Server *)arg;
    while (pb != NULL) {
        struct pbuf *this_pb = pb;
        bool jm = true;
        pb = pb->next;
        this_pb->next = NULL;
        ESP_LOGD(UDP_SRV_TAG, "Received %d bytes from %s:%d", this_pb->len,
                 ipaddr_ntoa(addr), port);
        udp_msg_t msg = {
            .len = this_pb->len,
        };
        msg.remote = malloc(sizeof(remote_t));
        if (msg.remote == NULL) jm=false;
        if (jm)
        {
            bool jm1=true;
            msg.payload = malloc(this_pb->len+1);
            if (msg.payload == NULL) {
                free(msg.remote);
                jm1=false;
            }
            if (jm1) {
                remote_t *remote = (remote_t *)msg.remote;
                ip_addr_copy(remote->addr, *addr);
                remote->port = port;
                memcpy(msg.payload, this_pb->payload, this_pb->len);
                
                if (!ths->wait_ack)
                {
                    if (esp_event_post(UDP_EVENT, UDP_EVENT_RECV, &msg, sizeof(udp_msg_t),
                                                    20 / portTICK_PERIOD_MS) != ESP_OK) {
                                        free(msg.remote);
                                        free(msg.payload);
                                    }
                } else {
                    xQueueSendToBack(ths->Recv_Queue,&msg,0); 
                   // printf("Gelen AA %d %s\n",msg.len, (char*)msg.payload);
                    vTaskDelay(10/portTICK_PERIOD_MS);
                    free(msg.remote);
                    free(msg.payload);   
                }                
            }           
        }    
        pbuf_free(this_pb);
    }
    
}

esp_err_t Udp_Server::send_unicast(remote_t *remote,const uint8_t *data, uint8_t len)
{
    udp_msg_t send = {};
    send.len = len;
    send.payload = malloc(send.len);
    memcpy(send.payload, data, send.len);
    send.remote = remote;
    esp_event_post(UDP_EVENT, UDP_EVENT_SEND, &send,
                               sizeof(udp_msg_t),
                               20 / portTICK_PERIOD_MS);
    return ESP_OK;  
}
esp_err_t Udp_Server::send_broadcast(const uint8_t *data, uint8_t len)
{
    /*
    udp_broadcast_t msg = {};
    msg.len = len;
    msg.port = port;
    msg.payload = malloc(len);
    memcpy(msg.payload, data, msg.len);
    esp_event_post(UDP_EVENT, UDP_EVENT_BROADCAST, &msg,
                               sizeof(udp_broadcast_t),
                               20 / portTICK_PERIOD_MS);
  */


    struct pbuf *send_pb = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    memcpy(send_pb->payload, data, len);
   // ESP_LOGI(UDP_SRV_TAG, "Send Broadcast %d bytes to port %d", len, port);
    ESP_LOGI(UDP_SRV_TAG, "Send Broadcast >> %s", data);
    udp_sendto(pcb, send_pb, IP_ADDR_BROADCAST, port);
    pbuf_free(send_pb);   

    return ESP_OK;
}


udp_msg_t *Udp_Server::send_receive_broadcast(const uint8_t *data, uint8_t len, esp_err_t *err)
{
    return send_receive_broadcast(data,len,err,TIMEOUT);
}

udp_msg_t *Udp_Server::send_receive_unicast(remote_t *remote, const uint8_t *data, uint8_t len, esp_err_t *err)
{
    return send_receive_unicast(remote, data,len,err,TIMEOUT);
}

udp_msg_t *Udp_Server::send_receive_broadcast(const uint8_t *data, uint8_t len, esp_err_t *err, uint16_t timeout)
{
    struct pbuf *send_pb = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    memcpy(send_pb->payload, data, len);
    udp_sendto(pcb, send_pb, IP_ADDR_BROADCAST, port);
    pbuf_free(send_pb);
    
    wait_ack = true;    
    udp_msg_t gelen = {};
    BaseType_t status = xQueueReceive(Recv_Queue,&gelen,pdMS_TO_TICKS(timeout));
    wait_ack = false;
    *err = ESP_FAIL;
    if (status==pdPASS)
    {
        udp_msg_t *mm = copy_msg(&gelen);
        *err = ESP_OK;
        return mm;
    } else {
        *err = ESP_ERR_TIMEOUT;
        return NULL;
    }
    return NULL;
}

udp_msg_t *Udp_Server::send_receive_unicast(remote_t *remote, const uint8_t *data, uint8_t len, esp_err_t *err, uint16_t timeout)
{
    struct pbuf *send_pb = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    memcpy(send_pb->payload, data, len);
    ESP_LOGD(UDP_SRV_TAG, "Send %d bytes to %s:%d", send_pb->len,
             ipaddr_ntoa(&remote->addr), remote->port);
    
    udp_sendto(pcb, send_pb, &remote->addr, remote->port);
    pbuf_free(send_pb);
    wait_ack = true;    
    udp_msg_t gelen = {};
    BaseType_t status = xQueueReceive(Recv_Queue,&gelen,pdMS_TO_TICKS(timeout));
    wait_ack = false;
    *err = ESP_FAIL;
    if (status==pdPASS)
    {
        udp_msg_t *mm = copy_msg(&gelen);
        *err = ESP_OK;
        return mm;
    } else {
        *err = ESP_ERR_TIMEOUT;
        return NULL;
    }
    return NULL;
}

esp_err_t Udp_Server::_deinit(void)
{
    esp_event_handler_unregister(UDP_EVENT, UDP_EVENT_SEND, _on_send);
    esp_event_handler_unregister(UDP_EVENT, UDP_EVENT_BROADCAST,_on_broadcast);
    if (pcb != NULL) {
        udp_remove(pcb);
        pcb = NULL;
    }
    return ESP_OK;
}

esp_err_t Udp_Server::stop(void)
{
    return _deinit();   
}

esp_err_t Udp_Server::start(uint16_t prt)
{
    port = prt;
    esp_err_t ret=ESP_OK;
    if (pcb!=NULL) {
        ESP_LOGE(UDP_SRV_TAG, "Udp server is running");
        return ESP_ERR_INVALID_STATE;
    }
    udp_init();
    pcb = udp_new();
    if (pcb == NULL) {
        ESP_LOGE(UDP_SRV_TAG, "Init udp server failed!");
        return ESP_ERR_NO_MEM;
    }
    ret = udp_bind(pcb, IP_ADDR_ANY, port);
    if (ret != ERR_OK) {
        ESP_LOGE(UDP_SRV_TAG, "Bind udp server failed: %d", ret);
        ret = ESP_ERR_INVALID_STATE;
        goto failed;
    }

    udp_recv(pcb, &_recv_proc, (void*)this);

    ret = esp_event_handler_register(UDP_EVENT, UDP_EVENT_SEND,_on_send, (void*)this);
    if (ret != ESP_OK) {
        ESP_LOGE(UDP_SRV_TAG, "Register udp server send failed: %d", ret);
        goto failed;
    }
    ret = esp_event_handler_register(UDP_EVENT, UDP_EVENT_BROADCAST,_on_broadcast, (void*)this);
    if (ret != ESP_OK) {
        ESP_LOGE(UDP_SRV_TAG, "Register udp server broadcast failed: %d", ret);
        goto failed;
    }

    Recv_Queue = xQueueCreate(5,sizeof(udp_msg_t));
    if (Recv_Queue ==NULL) {
        ESP_LOGE(UDP_SRV_TAG, "send_receive Queue failed");
        goto failed;
    }

    ESP_LOGW(UDP_SRV_TAG,"UDP Server %d port Running",port);

    return ESP_OK;

    failed:
        _deinit();
        return ret;
}

char *Udp_Server::remote_ipstr(const void *remote)
{
    if (remote == NULL) {
        return NULL;
    }
    remote_t *target = (remote_t *)remote;
    return ipaddr_ntoa(&target->addr);
}

int Udp_Server::remote_port(const void *remote)
{
    if (remote == NULL) {
        return -1;
    }
    remote_t *target = (remote_t *)remote;
    return target->port;
}

remote_t *Udp_Server::create_remote(const char *ip)
{
    remote_t *rm = (remote_t *)calloc(1,sizeof(remote_t));
    ip4addr_aton(ip, &rm->addr.u_addr.ip4);
    rm->port = port;
    return rm;
}