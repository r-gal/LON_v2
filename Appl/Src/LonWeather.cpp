#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "LonDatabase.hpp"

#if LON_USE_WEATHER_UNIT == 1

#include "LonWeather.hpp"

CommandWeather_c commandWeather;


float WeatherUnit_c::GetRainStat(int hours)
{
  return rainSensor.GetStat(hours);

}

float WeatherUnit_c::GetTempStat(int hours)
{
  return tempearatureSensor.GetStat(hours);

}

float WeatherUnit_c::GetAverageTemperature()
{

  return 0;
}


void WeatherUnit_c::HandleEvent(uint32_t lAdr,uint8_t port)
{
  LonDatabase_c database;
  WeatherConfig_st* configFile_p = (WeatherConfig_st*)database.ReadConfig(0x00020000);

  if(configFile_p != nullptr)
  {
    if((lAdr == configFile_p->sensorLadr[0] ) && (port == configFile_p->sensorPort[0] ))
    {
      rainSensor.HandleEvent();
    }
  }
  
}

void WeatherUnit_c::HandleSensorData(uint32_t lAdr,uint8_t port,float data1, float data2)
{
  LonDatabase_c database;
  WeatherConfig_st* configFile_p = (WeatherConfig_st*)database.ReadConfig(0x00020000);

  if(configFile_p != nullptr)
  {
    if((lAdr == configFile_p->sensorLadr[1] ) && (port == configFile_p->sensorPort[1] ))
    {
      tempearatureSensor.HandleMeasurement(data2);
    }

    if((lAdr == configFile_p->sensorLadr[2] ) && (port == configFile_p->sensorPort[2] ))
    {
      humiditySensor.HandleMeasurement(data1);
    }

    if((lAdr == configFile_p->sensorLadr[3] ) && (port == configFile_p->sensorPort[3] ))
    {
      pressureSensor.HandleMeasurement(data1);
    }
  }

}

void WeatherUnit_c::Tick(bool fullHourIndicator)
{

  rainSensor.SendAcumulatedData();

  for(int i=0;i<4; i++)
  {
    sensor[i]->Tick(fullHourIndicator);
  }

  
}

void WeatherUnit_c::GetRainStats(LonGetWeatherStats_c* recSig_p)
{
  sensor[recSig_p->idx]->GetStats(recSig_p);
}

void WeatherUnit_c::InitWeather(void)
{
  tempearatureSensor.idx = 1;
  humiditySensor.idx = 2;
  pressureSensor.idx = 3;

  sensor[0] = &rainSensor;
  sensor[1] = &tempearatureSensor;
  sensor[2] = &humiditySensor;
  sensor[3] = &pressureSensor;

  for(int i=0;i<4; i++)
  {
    sensor[i]->RestoreStats();
  }
}

/*********************** common sensor **************************/


void LonWeatherSensor_c::Tick(bool fullHourIndicator)
{
  lastHour[lastHourIdx] = GetActVal();
  lastHourIdx++;
  if(lastHourIdx >= 12) { lastHourIdx = 0; }

  if(fullHourIndicator)
  {  
    float hourSum = 0;
    int cnt = 0;

    for(int i=0;i<12;i++)
    {
      if(lastHour[i] > -64)
      {
        hourSum += lastHour[i];
        cnt++;
      }
    }

    if(cnt > 0)
    {
      if (averageMode == true)
      {
        hourSum /= cnt;
      }
    }
    else
    {
      if (averageMode == true)
      {
        hourSum = -100;
      }
      else
      {
        hourSum = 0;
      }
    }

    lastHours[lastHoursIdx] = hourSum;
    lastHoursIdx++;
    if(lastHoursIdx >= 12) { lastHoursIdx = 0; }
  }
  ClearActVal();
}

void LonWeatherSensor_c::GetStats(LonGetWeatherStats_c* recSig_p)
{

  recSig_p->averageMode = averageMode;

  recSig_p->current = GetActVal();;

  int idx = lastHourIdx;
  for(int i=0;i<12;i++)
  {
    idx--;
    if(idx <0) { idx = 11; }

    recSig_p->lastHour[i] = lastHour[idx];
  }

  idx = lastHoursIdx;
  for(int i=0;i<48;i++)
  {
    idx--;
    if(idx <0) { idx = 47; }

    recSig_p->lastHours[i] = lastHours[idx];
  }

  recSig_p->moreSensors = (recSig_p->idx+1 < NO_OF_RAIN_SENSORS);

  xTaskNotifyGive(recSig_p->task);
}

