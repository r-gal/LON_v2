#ifndef LONSIGNALS_H
#define LONSIGNALS_H

#include "LonDataDef.hpp"
#include "CommonSignal.hpp"

#include "TimeClass.hpp"

/****************** LON SIGNALS ************************/

class LonInterfaceTick_c : public Sig_c
{
  public:
  LonInterfaceTick_c(void) : Sig_c(SIGNO_LON_INTERFACE_TICK,HANDLE_LON_INTERFACE) {}
};

class LonDevCheckTick_c : public Sig_c
{
  public:
  LonDevCheckTick_c(void) : Sig_c(SIGNO_LON_DEVCHECK_TICK,HANDLE_LON_TRAFFIC) {}

};

class LonTmoScanTick_c : public Sig_c
{
  public:
  LonTmoScanTick_c(void) : Sig_c(SIGNO_LON_TMOSCAN_TICK,HANDLE_LON_TRAFFIC) {}


};

class LonTime_c : public Sig_c
{
  public:
  LonTime_c(void) : Sig_c(SIGNO_LON_TIME,HANDLE_LON_TRAFFIC) {}
  SystemTime_st time;
};

class LonIOdata_c : public Sig_c
{
  uint8_t chNo;
  public:

  uint8_t additionalInfo;
  uint8_t data[MAX_MESSAGE_SIZE];
  uint8_t ack[MAX_ACK_SIZE];

  uint8_t usedIndicator;
  uint8_t triesCount ;
  
  IOact_et ackOk;

  LonIOdata_c(void) : Sig_c(SIGNO_LON_IODATAOUT,HANDLE_LON_TRAFFIC) 
  {
    usedIndicator = 0;
    triesCount = 3;
  }


  void translate(SIGNO_et newSigNo)
  {
    switch(newSigNo)
    {
      case SIGNO_LON_IODATAOUT:
        sigNo = newSigNo;
        if(chNo<NO_OF_BASIC_CHANNELS)
        {
          handler = (HANDLERS_et)(HANDLE_LON_PHY_0 + chNo);
        }
        else
        {
          handler = HANDLE_LON_PHY_0;
        }
        break;

      case SIGNO_LON_IODATAIN_TRAF:        
        sigNo = newSigNo;
        handler = HANDLE_LON_TRAFFIC;
        break;
      case SIGNO_LON_IODATAIN_CTRL:        
        sigNo = newSigNo;
        handler = HANDLE_LON_TRAFFIC; /*handler = HANDLE_CTRL;*/
        break;
      default:
        break;
    }
  }
  void SetChNo(uint8_t chNo_in)
  {
    if(chNo_in<NO_OF_BASIC_CHANNELS)
    {
      chNo = chNo_in;
    }
    else
    {
      chNo = 0;
    }
    if(sigNo == SIGNO_LON_IODATAOUT)
    {
      handler = (HANDLERS_et)(HANDLE_LON_PHY_0 + chNo);
    }
  }
  uint8_t GetChNo(void) { return chNo; }

  void calculateCS(uint8_t count)
  {
    uint8_t cS= 0;
    for(uint8_t i = 0; i<count;i++)
    {
      cS ^= data[i];
    }
    data[count] = cS;
  }

  bool checkCS(uint8_t count)
  {
    uint8_t cS= 0;
    for(uint8_t i = 0; i<count;i++)
    {
      cS ^= data[i];
    }
    return (cS == 0);
  }

  IOact_et checkACKCS(uint8_t count)
  {
    uint8_t cS= 0;
    bool allFF = true;
    for(uint8_t i = 0; i<count;i++)
    {
      cS ^= ack[i];
      if(ack[i] != 0xFF) { allFF=false; }
    }

    if(allFF == true) { return IOACK_NORESP; }

    cS = (cS & 0x0F) ^ ((cS&0xF0)>>4);

    return (cS == 0) ? IOACK_OK : IOACK_NOK;
  }

  static uint8_t getAckLen(frameCode_et frameType)
  {
    if( frameType >= FRAME_NO_OF_TYPES)
    {
      return 1;
    } 
    else
    {
      return ackSize[frameType];
    }
  }

};

#if LON_USE_COMMAND_LINK == 1

class LonSetOutput_c : public Sig_c
{
  public:
  LonSetOutput_c(void) : Sig_c(SIGNO_LON_SET_OUTPUT,HANDLE_LON_TRAFFIC) {}

  uint32_t lAdr;
  TaskHandle_t task;
  uint8_t port;
  uint8_t value;

};

class LonGetOutput_c : public Sig_c
{
  public:
  LonGetOutput_c(void) : Sig_c(SIGNO_LON_GET_OUTPUT,HANDLE_LON_TRAFFIC) {}

  uint32_t lAdr;
  TaskHandle_t task;
  uint8_t port;

};

class LonDeviceConfig_c : public Sig_c
{
  public:
  LonDeviceConfig_c(void) : Sig_c(SIGNO_LON_DEVICECONFIG,HANDLE_LON_TRAFFIC) {}

  uint32_t lAdr;
  TaskHandle_t task;

};


class LonGetDevList_c: public Sig_c
{
  public:
  LonGetDevList_c(void) : Sig_c(SIGNO_LON_GETDEVLIST,HANDLE_LON_TRAFFIC) {}
  uint8_t chNo;
  TaskHandle_t task;

