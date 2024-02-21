#include "tcpserver.h"

void TcpServer::get_clients_address(struct sockaddr_storage *source_addr, char *address_str)
{
    char *res = NULL;
    //printf("Kaynak %d\n",((struct sockaddr_in *)source_addr)->sin_addr.s_addr); 32bit adres
    if (source_addr->ss_family == PF_INET) {
        res = inet_ntoa_r(((struct sockaddr_in *)source_addr)->sin_addr, address_str, 14);
    }
    if (!res) {
        address_str[0] = '\0';
    }
}

int TcpServer::socket_send(const char *tag, const int sock, const char * data, const size_t len)
{
    int to_write = len;
    while (to_write > 0) {
        int written = send(sock, data + (len - to_write), to_write, 0);
        if (written < 0 && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK) {
            log_socket_error(tag, sock, errno, "Gönderimde hata oluştu");
            return -1;
        }
        to_write -= written;
    }
    return len;
}

int TcpServer::try_receive(const char *tag, const int sock, uint8_t * data, size_t max_len)
{
    int len = recv(sock, data, max_len, 0);
    if (len < 0) {
        if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;   // Not an error
        }
        if (errno == ENOTCONN) {
            ESP_LOGW(tag, "[sock=%d]: Soket kapatıldı", sock);
            return -2;  
        }
        log_socket_error(tag, sock, errno, "Soket kapatılmış");
        return -1;
    }
    return len;
}

void TcpServer::log_socket_error(const char *tag, const int sock, const int err, const char *message)
{
    ESP_LOGE(tag, "[sock=%d]: %s\n"
                  "error=%d: %s", sock, message, err, strerror(err));
}

void TcpServer::tcp_server_task(void *arg)
{
    TcpServer *mthis = (TcpServer *)arg;
reopen:    
    uint8_t *rx_buffer = (uint8_t *)malloc(BUFFER_SIZE);
    int err = 0, flags = 0, flag0 = 1;
    
    struct addrinfo hints = {};
        hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *address_info;
    int listen_sock = INVALID_SOCK;
    const size_t max_socks = MAX_SOCKET - 1;
    static int sock[MAX_SOCKET - 1];

    for (int i=0; i<max_socks; ++i) {
        sock[i] = INVALID_SOCK;
    }

    int res = getaddrinfo(mthis->adres, mthis->port, &hints, &address_info);
    if (res != 0 || address_info == NULL) {
        ESP_LOGE(mthis->TAGTCP, "%s için getaddrinfo() %d hatası oluşturdu addrinfo=%p", mthis->adres, res, address_info);
        goto error;
    }

    listen_sock = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);

    if (listen_sock < 0) {
        log_socket_error(mthis->TAGTCP, listen_sock, errno, "Soket oluşturulamadı");
        goto error;
    }
    ESP_LOGI(mthis->TAGTCP, "soket oluşturuldu");

    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &flag0, sizeof(flag0));

    // Marking the socket as non-blocking
    flags = fcntl(listen_sock, F_GETFL);
    if (fcntl(listen_sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        log_socket_error(mthis->TAGTCP, listen_sock, errno, "Soket non bloking olarak işaretlenemiyor");
        goto error;
    }

    err = bind(listen_sock, address_info->ai_addr, address_info->ai_addrlen);
    if (err != 0) {
        log_socket_error(mthis->TAGTCP, listen_sock, errno, "Soket adrese bind edilemedi");
        goto error;
    }
    ESP_LOGI(mthis->TAGTCP, "%s:%s başlatıldı",mthis->adres,mthis->port);
    
    err = listen(listen_sock, 1);
    if (err != 0) {
        log_socket_error(mthis->TAGTCP, listen_sock, errno, "Dinleme sırasında hata oluştu");
        goto error;
    }
    ESP_LOGI(mthis->TAGTCP, "Socket dinleniyor");
    if (mthis->server_ready!=NULL) xSemaphoreGive(mthis->server_ready);

    while (1) {
        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);

        int new_sock_index = 0;
        for (new_sock_index=0; new_sock_index<max_socks; ++new_sock_index) {
            if (sock[new_sock_index] == INVALID_SOCK) {
                break;
            }
        }

        if (new_sock_index < max_socks) {
            sock[new_sock_index] = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
            if (sock[new_sock_index] < 0) {
                if (errno == EWOULDBLOCK) { // The listener socket did not accepts any connection
                                            // continue to serve open connections and try to accept again upon the next iteration
                    ESP_LOGV(mthis->TAGTCP, "Bekleyen baglantı yok...");
                } else {
                    log_socket_error(mthis->TAGTCP, listen_sock, errno, "Baglantı kabulünde hata");
                    goto error;
                }
            } else {
                char *ip = (char *) malloc(15);
                mthis->get_clients_address(&source_addr,ip); //ip adresini bul
                char *mac =(char*)malloc(16);
                mthis->cihaz->get_sta_mac(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr,mac); //mac idsini bul
                if (mac!=NULL)
                {
                    device_register_t *Bag=mthis->cihaz->cihazbul(mac); //mac e göre cihazı bul
                    if (Bag!=NULL) //Cihaz varsa ip ve soketi ekle
                        {
                            Bag->socket = sock[new_sock_index];
                            Bag->ip=((struct sockaddr_in *)&source_addr)->sin_addr.s_addr;
                        } else {
                            //Cihaz yoksa oluştur ip ve soketi ekle
                            Bag = mthis->cihaz->cihaz_ekle(mac,TR_UDP);
                            Bag->socket = sock[new_sock_index];
                            Bag->ip=((struct sockaddr_in *)&source_addr)->sin_addr.s_addr;
                        }  
                    ESP_LOGI(mthis->TAGTCP, "[sock=%d]: Connection accepted from IP:%s MAC:%s", sock[new_sock_index], ip,mac);
                    // ...and set the client's socket non-blocking
                    flags = fcntl(sock[new_sock_index], F_GETFL);
                    if (fcntl(sock[new_sock_index], F_SETFL, flags | O_NONBLOCK) == -1) {
                        log_socket_error(mthis->TAGTCP, sock[new_sock_index], errno, "Soket Non-block yapılamadı");
                        goto error;
                    }
                } else {
                    //Cihazın mac id si bbulunamadı
                    ESP_LOGI(mthis->TAGTCP, "[sock=%d]: IP:%s  Mac Id si bulunamadı.. Baglantı kapatılıyor.", sock[new_sock_index],ip);
                    close(sock[new_sock_index]);
                    sock[new_sock_index] = INVALID_SOCK;
                }
                free(mac); mac=NULL;
                free(ip);ip=NULL;                
            }
        }

        for (int i=0; i<max_socks; ++i) {
            if (sock[i] != INVALID_SOCK) {
                int len = try_receive(mthis->TAGTCP, sock[i], rx_buffer, BUFFER_SIZE-1);
                if (len < 0) {
                    close(sock[i]);
                    sock[i] = INVALID_SOCK;
                } else if (len > 0) {
                    //ESP_LOGI(mthis->TAG, "[sock=%d]: Received %.*s", sock[i], len, rx_buffer);
                    if (rx_buffer[len-1]=='&')
                      {
                        rx_buffer[len-1]=0;
                        device_register_t *Bag=mthis->cihaz->cihazbul_soket(sock[i]); 

                        if (mthis->callback!=NULL)
                        {
                            char *res = (char *)malloc(512);
                            memset(res,0,512);
                            mthis->callback((char*)rx_buffer, Bag->device_id, TR_UDP,(void *)res, false);
                            if (strlen(res)>2) 
                            {
                                strcat(res,"&");
                                //printf("callback Gidiyor\n");
                                socket_send(mthis->TAGTCP, sock[i], res, strlen(res));
                                vTaskDelay(150 / portTICK_PERIOD_MS);
                            }
                            free(res);res=NULL;
                        }
                      }
                }

            } // one client's socket
        } // for all sockets

        // Yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(YIELD_TO_ALL_MS));
    }