void LonWeatherSensor_c::RestoreStats(void)
{
  LonDatabase_c database;
  WeatherConfig_st* config_p = (WeatherConfig_st*)database.ReadConfig(0x00020000);
  if(config_p == nullptr)
  {
    return;
  }

  LonGetSensorsHistory_c* sig_p = new LonGetSensorsHistory_c();

  if(idx == 0)
  {
    sig_p->lAdr = 0;
    sig_p->port[0] = 0;
    sig_p->port[1] = 0;
    sig_p->type = 3; /* rain */
  }
  else
  {
    sig_p->lAdr = config_p->sensorLadr[idx];
    sig_p->port[0] = 0;
    sig_p->port[1] = 0;
    if(idx == 3)
    {
      sig_p->type = 1; /*press*/
    }
    else
    {
      sig_p->type = 0; /* temp/higro */
    }
  }

  sig_p->probesCount = 288 * 2;
  sig_p->probesArray1 = nullptr;
  sig_p->probesArray2 = nullptr;
  sig_p->task = xTaskGetCurrentTaskHandle();
  sig_p->Send();

  ulTaskNotifyTake(pdTRUE ,portMAX_DELAY );

  float* probesArray;

  if(idx == 1)
  {
    probesArray = sig_p->probesArray2;
  }
  else
  {
    probesArray = sig_p->probesArray1;
  }

  /* restore hours stats */
  for(int h = 0;h<48;h++)
  {
    float val = 0;
    int cnt = 0;
    
    for(int i=0;i<12;i++)
    {
      float v =  probesArray[i + 12*h];

      if(v > -64.0)
      {
        val += v;
        cnt++;
      }
      
    }
    if(cnt > 0)
    {
      if (averageMode == true)
      {
        val /= cnt;
      }
    }
    else
    {
      if (averageMode == true)
      {
        val = -100;
      }
      else
      {
        val = 0;
      }
    }


    lastHours[h] = val;
  }
  lastHoursIdx = 0;

  /* restore hour stats */
  for(int i = 0;i<12;i++)
  {
    lastHour[i] = probesArray[ 47*12 + i];
    if((averageMode == false) && (lastHour[i]  < -64))
    {
      lastHour[i] = 0;
    }
  }
  lastHourIdx = 0;

  delete[] sig_p->probesArray1;

  if(sig_p->probesArray2 != nullptr)
  {
     delete[] sig_p->probesArray2;
  }
  delete sig_p;
}

float LonWeatherSensor_c::GetStat(int hours)
{
    float sum = 0;
    int noOfProbes = 0;
    int idx = lastHoursIdx;
    for(int i=0;i<hours;i++)
    {
      idx--;
      if(idx <0) { idx = 47; }

      if(lastHours[idx] > -64)
      {
        noOfProbes++;
        sum += lastHours[idx];
      }
    }

    if(averageMode == true) 
    {
      if(noOfProbes > 0 )
      {
        return sum / noOfProbes;
      }
      else
      {
        return -100;
      }
    }
    else
    {
      return sum;
    }

}

/*********************** rain sensor **************************/

LonRainSensor_c rainSensor[NO_OF_RAIN_SENSORS];

void LonRainSensor_c::HandleEvent(void)
{
  accRain += scale;
}

void LonRainSensor_c::SendAcumulatedData(void )
{
  #if LON_USE_SENSORS_DATABASE == 1
  LonSensorData_c* sig_p = new LonSensorData_c;
  sig_p->lAdr = 0;
  sig_p->port = 0;
  sig_p->data[0] = accRain;
  sig_p->type = 3;
  sig_p->Send();
  #endif
 





}






/*********************** standard sensor **************************/

void LonStandardSensor_c::HandleMeasurement(float val)
{
  if(val > -64)
  {
    accValue += val;
    actProbes++;
  }

}




/****************commands**************************/

const char* weatherParamString[] = {"RAIN","TEMP","HUM","PRESS"};
const char* weatherUnitString[] = {"mm","C","%","hPa"};

