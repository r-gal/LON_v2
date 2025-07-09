#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#if LON_USE_SENSORS_DATABASE == 1

#include "LonDataDef.hpp"
#include "SignalList.hpp"

#include "LonSensorsDatabaseProcess.hpp"

#if LON_USE_HTTP_INTERFACE == 1
#include "HTTP_ServerProcess.hpp"
#include "LonHttpInterface.hpp"
#endif

#include "FileSystem.hpp"
#include "TimeClass.hpp"
#include "LonDevice.hpp"

CommandLonSensorsDatabase_c commandLonSensorsDatabase;

LonSensorsDatabaseProcess_c::LonSensorsDatabaseProcess_c(uint16_t stackSize, uint8_t priority, uint8_t queueSize, HANDLERS_et procId) : process_c(stackSize,priority,queueSize,procId,"SENS_DATA")
{
  for(int i=0;i< SENSOR_DATA_ARRRAY_SIZE;i++)
  {
    sensorDataArray[i].lAdr = 0xFFFFFFFF;
    sensorDataArray[i].lastIdx = 0xFFFF;
  }

}

void LonSensorsDatabaseProcess_c::main(void)
{
  #if LON_USE_HTTP_INTERFACE == 1
  LonHttpInterface_c httpPostInterface;
  HttpClientProcess_c::AssignPostHandler(&httpPostInterface);
  #endif

  //vTaskDelay(1000);

  while(1)
  {
    releaseSig = true;
    RecSig();
    //RestartData_c::ownPtr->signalsHandled[HANDLE_TRAFFIC]++;
    uint8_t sigNo = recSig_p->GetSigNo();
    switch(sigNo)
    {
      case SIGNO_LON_SENSOR_DATA:
      HandleSensorData((LonSensorData_c*)recSig_p);
      break;

      case SIGNO_LON_GET_SENSOR_HISTORY:
      HandleSensorHistory((LonGetSensorsHistory_c*)recSig_p);
      releaseSig = false;
      break;
      case SIGNO_LON_GET_SENSOR_STATS:
      GetSensorsStatistics((LonGetSensorsStats_c*)recSig_p);
      releaseSig = false;
      break;
      default:
      break;



    }
    if(releaseSig) { delete  recSig_p; }
 
  }


}


void LonSensorsDatabaseProcess_c::HandleSensorData(LonSensorData_c* sig_p)
{
  uint8_t* fileBuffer;
  uint16_t fileBufferSize;
  uint16_t fileLength;

  if(sig_p->type > 3)
  {
    return;
  }

  GetFileBufferSize(sig_p->type, &fileBufferSize, &fileLength,  nullptr);


  SystemTime_st time;
  if(TimeUnit_c::GetSystemTime(&time  ))
  {
    uint16_t timeIdx = (time.Minute / 5) + (12*time.Hour);

    if(CheckIfStore(sig_p->lAdr,sig_p->port,timeIdx))
    {

      fileBuffer = new uint8_t[fileBufferSize];
      float* floatPtr = (float*) fileBuffer;

      char* fileName = new char[128];

      uint32_t days = TimeUnit_c::MkDay(&time);

      GetFileName(sig_p->type,days,sig_p->lAdr,sig_p->port,fileName);

      File_c* currentFile = FileSystem_c::OpenFile( fileName, "r+" );

      if(currentFile == nullptr)
      {
        currentFile = FileSystem_c::OpenFile( fileName, "rw" );
        if(currentFile != nullptr)
        {
          /* file has been created now, initiate them */
          for(int i=0;i< fileLength;i++) {floatPtr[i] = -100.0;}        
        }
      }
      else
      {
        currentFile->Read(fileBuffer,fileBufferSize);
        currentFile->Seek(0,File_c::SEEK_MODE_SET);
      }
      delete[] fileName;

      if(currentFile != nullptr)
      {
      

        if(sig_p->type == 0) /* higro*/
        {
           if(sig_p->port == 0)
          {
            floatPtr[timeIdx] = sig_p->data[0];
            floatPtr[timeIdx + 288] = sig_p->data[1];
          }
          else if(sig_p->port == 1)
          {
            floatPtr[timeIdx + 2*288] = sig_p->data[0];
            floatPtr[timeIdx + 3*288] = sig_p->data[1];
          }       
        }
        else if(sig_p->type == 1) /* press */
        {
          floatPtr[timeIdx] = sig_p->data[0];
        }
        else if(sig_p->type == 2) /* powerData */
        {
          for(int i=0;i<14;i++)
          {
             floatPtr[(timeIdx*14) + i] = sig_p->data[i];
          }
        }
        else if(sig_p->type == 3) /* rain */
        {
          floatPtr[timeIdx] = sig_p->data[0];
        }

        currentFile->Write(fileBuffer,fileLength*4);
        currentFile->Close();
      }
      delete[] fileBuffer;
    }
  }

  
}