error:
    mthis->server_fatal_error=true;
    if (listen_sock != INVALID_SOCK) {
        close(listen_sock);
        listen_sock = INVALID_SOCK;
    }

    for (int i=0; i<max_socks; ++i) {
        if (sock[i] != INVALID_SOCK) {
            close(sock[i]);
            sock[i] = INVALID_SOCK;
        }
    }
     ESP_LOGE(mthis->TAGTCP, "Server Kapatıldı");
    free(address_info);
    free(rx_buffer);
    rx_buffer = NULL;
    goto reopen;
    vTaskDelete(NULL);
}

bool TcpServer::Start(home_network_config_t *cnf, 
                      home_global_config_t *hcnf, 
                      uint16_t tcpport,
                      transmisyon_callback_t cb,
                      Cihazlar *chz
                      )
{   
    net_config = cnf;
    dev_config = hcnf;

    sprintf(port,"%d",tcpport);
    strcpy(adres,addr.to_string(net_config->home_ip));
    
    callback = cb;
    
        cihaz = chz;
        server_ready = xSemaphoreCreateBinary();
        assert(server_ready);
        xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)this, 5, NULL);
        xSemaphoreTake(server_ready, portMAX_DELAY);
        vSemaphoreDelete(server_ready);
        server_ready=NULL;
        
        return !server_fatal_error;
}

esp_err_t TcpServer::_Send(char *data, int port)
{
    if (port>0)
    {
        char *dt = (char*)malloc(strlen(data)+2);
        strcpy(dt,data);
        strcat(dt,"&");
        int data_size = strlen(dt);
        int len = socket_send(TAGTCP, port, dt, data_size);
        free(dt);
        ESP_LOGI(TAGTCP,"GIDEN >> %s %s", (len==data_size)?"OK":"FAIL",data);
        if (len!=data_size) return ESP_FAIL;
        return ESP_OK;
    } else return ESP_FAIL;
}

esp_err_t TcpServer::Send(char *data, uint8_t id)
{
    //Soketi bul
    device_register_t *bag = cihaz->cihazbul(id);
    if (bag!=NULL) {
        return _Send(data, bag->socket);
    } else return ESP_FAIL;
}

esp_err_t TcpServer::PortSend(char *data, int port)
{
   return _Send(data, port); 
}
