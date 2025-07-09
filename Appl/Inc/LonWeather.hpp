#ifndef WEATHER_UNIT_H
#define WEATHER_UNIT_H

#include "SignalList.hpp"
#include "CommandHandler.hpp"

#define NO_OF_RAIN_SENSORS 1

struct WeatherConfig_st
{
 uint32_t longAdr;    /*B 0-3 */

 uint32_t sensorLadr[4];
 uint32_t sensorPort[4];


};

/*********************** common sensor **************************/

class LonWeatherSensor_c
{
  
  protected:
  float scale;
  

  int actProbes;
  

  bool averageMode;


  float lastHour[12];
  int lastHourIdx;

  float lastHours[48];
  int lastHoursIdx;

  public:
  int idx;

  void Tick(bool fullHourIndicator);

  LonWeatherSensor_c(float defaultValue)
  {
    for(int i=0;i<12;i++)
    {
      lastHour[i] = defaultValue;
    }
    lastHourIdx = 0;

    for(int i=0;i<48;i++)
    {
      lastHours[i] = defaultValue;
    }
    lastHoursIdx = 0;
  }
  void RestoreStats(void);
  void GetStats(LonGetWeatherStats_c* recSig_p);

  float GetStat(int hours);

  virtual float GetActVal(void) = 0;
  virtual void ClearActVal(void) = 0;

};

/*********************** rain sensor **************************/

class LonRainSensor_c: public LonWeatherSensor_c
{
  
  float accRain;

  public:

    LonRainSensor_c(void) : LonWeatherSensor_c(0)
    {
      scale = 0.2794;
      averageMode = false;
      accRain = 0;
      idx = 0;
    }    

    void HandleEvent(void);
    void SendAcumulatedData(void);
   
    float GetActVal(void) { return accRain; }
    void ClearActVal(void) { accRain = 0; actProbes = 0;}

};

class LonStandardSensor_c: public LonWeatherSensor_c
{
  
  float accValue;

  public:

    LonStandardSensor_c(void )  : LonWeatherSensor_c(-100)
    {
      scale = 1;
      averageMode = true;
      accValue = 0;
    }    

    void HandleMeasurement(float val);

    float GetActVal(void) 
    {
      if(actProbes > 0)
      { 
        return accValue / actProbes;
      }
      else
      {
        return -100;
      }
    }

    void ClearActVal(void) { actProbes = 0; accValue = 0; }
    //void SendAcumulatedData(bool fullHourIndicator);
   
    //float GetSum24h(void);
};


class WeatherUnit_c
{

  LonRainSensor_c rainSensor;
  LonStandardSensor_c tempearatureSensor; 
  LonStandardSensor_c humiditySensor;
  LonStandardSensor_c pressureSensor;


  LonWeatherSensor_c* sensor[4];

  public:
  void InitWeather(void);

  float GetRainStat(int hours);
  float GetTempStat(int hours);
  void GetRainStats(LonGetWeatherStats_c* recSig_p); 
  float GetAverageTemperature();

  void Tick(bool fullHourIndicator);

  void HandleEvent(uint32_t lAdr,uint8_t port);
  void HandleSensorData(uint32_t lAdr,uint8_t port,float data1, float data2);




};


/*****************commands *******************/

class Com_weatherstat : public Command_c
{
  public:
  char* GetComString(void) { return (char*)"weatherstat"; }
  void PrintHelp(CommandHandler_c* commandHandler ){}
  comResp_et Handle(CommandData_st* comData_);
};

class Com_weatherconfig : public Command_c
{
  public:
  char* GetComString(void) { return (char*)"weatherconfig"; }
  void PrintHelp(CommandHandler_c* commandHandler ){}
  comResp_et Handle(CommandData_st* comData_);
};

class Com_weatherprint : public Command_c
{
  public:
  char* GetComString(void) { return (char*)"weatherprint"; }
  void PrintHelp(CommandHandler_c* commandHandler ){}
  comResp_et Handle(CommandData_st* comData_);
};

class CommandWeather_c :public CommandGroup_c
{
  Com_weatherstat weatherstat;
  Com_weatherconfig weatherconfig;
  Com_weatherprint weatherprint;
  public:  
};


#endif