bool LonSensorsDatabaseProcess_c::CheckIfStore(uint32_t lAdr,uint8_t port, uint16_t timeIdx)
{
  int firstFreeIdx = -1;
  int idx = -1;

  for(int i=0;i< SENSOR_DATA_ARRRAY_SIZE; i++)
  {
    if((lAdr == sensorDataArray[i].lAdr) && (port == sensorDataArray[i].port ))
    {
      idx = i;
      break;
    }

    if((0xFFFFFFFF == sensorDataArray[i].lAdr) && (firstFreeIdx == -1))
    {
      firstFreeIdx = i;
    }
  }

  if((idx == -1) && (firstFreeIdx != -1))
  {
    idx = firstFreeIdx;
    sensorDataArray[idx].lAdr = lAdr;
    sensorDataArray[idx].port = port;
  }

  if(idx != -1)
  {
    if(sensorDataArray[idx].lastIdx != timeIdx)
    {
      sensorDataArray[idx].lastIdx = timeIdx;
      return true;
    }
    else
    {
      return false;
    }

  }
  else
  {
    return true; /* in case full array */
  }



}



void LonSensorsDatabaseProcess_c::HandleSensorHistory(LonGetSensorsHistory_c* recSig_p)
{

  //printf("sensors database, HandleSensorHistory, type=%d, ladr=0x%08X, port=%d\n",recSig_p->type, recSig_p->lAdr, recSig_p->port[0]);
  uint16_t fileBufferSize;
  bool use2ndBuffer;

  GetFileBufferSize(recSig_p->type, &fileBufferSize, nullptr,  &use2ndBuffer);

  SystemTime_st time;
  if(TimeUnit_c::GetSystemTime(&time  ))
  {
    int timeIdxStep = 1;

    uint8_t* fileBuffer = new uint8_t[fileBufferSize];
    float* floatPtr = (float*) fileBuffer;

    int  timeIdx = (time.Minute / 5) + (12*time.Hour);

    uint32_t days = TimeUnit_c::MkDay(&time);


    recSig_p->timestampStep = timeIdxStep * 5*60 ;
    recSig_p->min1 = -100.0;
    recSig_p->max1 = -100.0;
    recSig_p->min2 = -100.0;
    recSig_p->max2 = -100.0;

    recSig_p->probesArray1 = new float[recSig_p->probesCount];
    if(use2ndBuffer)
    {
      recSig_p->probesArray2 = new float[recSig_p->probesCount];
    }

    uint8_t fileStatus = 0; /* 0- not loaded, 1 - loaded, 2 - notexists */
    uint32_t fileIdx = 0;

    char* fileName = new char[128];

    File_c* currentFile = nullptr;

    for(int i = recSig_p->probesCount-1;i>=0;i--)
    {
      if(fileIdx != days )
      {
        if(currentFile != nullptr)
        {
          currentFile->Close();
          currentFile = nullptr;          
        }
        fileStatus = 0;
      }

      if(fileStatus == 0)
      {
        GetFileName(recSig_p->type,days,recSig_p->lAdr,recSig_p->port[0],fileName);

        currentFile = FileSystem_c::OpenFile( fileName, "r" );
        fileIdx = days;
        if(currentFile != nullptr)
        {
          fileStatus = 1;
          currentFile->Read(fileBuffer,fileBufferSize);
        }
        else
        {
          fileStatus = 2;
        }
      }

      float probe1;
      float probe2;

      if(fileStatus == 2)
      {
        probe1 = -100.0;
        probe2 = -100.0;
      }
      else
      {
        if(recSig_p->type == 0) 
        {
          if(recSig_p->port[0] == 0)
          {
            probe1 = floatPtr[timeIdx];
            probe2 = floatPtr[timeIdx+288];
          }
          else if(recSig_p->port[0] == 1)
          {
            probe1 = floatPtr[timeIdx+2*288];
            probe2 = floatPtr[timeIdx+3*288];
          }
        }
        else if (recSig_p->type == 1)
        {
          probe1 = floatPtr[timeIdx];
        } 
        else if(recSig_p->type == 2)
        {
          probe1 = floatPtr[timeIdx+(recSig_p->port[0]*288)];
          probe2 = floatPtr[timeIdx+(recSig_p->port[1]*288)];
        } 
        else if(recSig_p->type == 3) 
        {
          probe1 = floatPtr[timeIdx];
        }    
      }

      if(recSig_p->probesArray1!= nullptr)
      {
        recSig_p->probesArray1[i] = probe1;
        if(probe1 > -64.0)
        {
          if(recSig_p->min1 > probe1 || recSig_p->min1 < -64.0) { recSig_p->min1 = probe1; }
          if(recSig_p->max1 < probe1 || recSig_p->max1 < -64.0) { recSig_p->max1 = probe1; }
        }
      }

      if(recSig_p->probesArray2!= nullptr)
      {
        recSig_p->probesArray2[i] = probe2;
        if(probe2 > -64.0)
        {
          if(recSig_p->min2 > probe2 || recSig_p->min2 < -64.0) { recSig_p->min2 = probe2; }
          if(recSig_p->max2 < probe2 || recSig_p->max2 < -64.0) { recSig_p->max2 = probe2; }
        }
      }


      timeIdx -= timeIdxStep;
      if(timeIdx < 0)
      {
        days--;
        timeIdx += 288;
      }
    }
    recSig_p->startTimestamp = (days * (24*60*60)) + (timeIdx * 5*60 );

    if(currentFile != nullptr) 
    {
      currentFile->Close();
    }

    delete[] fileName;
    delete[] fileBuffer;
  }


  xTaskNotifyGive(recSig_p->task);
}