comResp_et Com_weatherstat::Handle(CommandData_st* comData)
{

  uint8_t ch = 0;
  bool chValid = false;
  int chIdx = FetchParameterString(comData,"PARAM");
  if(chIdx >= 0)
  {
    
    char* paramString = &comData->buffer[comData->argValPos[chIdx]];
    for(int i=0;i<4;i++)
    {
      if(strcmp(paramString,weatherParamString[i]) == 0 )
      { 
       ch = i;
       chValid = true;
       break;
      }
    }
  }

  if(chValid == true)
  {
    char* textBuf  = new char[256];

    LonGetWeatherStats_c* sig_p = new LonGetWeatherStats_c;
    sig_p->idx = ch;
    sig_p->task = xTaskGetCurrentTaskHandle();
    sig_p->Send();

    ulTaskNotifyTake(pdTRUE ,portMAX_DELAY );


    float lastHour = 0;
    float last24Hours = 0;
    float last48Hours = 0;

    int lastHourProbes = 0;
    int last24HoursProbes = 0;
    int last48HoursProbes = 0;

    for(int idx=0; idx<12;idx++)
    {
      if(sig_p->lastHour[idx] > -64)
      {
        lastHour += sig_p->lastHour[idx] ;
        lastHourProbes++;
      }
    }


    for(int idx=0; idx<24;idx++)
    {
      if(sig_p->lastHours[idx] > -64)
      {
        last24Hours += sig_p->lastHours[idx] ;
        last24HoursProbes++;
      }
      

      if(sig_p->lastHours[idx+24] > -64)
      {
        last48Hours += sig_p->lastHours[idx+24] ;
        last48HoursProbes++;
      }
    }
    last48Hours += last24Hours;
    last48HoursProbes += last24HoursProbes;

    if(sig_p->averageMode)
    {
      if(lastHourProbes> 0)
      {
        lastHour /= lastHourProbes;
      }
      if(last24HoursProbes> 0)
      {
        last24Hours /= last24HoursProbes;
      }
      if(last48HoursProbes> 0)
      {
        last48Hours /= last48HoursProbes;
      }
    }

    sprintf(textBuf," %s sensor data \n",weatherParamString[ch]);
    Print(comData->commandHandler,textBuf);

    sprintf(textBuf,"Act: %.2f%s LastHour: %.2f%s, last 24H: %.2f%s last 48H: %.2f%s \n",
     sig_p->current,
     weatherUnitString[ch],
     lastHour,
     weatherUnitString[ch],
     last24Hours,
     weatherUnitString[ch],
     last48Hours,
     weatherUnitString[ch]
   
    );
    Print(comData->commandHandler,textBuf);

    sprintf(textBuf,"Last Hour: \n" );
    Print(comData->commandHandler,textBuf);

    for(int idx=0; idx<12;idx++)
    {
      sprintf(textBuf,"%d min: %.2f%s\n", 5*idx,  sig_p->lastHour[idx] ,weatherUnitString[ch]);
      Print(comData->commandHandler,textBuf);
    }
    sprintf(textBuf,"Last 48 Hours: \n" );
    Print(comData->commandHandler,textBuf);

    for(int idx=0; idx<48;idx++)
    {
      sprintf(textBuf,"%d : %.2f%s\n", idx,  sig_p->lastHours[idx],weatherUnitString[ch]);
      Print(comData->commandHandler,textBuf);
    }


    delete[] textBuf;

    delete sig_p;
  }

  return COMRESP_OK;

}



comResp_et Com_weatherconfig::Handle(CommandData_st* comData)
{

  uint8_t ch = 0;
  bool chValid = false;
  int chIdx = FetchParameterString(comData,"PARAM");
  if(chIdx >= 0)
  {
    
    char* paramString = &comData->buffer[comData->argValPos[chIdx]];
    for(int i=0;i<4;i++)
    {
      if(strcmp(paramString,weatherParamString[i]) == 0 )
      { 
       ch = i;
       chValid = true;
       break;
      }
    }
  }

  uint32_t lAdr;
  bool lAdrValid = FetchParameterValue(comData,"LADR",&lAdr,0, 0xFFFFFFFF);
  if((lAdr >0 ) && (lAdr < 0x10000000)) {lAdrValid = false; }
  uint32_t port;
  bool portValid = FetchParameterValue(comData,"PORT",&port,0,7);


  bool allOk = false;
  if(chValid == true)
  {
    allOk = lAdrValid| portValid;


  }

  uint32_t lAdrConf = 0x00020000;
  if(allOk)
  {
    LonDatabase_c database;

    WeatherConfig_st* config_p = new WeatherConfig_st;
    WeatherConfig_st* orgConfig_p = (WeatherConfig_st*)database.ReadConfig(lAdrConf);
    if(orgConfig_p == NULL)
    {
      memset(config_p,0,sizeof(WeatherConfig_st));
      config_p->longAdr = lAdrConf;
    }
    else
    {
      memcpy(config_p,orgConfig_p,sizeof(WeatherConfig_st));
    }

    if(lAdrValid) config_p->sensorLadr[ch] = lAdr;
    if(portValid) config_p->sensorPort[ch] = port;

     database.SaveConfig((LonConfigPage_st*)config_p);
     /*LonIrrigConfUpdate_c* sig_p = new LonIrrigConfUpdate_c;
     sig_p->Send();*/
  }

  return COMRESP_OK;

}


comResp_et Com_weatherprint::Handle(CommandData_st* comData)
{
  char* textBuf  = new char[256];

  LonDatabase_c database;
  WeatherConfig_st* configFile_p = (WeatherConfig_st*)database.ReadConfig(0x00020000);

  if(configFile_p != nullptr)
  {
    for(int i=0;i<4;i++)
    {
      sprintf(textBuf,"Input device for %s: LADR = 0x%08X PORT = %d \n", weatherParamString[i], configFile_p->sensorLadr[i] , configFile_p->sensorPort[i]);
      Print(comData->commandHandler,textBuf);
    }
  }

  return COMRESP_OK;

}




#endif