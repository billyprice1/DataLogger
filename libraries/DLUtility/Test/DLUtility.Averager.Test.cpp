#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "unity.h"

#include "../DLUtility.Averager.h"
#include "../DLUtility.HelperMacros.h"

/*
 * Defines and Typedefs
 */

template <typename T>
void testReset(T expected_result)
{
	Averager<T> averager = Averager<T>(20);
	averager.reset(&expected_result);

	TEST_ASSERT_EQUAL(expected_result, averager.getAverage());

	T zero = 0;
	averager.reset(&zero);
	TEST_ASSERT_EQUAL(0, averager.getAverage());

	averager.reset(NULL);
	TEST_ASSERT_EQUAL(0, averager.getAverage());
}

template <typename T>
void testRunning(T * data_ptr, uint16_t size, T expected_result, float expectedFloatResult)
{
	Averager<T> averager = Averager<T>(size);
	averager.fillFromArray(data_ptr, size);
	TEST_ASSERT_EQUAL(expected_result, averager.getAverage());
	TEST_ASSERT_EQUAL_FLOAT(expectedFloatResult, averager.getFloatAverage());
}

/*
 * Private Test Variables
 */
 
// The average of these data is 8 (rounded)
static int8_t s8data[] = {58, -41, 103, 127, 104, -84, 80, -8, 4, -127, 50, -97, -69, 44, -57, 29, 25, 38, -101, 74};
static int8_t s8dataAverage = 8;
static float s8dataFloatAverage = 7.6f;

// The average of these data is 137 (rounded)
static uint8_t u8data[] = {71,45,228,13,126,199,250,104,147,178,143,180,88,194,123,115,156,59,70,248};
static uint8_t u8dataAverage = 137;
static float u8dataFloatAverage = 136.85f;

// The average of these data is 5650 (rounded)
static int16_t s16data[] =
		{-3739, 22893, 7805, 17336, -11750, 28826, -2848, 32719, 3500, 14826, -14924, -2126, 13481, -1671, -2755, 26115, -10519, -32498, 22731, 5599};
static int16_t s16dataAverage = 5650;
static float s16dataFloatAverage = 5650.05f;

// The average of these data is 30019 (rounded)
static uint16_t u16data[] =
		{59044,44841,799,40117,15411,3329,65183,44259,4952,53251,23130,41137,21522,31266,4668,45402,28737,4986,18550,49794};
static uint16_t u16dataAverage = 30019;
static float u16dataFloatAverage = 30018.9f;

// The average of these data is 18173 (rounded)
static int32_t s32data[] =
		{20555, 26169, 3383, 31579, 12439, 10241, 27172, 81, 16275, 29671, 20419, 9156, 18084, 13494, 17657, 28744, 27936, 30908, 2160, 17334};
static int32_t s32dataAverage = 18173;
static float s32dataFloatAverage = 18172.85f;

// The average of these data is 137303231 (rounded)
static uint32_t u32data[] =
		{247257524, 93168365, 213206737, 173374918, 130153833, 247899438, 230140165, 68239320, 
		109850936, 159456795, 144173778, 261536527, 3055151, 74545759, 3605332, 212496497, 
		31211219, 277576, 192502949, 149911801};
static uint32_t u32dataAverage = 137303231;
static float u32dataFloatAverage = 137303231.0f;

static int8_t s8resetValue = -10;
static uint8_t u8resetValue = 10;
static int16_t s16resetValue = -10;
static uint16_t u16resetValue = 10;
static int32_t s32resetValue = -10;
static uint32_t u32resetValue = 10;

void setUp(void)
{

}

void tearDown(void)
{

}

void test_AveragerS8Running(void) {	testRunning(s8data, N_ELE(s8data), s8dataAverage, s8dataFloatAverage); }
void test_AveragerU8Running(void) {	testRunning(u8data, N_ELE(u8data), u8dataAverage, u8dataFloatAverage); }
void test_AveragerS16Running(void) { testRunning(s16data, N_ELE(s16data), s16dataAverage, s16dataFloatAverage); }
void test_AveragerU16Running(void) { testRunning(u16data, N_ELE(u16data), u16dataAverage, u16dataFloatAverage); }
void test_AveragerS32Running(void) { testRunning(s32data, N_ELE(s32data), s32dataAverage, s32dataFloatAverage); }
void test_AveragerU32Running(void) { testRunning(u32data, N_ELE(u32data), u32dataAverage, u32dataFloatAverage); }

void test_AveragerS8Reset(void) { testReset(s8resetValue); }
void test_AveragerU8Reset(void) { testReset(u8resetValue); }
void test_AveragerS16Reset(void) { testReset(s16resetValue); }
void test_AveragerU16Reset(void) { testReset(u16resetValue); }
void test_AveragerS32Reset(void) { testReset(s32resetValue); }
void test_AveragerU32Reset(void) { testReset(u32resetValue); }

void test_AveragerSizeIsCorrect(void)
{
	Averager<uint32_t> averager = Averager<uint32_t>(32);

	uint8_t i;
	for (i = 0; i < 32; i++)
	{
		TEST_ASSERT_EQUAL(i, averager.N());
		TEST_ASSERT_FALSE(averager.full());
		averager.newData(0);
	}
	TEST_ASSERT_TRUE(averager.full());
}

//=======Test Reset Option=====
void resetTest()
{
  tearDown();
  setUp();
}

//=======MAIN=====
int main(void)
{
	UnityBegin("DLUtility.Averager.Test.cpp");

	RUN_TEST(test_AveragerS8Running);
	RUN_TEST(test_AveragerU8Running);
	RUN_TEST(test_AveragerS16Running);
	RUN_TEST(test_AveragerU16Running);
	RUN_TEST(test_AveragerS32Running);
	RUN_TEST(test_AveragerU32Running);

	RUN_TEST(test_AveragerS8Reset);
	RUN_TEST(test_AveragerS16Reset);
	RUN_TEST(test_AveragerS32Reset);
	RUN_TEST(test_AveragerU8Reset);
	RUN_TEST(test_AveragerU16Reset);
	RUN_TEST(test_AveragerU32Reset);

	RUN_TEST(test_AveragerSizeIsCorrect);
	
	return (UnityEnd());
}
