#ifndef _HTIME_H
#define _HTIME_H

#include <time.h>
#include <sys/time.h>
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_sntp.h"
#include <stdio.h>
#include <string.h>


void Set_SystemTime_SNTP(void);
void Get_current_date_time(char *date_time);
time_t Get_ctime(void);
void Get_ctime(char *td, uint8_t len);
void Set_ctime(time_t t);




#endif