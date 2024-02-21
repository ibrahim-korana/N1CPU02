#ifndef __TOOL_H__
#define __TOOL_H__

#define PCF_4 

#include "ice_pcf8574.h"

#include "geneltanim.h" 

#define BIN_PATTERN "%c %c %c %c   %c %c %c %c"
#define BYTE_TO_BIN(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

esp_err_t pcf_init(i2c_dev_t *pp, uint8_t addr, uint8_t retry, bool log, gpio_num_t LLD);
void inout_test(i2c_dev_t **pcf);
bool out_pin_convert(uint8_t pin, uint8_t *pcfno, uint8_t *pcfpin);
bool in_pin_convert(uint8_t pin, uint8_t *pcfno, uint8_t *pcfpin);

void test01(uint16_t a_delay2, i2c_dev_t **pcf);
void test02(uint16_t a_delay2, i2c_dev_t **pcf);
void rs485_output_test(void);

#endif