  uint8_t count;
  uint32_t lAdr[NO_OF_DEV_IN_CHANNEL];
  uint8_t sAdr[NO_OF_DEV_IN_CHANNEL];
  uint8_t status[NO_OF_DEV_IN_CHANNEL];
  bool moreData;
  bool chValid;

};

class LonGetDevInfo_c: public Sig_c
{
  public:
  LonGetDevInfo_c(void) : Sig_c(SIGNO_LON_GETDEVINFO,HANDLE_LON_TRAFFIC) {}
  uint32_t lAdr;
  TaskHandle_t task;
  bool result;
  uint8_t sAdr;
  uint8_t status;
  float value[8];
  float value2[8];
};


#endif

#if CONF_USE_TIME == 1

class Lon5MinTick_c: public Sig_c
{
  public:
  Lon5MinTick_c(void) : Sig_c(SIGNO_LON_5_MIN_TICK,HANDLE_LON_TRAFFIC) {}
  bool fullHourIndicator;

};
#endif


#if LON_USE_SENSORS == 1

struct SensorValues_st
{
  uint32_t lAdr;
  uint8_t port;
  uint8_t valid;
  uint16_t spare;
  float value1;
  float value2;
};

#if LON_USE_WEATHER_UNIT == 1
class LonGetWeatherStats_c: public Sig_c
{
  public:
  LonGetWeatherStats_c(void) : Sig_c(SIGNO_LON_GET_WEATHER_STATS,HANDLE_LON_TRAFFIC) {}

  TaskHandle_t task;
  int idx; /* 0-rain, 1- temeparature, proposed: o2 - humidity, 3-pressure */
  bool moreSensors;

  float scale;
  bool averageMode;

  float current;
  float lastHour[12];
  float lastHours[48];

};
#endif

class LonGetSensorsValues_c : public Sig_c
{
    public:
  LonGetSensorsValues_c(void) : Sig_c(SIGNO_LON_GET_SENSOR_VALUES,HANDLE_LON_TRAFFIC) {}

  uint16_t noOfDevices;
  TaskHandle_t task;

  SensorValues_st* sensorList;
};

#endif

#if LON_USE_SENSORS_DATABASE == 1


class LonSensorData_c : public Sig_c
{
  public:
  LonSensorData_c(void) : Sig_c(SIGNO_LON_SENSOR_DATA,HANDLE_LON_SENSORS_DATABASE) {}

  uint32_t lAdr;
  uint8_t port;
  uint8_t type; /* 0-higro, 1 - press 2- powerData*/
  float data[14];

};





class LonGetSensorsHistory_c: public Sig_c
{
  public:
  LonGetSensorsHistory_c(void) : Sig_c(SIGNO_LON_GET_SENSOR_HISTORY,HANDLE_LON_SENSORS_DATABASE) {}

  TaskHandle_t task;
  uint32_t lAdr;
  uint16_t probesCount;
  uint8_t port[2];
  uint8_t type;
  uint8_t scale;  



  uint32_t timestampStep;
  uint32_t startTimestamp;

  float min1;
  float min2;
  float max1;
  float max2;
  
  float* probesArray1;
  float* probesArray2;
};

struct OutputValues_st
{
  uint32_t lAdr;
  uint16_t validBitmap; /* 0-not valid, 1-relay, 2-pwm */
  uint16_t spare;
  uint8_t values[8];
};

class LonGetOutputsList_c : public Sig_c
{
    public:
  LonGetOutputsList_c(void) : Sig_c(SIGNO_LON_GET_OUTPUTS_LIST,HANDLE_LON_TRAFFIC) {arraySize = 256;}

  uint16_t noOfDevices;
  uint16_t noOfOutputs;
  uint16_t arraySize;
  TaskHandle_t task;

  OutputValues_st outputList[256];
};

class LonGetPowerInfo_c: public Sig_c
{
  public:
  LonGetPowerInfo_c(void) : Sig_c(SIGNO_LON_GET_POWER_INFO,HANDLE_POWER) {}
  TaskHandle_t task;
  float data[14];

};







class LonGetSensorsStats_c: public Sig_c
{
  public:
  LonGetSensorsStats_c(void) : Sig_c(SIGNO_LON_GET_SENSOR_STATS,HANDLE_LON_SENSORS_DATABASE) {}

  TaskHandle_t task;

  STATS_DATA_st sensorData[32];

  uint8_t range; /* 0-one day, 1-one month */
  uint8_t day;
  uint8_t month;
  uint16_t year;
  uint32_t lAdr;
  uint8_t port;
  uint8_t subport;
  uint8_t type;
    
  uint8_t probesValid;
};


#endif

#if LON_USE_IRRIGATION_UNIT == 1
class LonIrrigConfUpdate_c: public Sig_c
{
  public:
  LonIrrigConfUpdate_c(void) : Sig_c(SIGNO_LON_IRRIG_UPD_CONFIG,HANDLE_LON_TRAFFIC) {}
};

class LonIrrigGetstat_c: public Sig_c
{
  public:
  LonIrrigGetstat_c(void) : Sig_c(SIGNO_LON_IRRIG_GETSTAT,HANDLE_LON_TRAFFIC) {}
  TaskHandle_t task;

  int state[IRRIG_NO_OF];
  float usedRain[IRRIG_NO_OF];
  float usedTemp[IRRIG_NO_OF];
  int realStopTime[IRRIG_NO_OF];
};


#endif

#endif