void LonSensorsDatabaseProcess_c::GetFileBufferSize(int type, uint16_t* bufferSize_p, uint16_t* fileSize_p, bool* use2ndBuffer_p)
{
  uint16_t fileBufferSize;
  uint16_t fileSize;
  bool  use2ndBuffer;
  switch(type)
  {
    case 0: /*higro */
      fileBufferSize = 10*512;
      fileSize = 12*24*2*2;
      use2ndBuffer = true;
    break;

    case 2: /* power */
      fileBufferSize = 32*512;
      fileSize = 12*24*14;
      use2ndBuffer = true;
      break;

    case 1: /*pressure */
    case 3: /* rain */
    default:
      fileBufferSize = 3*512; 
      fileSize = 12*24;
      use2ndBuffer = false;
      break;
  }
  if(bufferSize_p != nullptr) { *bufferSize_p = fileBufferSize; }
  if(fileSize_p != nullptr) { *fileSize_p = fileSize; }
  if(use2ndBuffer_p != nullptr) { *use2ndBuffer_p = use2ndBuffer; }
}

void LonSensorsDatabaseProcess_c::GetFileName(int type, int day, uint32_t lAdr, uint8_t port, char* nameBuf)
{
  int dayPrefix = day/100;
  dayPrefix = dayPrefix*100;

  switch(type)
  {
    case 0: /* higro */
    case 1: /* press */
      snprintf(nameBuf,126,"sensors/%d/%d/%08X.LOG",dayPrefix,day,lAdr);
    break;

    case 2: /* power */
      snprintf(nameBuf,126,"sensors/%d/%d/POWER.LOG",dayPrefix,day);
      break;

    case 3: /* rain */
      snprintf(nameBuf,126,"sensors/%d/%d/RAIN%d.LOG",dayPrefix,day,port);
      break;
    default:
      strcpy(nameBuf, "");

  }

}

