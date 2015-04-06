/*
 * LinkItONE.Crest
 *
 * Application for Crest PV on LinkItONE hardware
 *
 * See README.md for full details
 *
 * Basic summary:
 *
 * - Reads data from ADS1115 ADCs (addresses 0x48, 0x49, 0x4A)
 * - Averages data every second
 * - Stores data to SD card every minute
 */

/*
 * Standard Library Includes
 */

#include <stdint.h>

/*
 * Arduino Library Includes
 */

#include <Wire.h>

/*
 * LinkIt One Includes
 */

#include <LSD.h>
#include <LGPS.h>
#include <LDateTime.h>

/*
 * DataLogger Includes
 */

#include "DLUtility.Time.h"
#include "DLTime.h"
#include "DLCSV.h"
#include "DLGPS.h"
#include "DLFilename.h"
#include "DLLocalStorage.h"
#include "DLUtility.h"
#include "DLLocation.h"
#include "DLSettings.h"
#include "DLDataField.h"
#include "DLSensor.ADS1x1x.h"
#include "DLSensor.LinkItONE.h"
#include "DLSensor.Thermistor.h"

#include "TaskAction.h"

/*
 * Application Includes
 */

#include "app.data_conversion.h"
#include "app.upload_manager.h"

// Pointers to fuctionality objects

static ADS1115 s_ADCs[] = {
    ADS1115(0x48),
    ADS1115(0x49),
    ADS1115(0x4A)
};

static LinkItONEADC s_adcs[] = {
    LinkItONEADC(A0),
    LinkItONEADC(A1),
    LinkItONEADC(A2)
};

static Thermistor s_thermistors[] = {
    Thermistor(3977, 10000),
    Thermistor(3977, 10000),
    Thermistor(3977, 10000)
};

// Short term (1 second) storage

// There are 12 averages for voltage and current data
#define AVERAGER_COUNT (12)

// ...plus three more fields for temperature data
#define THERMISTOR_COUNT (3)

#define FIELD_COUNT (AVERAGER_COUNT + THERMISTOR_COUNT)

#define ADC_READS_PER_SECOND (10)
#define MS_PER_ADC_READ (1000 / ADC_READS_PER_SECOND)

static Averager<uint16_t> s_averagers[] = {
    Averager<uint16_t>(ADC_READS_PER_SECOND),
    Averager<uint16_t>(ADC_READS_PER_SECOND),
    Averager<uint16_t>(ADC_READS_PER_SECOND),
    Averager<uint16_t>(ADC_READS_PER_SECOND),
    Averager<uint16_t>(ADC_READS_PER_SECOND),
    Averager<uint16_t>(ADC_READS_PER_SECOND),
    Averager<uint16_t>(ADC_READS_PER_SECOND),
    Averager<uint16_t>(ADC_READS_PER_SECOND),
    Averager<uint16_t>(ADC_READS_PER_SECOND),
    Averager<uint16_t>(ADC_READS_PER_SECOND),
    Averager<uint16_t>(ADC_READS_PER_SECOND),
    Averager<uint16_t>(ADC_READS_PER_SECOND)
};

