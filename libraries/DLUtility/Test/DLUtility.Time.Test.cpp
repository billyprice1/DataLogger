#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "unity.h"

#include "../DLUtility.Time.h"

static char message[100];

static char const * wkdy[] = {
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"
};

void test_LeapYears(void)
{
	TEST_ASSERT_EQUAL(is_leap_year(1970), false);
	TEST_ASSERT_EQUAL(is_leap_year(1971), false);
	TEST_ASSERT_EQUAL(is_leap_year(1972), true);
	TEST_ASSERT_EQUAL(is_leap_year(1973), false);
}

void test_DaysInMonth(void)
{
	TEST_ASSERT_EQUAL(days_in_month(1, true), 29);
	TEST_ASSERT_EQUAL(days_in_month(1, false), 28);
}

void test_UnixSecondsToTime(void)
{
	UNIX_TIMESTAMP seconds = 0;
	TM time;
	int dd = 1;
	int mm = 0;
	int yy = 70;
	int yday = 0;
	
	bool bLeapYear = is_leap_year(1970);
	char * pMessage = message;
	
	do
	{
		unix_seconds_to_time(seconds, &time);
		
		pMessage = message;
		pMessage += sprintf(pMessage, "%lu - %s ", seconds, wkdy[time.tm_wday]);
		pMessage += sprintf(pMessage, "%d/%d/%d ", time.tm_mday, time.tm_mon + 1, TWO_DIGIT_YEAR(time.tm_year));
		
		if (dd > days_in_month(mm, bLeapYear))
		{
			dd = 1;
			++mm;
			if (mm == (DECEMBER+1))
			{
				mm = JANUARY;
				++yy;
				yday = 0;
				bLeapYear = is_leap_year(yy+1900);
			}
		}
		
		TEST_ASSERT_EQUAL_MESSAGE(dd, time.tm_mday, message);
		TEST_ASSERT_EQUAL_MESSAGE(mm, time.tm_mon, message);
		TEST_ASSERT_EQUAL_MESSAGE(yy, time.tm_year, message);
		TEST_ASSERT_EQUAL_MESSAGE(yday, time.tm_yday, message);
		
		++dd;
		++yday;
		
	} while ((seconds += 86400) < 1379969817);
}

void test_TimeToUnixSeconds(void)
{
	UNIX_TIMESTAMP seconds = 0;
	TM time;
	int dd = 1;
	int mm = 0;
	int yy = 70;

	bool bLeapYear = is_leap_year(1970);
	char * pMessage = message;
	
	do
	{
		unix_seconds_to_time(seconds, &time);
		
		pMessage = message;
		pMessage += sprintf(pMessage, "%lu - %s ", seconds, wkdy[time.tm_wday]);
		pMessage += sprintf(pMessage, "%d/%d/%d (%d)", time.tm_mday, time.tm_mon + 1, TWO_DIGIT_YEAR(time.tm_year), time.tm_yday);
		
		if (dd > days_in_month(mm, bLeapYear))
		{
			dd = 1;
			++mm;
			if (mm == (DECEMBER+1))
			{
				mm = JANUARY;
				++yy;
				bLeapYear = is_leap_year(yy+1900);
			}
		}

		TEST_ASSERT_EQUAL_MESSAGE(seconds, time_to_unix_seconds(&time), message);
		++dd;
	} while ((seconds += 86400) < 1379969817);
}

void test_IncrementSeconds(void)
{
	TM time;
	UNIX_TIMESTAMP seconds;
	unix_seconds_to_time(seconds = 0, &time); // 23:59:58 on 30DEC99
	
	time_increment_seconds(&time);
	TEST_ASSERT_EQUAL(time_to_unix_seconds(&time), seconds = 1);  // 23:59:59 on 30DEC99
	
	time_increment_seconds(&time);
	TEST_ASSERT_EQUAL(time_to_unix_seconds(&time), seconds = 2);  // 00:00:00 on 01JAN00
	
	for (int i = 0; i < 1379969817; ++i)
	{
		time_increment_seconds(&time);
		TEST_ASSERT_EQUAL(time_to_unix_seconds(&time), ++seconds);
	}
}

//=======MAIN=====
int main(void)
{
  UnityBegin("DLUtility.Time.cpp");
  RUN_TEST(test_LeapYears);
  RUN_TEST(test_DaysInMonth);
  RUN_TEST(test_UnixSecondsToTime);
  RUN_TEST(test_TimeToUnixSeconds);
  RUN_TEST(test_IncrementSeconds); 
  return (UnityEnd());
}