#define FLOAT_NOT_VALID (-100.0)

bool LonSensorsDatabaseProcess_c::GetSensorsDayStatistics(int day, int type, uint32_t lAdr, int port, int subport, STATS_DATA_st* buffer, STATS_MODE_et mode,bool detailed)
{
  uint16_t fileBufferSize;
  //bool use2ndBuffer;
  char* fileName = new char[128];
  File_c* currentFile = nullptr;
  uint8_t* fileBuffer;

  bool result = false;

  GetFileBufferSize(type, &fileBufferSize, nullptr, nullptr);

  GetFileName(type,day,lAdr,port,fileName);

  currentFile = FileSystem_c::OpenFile( fileName, "r" );
  if(currentFile != nullptr)
  {
    fileBuffer = new uint8_t[fileBufferSize];
    currentFile->Read(fileBuffer,fileBufferSize);

    float* floatPtr = (float*) fileBuffer;

    float sum = 0;
    float min = 0;
    float max = 0;

    int offset = (2*288 * port) + (288* subport);

    int i_max,h_max;
    if(detailed)
    {
       h_max = 24;
       i_max = 12;
    }
    else
    {
      h_max = 1;
      i_max = 288;
    }

    for(int h=0;h<h_max;h++)
    {
      int hoffset = h*12;

      min = FLOAT_NOT_VALID;
      max = FLOAT_NOT_VALID;
      sum = 0;
      int validProbes = 0;

      for(int i=0; i< i_max ;i++)
      {
        if(floatPtr[offset+i+hoffset] != FLOAT_NOT_VALID)
        {
          if((min == FLOAT_NOT_VALID) ||  (min > floatPtr[offset+i+hoffset]))
          {
            min = floatPtr[offset+i+hoffset]; 
          }
          if((max == FLOAT_NOT_VALID) ||  (max < floatPtr[offset+i+hoffset]))
          {
            max = floatPtr[offset+i+hoffset];
          }
          validProbes++;
          sum += floatPtr[offset+i+hoffset];
        }

        
        
      }

      buffer[h].min = min;
      buffer[h].max = max;
      if(mode == SENSORS_STATS_AVG)
      {
        if(validProbes > 0)
        {
          buffer[h].average = sum / validProbes;
        }
        else
        {
          buffer[h].average = FLOAT_NOT_VALID;
        }
      }
      else
      {
        buffer[h].sum = sum;
      }
    }
    result = true;


    currentFile->Close();
    delete[] fileBuffer;
  }



  delete[] fileName;


  return result;
};


void LonSensorsDatabaseProcess_c::GetSensorsStatistics(LonGetSensorsStats_c* sig_p)
{
  STATS_MODE_et mode = SENSORS_STATS_AVG;
  int type = 0;

  if(type == 3)
  {
    mode = SENSORS_STATS_SUM;
  }


  if(sig_p->range == 0) /* one day */
  {
    SystemTime_st time;
    time.Day = sig_p->day;
    time.Month = sig_p->month;
    time.Year = sig_p->year;
    uint32_t days = TimeUnit_c::MkDay(&time); 

    GetSensorsDayStatistics(days,type, sig_p->lAdr,sig_p->port,sig_p->subport, sig_p->sensorData, mode, true); 

    sig_p->probesValid = 24;

  }
  else /* one month */
  {
    int lastDay = TimeUnit_c::GetMonthLength(sig_p->year, sig_p->month);

    for(int day = 1; day <= lastDay; day++)
    {
      SystemTime_st time;
      time.Day = day;
      time.Month = sig_p->month;
      time.Year = sig_p->year;
      uint32_t days = TimeUnit_c::MkDay(&time); 

      GetSensorsDayStatistics(days,type, sig_p->lAdr,sig_p->port,sig_p->subport, &(sig_p->sensorData[day-1]), mode, false);
    }
    sig_p->probesValid = lastDay;


  }



  xTaskNotifyGive(sig_p->task);
}



