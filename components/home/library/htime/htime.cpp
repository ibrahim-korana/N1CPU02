

#include "esp_log.h"
#include "htime.h"

const char *TM_TAG = "Time";


void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TM_TAG, "Notification of a time synchronization event");
}

time_t Get_ctime(void)
{
    struct timeval tv;
	gettimeofday(&tv, NULL);
    return tv.tv_usec;
}

void Set_ctime(time_t t)
{
    struct timeval tv;
    tv.tv_usec = t;
    setenv("TZ", "UTC-03:00", 1);
	tzset();
    settimeofday(&tv, NULL); 
}

void Get_ctime(char *td, uint8_t len)
{
    time_t aa = Get_ctime();
    struct tm *timeinfo = localtime(&aa);
    strftime(td, len, "%d.%m.%Y %H:%M:%S", timeinfo);
}

void Get_current_date_time(char *date_time)
{
	char strftime_buf[64];
	time_t now;
	    struct tm timeinfo;
	    time(&now);
	    localtime_r(&now, &timeinfo);

	    	// Set timezone to Indian Standard Time
	    	    setenv("TZ", "UTC-03:00", 1);
	    	    tzset();
	    	    localtime_r(&now, &timeinfo);

	    	    strftime(strftime_buf, sizeof(strftime_buf), "%d.%m.%Y %H:%M:%S", &timeinfo);
                /*
                sprintf(strftime_buf,"%02d.%02d.%04d %02d:%02d:%02d",  timeinfo.tm_mday,timeinfo.tm_mon,timeinfo.tm_year,
                    timeinfo.tm_hour ,
                    timeinfo.tm_min ,
                    timeinfo.tm_sec
                 );
                 */
	    	    ESP_LOGI(TM_TAG, "The current date/time is: %s", strftime_buf);
                strcpy(date_time,strftime_buf);

                //tm yi time_t ye çevir
                //time_t aaa = mktime(&timeinfo);
}


static void initialize_sntp(void)
{
    ESP_LOGI(TM_TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
/*    
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
*/
    esp_sntp_init();
}
static void obtain_time(void)
{
    initialize_sntp();
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = {};
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TM_TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}
 void Set_SystemTime_SNTP(void)  {

	    time_t now;
	    struct tm timeinfo;
	    time(&now);
	    localtime_r(&now, &timeinfo);
	    // Is time set? If not, tm_year will be (1970 - 1900).
	    if (timeinfo.tm_year < (2016 - 1900)) {
	        ESP_LOGI(TM_TAG, "Time is not set yet. Connecting Internet and getting time over NTP.");
	        obtain_time();
	        // update 'now' variable with current time
	        time(&now);
	    }
}