// Medium term (10 minute) in-RAM data storage
#define STORE_TO_SD_INTERVAL_SECS (10)// * 60)
static NumericDataField<float> s_dataFields[] = {
    NumericDataField<float>(VOLTAGE, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(VOLTAGE, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(VOLTAGE, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(VOLTAGE, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(CURRENT, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(CURRENT, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(CURRENT, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(CURRENT, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(CURRENT, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(CURRENT, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(CURRENT, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(CURRENT, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(TEMPERATURE_C, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(TEMPERATURE_C, STORE_TO_SD_INTERVAL_SECS),
    NumericDataField<float>(TEMPERATURE_C, STORE_TO_SD_INTERVAL_SECS)
};

static CONVERSION_FN s_conversionFunctions[] = 
{
    channel01Conversion,
    channel02Conversion,
    channel03Conversion,
    channel04Conversion,
    channel05Conversion,
    channel06Conversion,
    channel07Conversion,
    channel08Conversion,
    channel09Conversion,
    channel10Conversion,
    channel11Conversion,
    channel12Conversion,
};

static LocalStorageInterface * s_sdCard;
static FILE_HANDLE s_fileHandle;
static uint32_t s_entryID = 0;

static TM s_time;
static TM s_lastSDTimestamp;

static uint16_t s_storedDataCount = 0;
static uint16_t s_adcReadCount = 0;

static void writeTimestampToOpenFile(void)
{
    char buffer[30];
    time_increment_seconds(&s_lastSDTimestamp);
    CSV_writeTimestampToBuffer(&s_lastSDTimestamp, buffer);
    s_sdCard->write(s_fileHandle, buffer);
    s_sdCard->write(s_fileHandle, ", ");
}

static void writeEntryIDToOpenFile(void)
{
    char buffer[30];
    s_entryID++;
    sprintf(buffer, "%d, ", s_entryID);
    s_sdCard->write(s_fileHandle, buffer);
}

static TaskAction averageAndStoreTask(averageAndStoreTaskFn, 1000, INFINITE_TICKS);
static void averageAndStoreTaskFn(void)
{
    s_storedDataCount++;
    uint8_t field = 0;
    uint8_t i;

    for (i = 0; i < AVERAGER_COUNT; i++)
    {
        uint16_t average = s_averagers[i].getAverage();
        s_averagers[i].reset(NULL);

        float toStore;
        if (s_conversionFunctions[i])
        {
            toStore = s_conversionFunctions[i](average); 
        }
        else
        {
            toStore = (float)average;
        }
        s_dataFields[i].storeData( toStore );

    }

    // The ADC range is 0 to 5v, but thermistors are on 3.3v rail, so maximum is 1023 * 3.3/5 = 675
    for (i = 0; i < THERMISTOR_COUNT; i++)
    {
        float data = s_thermistors[i].TemperatureFromADCReading(10000.0, s_adcs[i].read(), 675);
        s_dataFields[i + AVERAGER_COUNT].storeData(data);
    }
}

static TaskAction writeToSDCardTask(writeToSDCardTaskFn, STORE_TO_SD_INTERVAL_SECS*1000, INFINITE_TICKS);
static void writeToSDCardTaskFn(void)
{
    Serial.println("Writing averages to SD card");
    uint8_t field = 0;
    uint8_t i, j;
    char buffer[10];

    openDataFileForToday();

    if (s_fileHandle != INVALID_HANDLE)
    {
        for (i = 0; i < STORE_TO_SD_INTERVAL_SECS; ++i)
        {
            writeTimestampToOpenFile();
            writeEntryIDToOpenFile();
            
            for (j = 0; j < FIELD_COUNT; ++j)
            {
                // Write from datafield to buffer then from buffer to SD file
                s_dataFields[j].getDataAsString(buffer, "%.4f", i);

                s_sdCard->write(s_fileHandle, buffer);

                if (!lastinloop(j, FIELD_COUNT))
                {
                    s_sdCard->write(s_fileHandle, ", ");
                }
            }
            s_sdCard->write(s_fileHandle, "\n");
        }

        s_sdCard->closeFile(s_fileHandle);
    }
    else
    {
        Serial.print("Could not open '");
        Serial.print(Filename_get());
        Serial.println("' when trying to write data.");
    }
}

TaskAction readFromADCsTask(readFromADCsTaskFn, MS_PER_ADC_READ, INFINITE_TICKS);
void readFromADCsTaskFn(void)
{
    s_adcReadCount++;
    uint8_t adc = 0;
    uint8_t ch = 0;
    uint8_t field = 0;
    for (adc = 0; adc < 3; adc++)
    {
        for (ch = 0; ch < 4; ch++)
        {
            field = (adc*4)+ch;
            s_averagers[field].newData(s_ADCs[adc].readADC_SingleEnded(ch));
        }
    }
}

static void openDataFileForToday(void)
{

    Time_GetTime(&s_time, TIME_PLATFORM);
    Filename_setFromDate(s_time.tm_mday, s_time.tm_mon, s_time.tm_year);

    char const * const pFilename = Filename_get();

    Serial.print("CSV file: ");
    Serial.println(pFilename);

    if (s_sdCard->fileExists(pFilename))
    {
        s_fileHandle = s_sdCard->openFile(pFilename, true);
    }
    else
    {
        s_fileHandle = s_sdCard->openFile(pFilename, true);

        // Write Timestamp and Entry ID headers
        s_sdCard->write(s_fileHandle, "Timestamp, Entry ID, ");

        char csvHeaders[200];
        DataField_writeHeadersToBuffer(csvHeaders, s_dataFields, FIELD_COUNT, 200);
        s_sdCard->write(s_fileHandle, csvHeaders);
        s_entryID = 0;
    }
}

void setup()
{
    // setup Serial port
    Serial.begin(115200);

    delay(10000);

    uint8_t i = 0;
    for (i = 0; i < 3; i++)
    {
        s_ADCs[i].begin();
        s_ADCs[i].setGain(GAIN_ONE);

    }

    s_sdCard = LocalStorage_GetLocalStorageInterface(LINKITONE_SD_CARD);
    s_sdCard->setEcho(true);

    Time_GetTime(&s_lastSDTimestamp, TIME_PLATFORM);

}

void loop()
{
    readFromADCsTask.tick();
    averageAndStoreTask.tick();
    writeToSDCardTask.tick();
}