comResp_et Com_getsensorhist::Handle(CommandData_st* comData)
{
  SystemTime_st time;

  uint32_t year = 2025;
  uint32_t month = 1;
  uint32_t day = 1;
  if(TimeUnit_c::GetSystemTime(&time  ))
  {
    year = time.Year;
    month = time.Month;
    day = time.Day;
  }


  uint32_t lAdr = 0;
  bool lAdrValid = FetchParameterValue(comData,"LADR",&lAdr,0x10000000, 0xFFFFFFFF);
  uint32_t port = 0;
  bool portValid = FetchParameterValue(comData,"PORT",&port,0,1);

  
  bool yearValid = FetchParameterValue(comData,"YEAR",&year,2000,2100);
  
  bool monthValid = FetchParameterValue(comData,"MONTH",&month,1,12);
  
  bool dayValid = FetchParameterValue(comData,"DAY",&day,1,31);

  uint8_t range = 0;
  bool rangeValid = false;
  int rangeIdx = FetchParameterString(comData,"RANGE");
  if(rangeIdx >= 0)
  {
    rangeValid = true;
    char* paramString = &comData->buffer[comData->argValPos[rangeIdx]];
    if(strcmp(paramString,"DAY") == 0 ) { range = 0; }
    else if(strcmp(paramString,"MONTH") == 0 ) { range = 1; }
    else {rangeValid = false;}
  }

  int type = 0;
  int subport = 0;

  bool typeValid = false;
  int typeIdx = FetchParameterString(comData,"TYPE");
  if(typeIdx >= 0)
  {
    typeValid = true;
    char* paramString = &comData->buffer[comData->argValPos[typeIdx]];
    if(strcmp(paramString,"HIG") == 0 ) { type = 0;  subport = 0;}
    else if(strcmp(paramString,"TEMP") == 0 ) { type  = 0; subport = 1; }
    else if(strcmp(paramString,"PRS") == 0 ) { type  = 1;  }
    else if(strcmp(paramString,"PWR") == 0 ) { type  = 2;  }
    else if(strcmp(paramString,"RAIN") == 0 ) { type  = 3;  }
    else {typeValid = false;}
  }

  bool ok = true;

  if(typeValid == true) 
  {
    if((type == 0) || (type == 1))
    {
      if(lAdrValid == false)
      {
        ok = false;
      }
    }
    if((type == 0) && (portValid == false))
    {
      ok = false;
    }


  }
  else
  {
    ok = false;
  }

  if(ok == false)
  {
    return COMRESP_NOPARAM;

  }


  char* textBuf  = new char[256];

  LonGetSensorsStats_c* sig_p = new LonGetSensorsStats_c;

  sig_p->range = range; /* 0-one day, 1-one month */
  sig_p->day = day;
  sig_p->month = month;
  sig_p->year = year;
  sig_p->lAdr = lAdr;
  sig_p->port = port;
  sig_p->subport = subport;
  sig_p->type = type;

  sig_p->task = xTaskGetCurrentTaskHandle();
  sig_p->Send();

  ulTaskNotifyTake(pdTRUE ,portMAX_DELAY );

  sprintf(textBuf,"Sensor statistics: \n" );
  Print(comData->commandHandler,textBuf);


  for(int i=0;i< sig_p->probesValid;i++)
  {
    sprintf(textBuf,"%2d: min=%.2f max = %.2f %s = %.2f\n",
    range + i, 
    sig_p->sensorData[i].min,
    sig_p->sensorData[i].max,
    type == 3 ? "sum" : "avg",
    sig_p->sensorData[i].average  );
    Print(comData->commandHandler,textBuf);
  }


  delete[] textBuf;

  delete sig_p;

  return COMRESP_OK;

}

#endif