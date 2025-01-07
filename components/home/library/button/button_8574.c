
#include "button_8574.h"
#include "../pcf/ice_pcf8574.h"

static const char *TAG = "8574 button";

#define GPIO_BTN_CHECK(a, str, ret_val)                          \
    if (!(a))                                                     \
    {                                                             \
        ESP_LOGE(TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val);                                         \
    }

esp_err_t button_8574_init(const button_8574_config_t *config)
{
    GPIO_BTN_CHECK(NULL != config, "Pointer of config is invalid", ESP_ERR_INVALID_ARG);
    config->device->reg |= (1 << config->pin_num);
    pcf8574_pin_write(config->device,config->pin_num,1);
    return ESP_OK;
}

esp_err_t button_8574_deinit(int gpio_num)
{
    return ESP_OK;
}

uint8_t button_8574_get_key_level(void *hardware)
{
    button_8574_config_t *cfg = (button_8574_config_t *) hardware;
    uint8_t dt=0;
    pcf8574_port_read(cfg->device,&dt);
    //if (cfg->device->addr==0x20) printf("%02X\n",dt);
    return (dt & (1 << (uint32_t)cfg->pin_num)) > 0;
}


