/*
   MIT License

  Copyright (c) 2021 Felix Biego

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef ESP32TIME_H
#define ESP32TIME_H

#include <time.h>
#include <sys/time.h>
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_sntp.h"
#include <stdio.h>
#include <string.h>

class ESP32Time {
	
	public:
		ESP32Time();
		ESP32Time(unsigned long offset);
		void setTime(unsigned long epoch = 1609459200, int ms = 0);	// default (1609459200) = 1st Jan 2021
		void setTime(int sc, int mn, int hr, int dy, int mt, int yr, int ms = 0);
		void setTimeStruct(tm t);
		tm getTimeStruct();
		void getTime(char *ret, uint8_t len, char * format);
		
		void getTime(char *ret);
		void getDateTime(char *ret, bool mode = false);
		void getTimeDate(char *ret, bool mode = false);
		void getTimeDateTR(char *ret);
		void getDate(char *ret, bool mode = false);
		void getAmPm( char *ret, bool lowercase = false);
		
		unsigned long getEpoch();
		unsigned long getMillis();
		unsigned long getMicros();
		int getSecond();
		int getMinute();
		int getHour(bool mode = false);
		int getDay();
		int getDayofWeek();
		int getDayofYear();
		int getMonth();
		int getYear();
		
		unsigned long offset;
		unsigned long getLocalEpoch();
		
	private:
		bool overflow;
		

};


#endif
