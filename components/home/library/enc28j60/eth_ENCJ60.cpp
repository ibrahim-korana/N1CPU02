
#include "eth_ENCJ60.h"

static const char *TAG = "ETH_ENCJ60";

#define ETH_SPI_HOST (spi_host_device_t) CONFIG_ICE_SPI_HOST
#define ETH_SPI_SCLK_GPIO CONFIG_ICE_ETH_SCLK
#define ETH_SPI_MOSI_GPIO CONFIG_ICE_ETH_MOSI
#define ETH_SPI_MISO_GPIO CONFIG_ICE_ETH_MISO
#define ETH_SPI_CLOCK_MHZ CONFIG_ICE_ETH_CLOCK_MHZ
#define ETH_SPI_CS0_GPIO CONFIG_ICE_ETH_CS
#define ETH_SPI_INT0_GPIO CONFIG_ICE_ETH_INT
#define ETH_SPI_PHY_RST0_GPIO CONFIG_ICE_ETH_RST
#define ETH_SPI_PHY_ADDR0 CONFIG_ICE_ETH_PHY_ADDR

static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    Ethj60 *aa=(Ethj60 *) arg;
    uint8_t mac_addr[6] = {0};
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);         
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        xEventGroupClearBits(aa->NetEvent, NETWORK_ETH);
        xEventGroupSetBits(aa->NetEvent, NETWORK_ETH_ERR | NETWORK_ERROR);
        if (aa->callback !=NULL) aa->callback(aa);
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        xEventGroupClearBits(aa->NetEvent, NETWORK_ETH);
        xEventGroupSetBits(aa->NetEvent, NETWORK_ETH_ERR | NETWORK_ERROR);
        if (aa->callback !=NULL) aa->callback(aa);
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    Ethj60 *aa=(Ethj60 *) arg;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~~~~~~~~~~~~");
    xEventGroupSetBits(aa->NetEvent, NETWORK_ETH);
    xEventGroupClearBits(aa->NetEvent, NETWORK_ETH_ERR | NETWORK_ERROR);
    if (aa->callback !=NULL) aa->callback(aa);
}

bool Ethj60::init(network_config_t cnf , EventGroupHandle_t NetEv, change_callback_t cb)
{
    mConfig = cnf;
    NetEvent = NetEv;
    xEventGroupClearBits(NetEvent, NETWORK_ETH | NETWORK_ETH_ERR | NETWORK_ERROR);
    callback = cb;
    return true;
}

bool Ethj60::start(void)
{
    gpio_install_isr_service(0);
    if(esp_netif_init()!=ESP_OK) return false;
    if(esp_event_loop_create_default()!=ESP_OK) return false;
    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&netif_cfg);

 if (eth_netif)
    {
      if (mConfig.ipstat==STATIC_IP)
        {  
            esp_netif_ip_info_t info_t;
            memset(&info_t, 0, sizeof(esp_netif_ip_info_t));
            if(esp_netif_dhcpc_stop(eth_netif)!=ESP_OK) return false;
            info_t.ip.addr = esp_ip4addr_aton((const char *)mConfig.ip);
            info_t.netmask.addr = esp_ip4addr_aton((const char *)mConfig.netmask);
            info_t.gw.addr = esp_ip4addr_aton((const char *)mConfig.gateway);
            esp_netif_set_ip_info(eth_netif, &info_t);
        }
    }

    spi_bus_config_t buscfg = {};
        buscfg.miso_io_num = ETH_SPI_MISO_GPIO;
        buscfg.mosi_io_num = ETH_SPI_MOSI_GPIO;
        buscfg.sclk_io_num = ETH_SPI_SCLK_GPIO;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz= 8186;

     if( spi_bus_initialize(ETH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK) return false;

    /* ENC28J60 ethernet driver is based on spi driver */
    spi_device_interface_config_t devcfg = {};
        devcfg.command_bits = 3;
        devcfg.address_bits = 5;
        devcfg.mode = 0;
        devcfg.clock_speed_hz = ETH_SPI_CLOCK_MHZ * 1000 * 1000;
        devcfg.spics_io_num = ETH_SPI_CS0_GPIO;
        devcfg.queue_size = 20;
        devcfg.cs_ena_posttrans = enc28j60_cal_spi_cs_hold_time(ETH_SPI_CLOCK_MHZ);

    spi_device_handle_t spi_handle = NULL;
    if(spi_bus_add_device((spi_host_device_t)ETH_SPI_HOST, &devcfg, &spi_handle)!=ESP_OK) return false;

    eth_enc28j60_config_t enc28j60_config = ETH_ENC28J60_DEFAULT_CONFIG(spi_handle);
    enc28j60_config.int_gpio_num = ETH_SPI_INT0_GPIO;

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.smi_mdc_gpio_num = -1;  
    mac_config.smi_mdio_gpio_num = -1;
    esp_eth_mac_t *mac = esp_eth_mac_new_enc28j60(&enc28j60_config, &mac_config);

    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.autonego_timeout_ms = 0; 
    phy_config.reset_gpio_num = -1; 
    esp_eth_phy_t *phy = esp_eth_phy_new_enc28j60(&phy_config);

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    if(esp_eth_driver_install(&eth_config, &eth_handle_spi)!=ESP_OK) return false;

    uint8_t aaa[] =  {
        0x02, 0x00, 0x00, 0x12, 0x34, 0x56
    };
    mac->set_addr(mac,  aaa);

    // ENC28J60 Errata #1 check
    if (emac_enc28j60_get_chip_info(mac) < ENC28J60_REV_B5 && ETH_SPI_CLOCK_MHZ < 8) {
        ESP_LOGE(TAG, "SPI frequency must be at least 8 MHz for chip revision less than 5");
        return false;
    }

    if(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle_spi))!=ESP_OK) return false;

    if(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, this)!=ESP_OK) return false;
    if(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, this)!=ESP_OK) return false;
    /* start Ethernet driver state machine */
    if(esp_eth_start(eth_handle_spi)!=ESP_OK) return false;

  //  enc28j60_set_phy_duplex(phy, ETH_DUPLEX_FULL);

        xEventGroupWaitBits(NetEvent,
            NETWORK_ETH | NETWORK_ETH_ERR,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

        if (callback != NULL) callback(this);

    return true;
};