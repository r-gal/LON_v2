#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#if LON_USE_HTTP_INTERFACE == 1

#include "LonDataDef.hpp"
#include "SignalList.hpp"

#include "LonHttpInterface.hpp"
#include "LonSensorsDatabaseProcess.hpp"
#include "LonDatabase.hpp"

void LonHttpInterface_c::HandleCommand(char* line,char* extraData,SocketTcp_c* socket)
{
  char* endSignPtr = strchr(extraData,';');
  if(endSignPtr != nullptr)
  {
    *endSignPtr = ',';
  }

  char* tmpPtr = extraData;
 
  uint32_t argVal;

  char* endPtr = strchr(extraData,';');

  bool res = FetchNextArg(&tmpPtr,&argVal);

  if(res == true)
  {
    switch(argVal)
    {
      case 1:
        HandleGetSensorHistory(tmpPtr,socket);
        break;

      case 2:
        HandleSetRelVal(tmpPtr,socket);
        break;

      case 3:
        HandleGetTimers(tmpPtr,socket);
        break;
      
      case 4:
        HandleSetTimer(tmpPtr,socket);
        break;

      case 5:
        HandleGetSensorsValues(tmpPtr,socket);
        break;

      case 8:
        HandleGetOutputs(tmpPtr,socket,false);
        break;

      case 9:
        HandleGetOutputs(tmpPtr,socket,true);
        break;

      case 10:
        HandleGetPowerInfo(tmpPtr,socket);
        break;

      default:
        SendAnswer(socket,WEB_BAD_REQUEST,nullptr);
        break;
    }
  }
  else
  {
    SendAnswer(socket,WEB_BAD_REQUEST,nullptr);
  }

}

void LonHttpInterface_c::HandleGetSensorsValues(char* argsStr, SocketTcp_c* socket)
{
  //printf("HandleGetSensorsValues\n");

  char* tmpPtr = argsStr;

  uint32_t noOfDevices;
  bool ok = FetchNextArg(&tmpPtr,&noOfDevices);  

  int length = 0;
  char* responseBuffer;

  if(ok && noOfDevices > 0)
  {
    LonGetSensorsValues_c* sig_p = new LonGetSensorsValues_c;
    sig_p->sensorList = new SensorValues_st[noOfDevices];
    sig_p->noOfDevices = noOfDevices;

    for(int i=0;i<noOfDevices;i++)
    {
      uint32_t lAdr;
      uint32_t port;
 
      ok = FetchNextArg(&tmpPtr,&lAdr);
      if(ok == false) { break; }
      ok = FetchNextArg(&tmpPtr,&port);
      if(ok == false) { break; }

      sig_p->sensorList[i].lAdr = lAdr;
      sig_p->sensorList[i].port = port;

    }

    if(ok)
    {

      sig_p->task = xTaskGetCurrentTaskHandle();

      sig_p->Send();

      ulTaskNotifyTake(pdTRUE ,portMAX_DELAY );

      uint16_t bufferLength = 25*(noOfDevices+1);
      responseBuffer = new char[bufferLength];

      sprintf(responseBuffer,"%d,0,0,0",noOfDevices);
      uint16_t totalLength = strlen(responseBuffer);

      for(int i=0;i<noOfDevices;i++)
      {
        char tmpStr[64];
        if(sig_p->sensorList[i].valid == 0)
        {
          sig_p->sensorList[i].value1 = 0;
          sig_p->sensorList[i].value2 = 0;
        }
        snprintf(tmpStr,62,",0x%08X,%d,%.1f,%.1f",sig_p->sensorList[i].lAdr,sig_p->sensorList[i].port,sig_p->sensorList[i].value1,sig_p->sensorList[i].value2);
        tmpStr[63] = 0;
        uint8_t lineLen = strlen(tmpStr);
        if(totalLength + lineLen <  bufferLength)
        {
          strcat(responseBuffer,tmpStr);
          totalLength += lineLen;
        }
        else
        { 
          //printf("buffer limit reached\n");
        }
      }
      length = totalLength;
    }


    delete[] sig_p->sensorList;
    delete sig_p;

    //sprintf(responseBuffer,"4,0,0,0,0xA0010000,1,12,23,0xA0020000,1,12,23,0xA0030000,1,12,23,0xA0040000,1,12,23");

    
  }
  char extraLine[32];
  sprintf(extraLine,"Content-Length: %d\r\n",length);

  SendAnswer(socket,WEB_REPLY_OK,extraLine);

  if(noOfDevices > 0)
  {
  socket->Send((uint8_t*)responseBuffer,strlen(responseBuffer), 0 );
  }

}

void LonHttpInterface_c::HandleGetSensorHistory(char* argsStr,  SocketTcp_c* socket)
{
  //printf("HandleGetSensorHistory\n");

  uint32_t lAdr;
  uint32_t port;
  uint32_t scale;
  uint32_t type;
  uint16_t noOfProbes = 200;

  char* tmpPtr = argsStr;

  bool ok = FetchNextArg(&tmpPtr,&lAdr);
  if(ok)
  {
    ok = FetchNextArg(&tmpPtr,&port);
  }
  if(ok)
  {
    ok = FetchNextArg(&tmpPtr,&scale);
  }
  if(ok)
  {
    ok = FetchNextArg(&tmpPtr,&type);
  }

  if(ok)
  {

    //printf("ladr=%x, %d %d\n",lAdr,port,scale);



    LonGetSensorsHistory_c* sig_p = new LonGetSensorsHistory_c;
    sig_p->lAdr = lAdr;
    sig_p->port[0] = port & 0xFF;
    sig_p->port[1] = (port >> 8)& 0xFF;
    sig_p->scale = scale;
    sig_p->type = type;
    sig_p->probesCount = noOfProbes;
    sig_p->probesArray1 = nullptr;
    sig_p->probesArray2 = nullptr;
    sig_p->task = xTaskGetCurrentTaskHandle();
    sig_p->Send();

    ulTaskNotifyTake(pdTRUE ,portMAX_DELAY );

    char* printBuffer = nullptr;

    uint16_t wantedSize = 0;
    uint8_t noOfSeries = 0;

    if(sig_p->probesArray1 != nullptr)
    { 
      wantedSize = 8*noOfProbes; 
      noOfSeries++;
      if(sig_p->probesArray2 != nullptr)
      { 
        wantedSize += 8*noOfProbes;
        noOfSeries++;
      }
    }
    

    if(wantedSize > 0) 
    { 
      wantedSize += 32;

      printBuffer = new char[wantedSize];
    }

    char tmpStr[32];

    if(printBuffer != nullptr)
    {
      uint16_t bufferUsage = 0;
      uint16_t bufferSize = wantedSize;
      sprintf(printBuffer,"%d,%d,%d,%d,%.1f,%.1f,%.1f,%.1f",noOfSeries,noOfProbes,sig_p->startTimestamp,sig_p->timestampStep,sig_p->min1,sig_p->max1,sig_p->min2,sig_p->max2 );
      bufferUsage += strlen(printBuffer);

      for(int i=0;i<noOfProbes;i++)
      {
        sprintf(tmpStr,",%.1f",sig_p->probesArray1[i]);
        uint8_t s = strlen(tmpStr);
        if(bufferUsage + s  < bufferSize)
        {
          strcpy(printBuffer+bufferUsage,tmpStr);
          bufferUsage +=s;          
        }
        else
        { 
          //printf("buffer limit reached\n");
        }

        
      }

      if(sig_p->probesArray2 != nullptr)
      {
        for(int i=0;i<noOfProbes;i++)
        {
          sprintf(tmpStr,",%.1f",sig_p->probesArray2[i]);
          uint8_t s = strlen(tmpStr);
          if(bufferUsage + s  < bufferSize)
          {
            strcpy(printBuffer+bufferUsage,tmpStr);
            bufferUsage +=s;          
          }
          else
          { 
            //printf("buffer limit reached\n");
          }
        }
      }
    
      char extraLine[32];
      sprintf(extraLine,"Content-Length: %d\r\n",bufferUsage);

      SendAnswer(socket,WEB_REPLY_OK,extraLine);

      socket->Send((uint8_t*)printBuffer,bufferUsage, 0 );

      //delete[] printBuffer;
    }

    if(sig_p->probesArray1 != nullptr) { delete[] sig_p->probesArray1; }
    if(sig_p->probesArray2 != nullptr) { delete[] sig_p->probesArray2; }
    delete sig_p;
  }
  else
  {

  } 

}

void LonHttpInterface_c::HandleSetRelVal(char* argsStr,  SocketTcp_c* socket)
{
  char* tmpPtr = argsStr;

  uint32_t noOfDevices;
  bool ok = FetchNextArg(&tmpPtr,&noOfDevices);  

  int length = 0;
  char* responseBuffer;

  if(ok && noOfDevices > 0)
  {

    for(int i=0;i<noOfDevices;i++)
    {
      uint32_t lAdr;
      uint32_t port;
      uint32_t value;
 
      ok = FetchNextArg(&tmpPtr,&lAdr);
      if(ok == false) { break; }
      ok = FetchNextArg(&tmpPtr,&port);
      if(ok == false) { break; }
      ok = FetchNextArg(&tmpPtr,&value);
      if(ok == false) { break; }

      LonSetOutput_c* sig_p = new LonSetOutput_c;
      sig_p->lAdr = lAdr;
      sig_p->port = port;
      sig_p->value = value;
      sig_p->Send();
    }
  }

  if(ok)
  {
    SendAnswer(socket,WEB_REPLY_OK,"Content-Length: 0\r\n");
  }
  else
  {
    SendAnswer(socket,WEB_BAD_REQUEST,nullptr);
  }

}

#define PRINT_BUFFER_SIZE 4096

void LonHttpInterface_c::HandleGetOutputs(char* argsStr,  SocketTcp_c* socket,bool fullData)
{
  LonGetOutputsList_c* sig_p = new LonGetOutputsList_c;

  sig_p->task = xTaskGetCurrentTaskHandle();
  sig_p->noOfDevices = 0;
  sig_p->noOfOutputs = 0;
  sig_p->Send();

  ulTaskNotifyTake(pdTRUE ,portMAX_DELAY );

  if(sig_p->noOfDevices > 0)
  {
    char* printBuffer = new char[PRINT_BUFFER_SIZE];
    sprintf(printBuffer,"%d",sig_p->noOfOutputs);
    uint16_t printBufferUsage = strlen(printBuffer);
    SendAnswer(socket,WEB_REPLY_OK,"Content-Type: multipart/form-data\r\n");
    for(int devIdx=0;devIdx<sig_p->noOfDevices;devIdx++)
    {
      LonDatabase_c database;
      LonConfigPage_st* config = nullptr;
      if(fullData)
      {
        config = database.ReadConfig(sig_p->outputList[devIdx].lAdr);
      }

      for(int port=0;port<8;port++)
      {
        uint8_t valMasc = (sig_p->outputList[devIdx].validBitmap >> (2*port)) & 0x03;

        if(valMasc != 0)
        {
     

          if(printBufferUsage + 64 > PRINT_BUFFER_SIZE)
          {
            socket->Send((uint8_t*)printBuffer,printBufferUsage, 0 );
            printBuffer = new char[PRINT_BUFFER_SIZE];
            printBufferUsage = 0;
          }

          if(fullData)
          {
            LonPortData_st* portData;
            if(config!= nullptr)
            {
              portData = &(config->action[port].portData);
            }
            sprintf(printBuffer + printBufferUsage,",0x%08X,%d,%d,%d,%s",
                    sig_p->outputList[devIdx].lAdr,
                    port,
                    valMasc,
                    sig_p->outputList[devIdx].values[port],
                    (config != NULL) ? portData->name : "unnamed");
          }
          else
          {
            sprintf(printBuffer + printBufferUsage,",0x%08X,%d,%d",
                    sig_p->outputList[devIdx].lAdr,
                    port,
                    sig_p->outputList[devIdx].values[port]);
          }
          printBufferUsage += strlen(printBuffer+printBufferUsage);
        }
      }
    }
    socket->CloseAfterSend();
    socket->Send((uint8_t*)printBuffer,printBufferUsage, 0 );
  }
  else
  {
    socket->CloseAfterSend();
    SendAnswer(socket,WEB_REPLY_OK,nullptr);
  }



  delete sig_p;
}

void LonHttpInterface_c::HandleGetTimers(char* argsStr,  SocketTcp_c* socket)
{
  LonDatabase_c database;
  uint16_t noOfTimers = database.GetNoOfTimers();


  char* printBuffer = new char[256*noOfTimers];
  sprintf(printBuffer,"%d,30",noOfTimers);
  uint16_t bufferUsage = strlen(printBuffer);

  for(int i=0;i<noOfTimers;i++)
  {
    LonTimerConfig_st* timerConfig = database.ReadTimerConfig(i);

    sprintf(printBuffer+bufferUsage,",%d",i);
    bufferUsage += strlen(printBuffer+bufferUsage);

    snprintf(printBuffer+bufferUsage,12,",%s",timerConfig->name);
    printBuffer[bufferUsage+12] = 0;
    bufferUsage += strlen(printBuffer+bufferUsage);

    sprintf(printBuffer+bufferUsage,",%d,%d,%d,%d",
    timerConfig->ena,
    timerConfig->type,
    timerConfig->seq1,
    timerConfig->seq2);
    bufferUsage += strlen(printBuffer+bufferUsage);

    for(int i=0;i<4;i++)
    {
      sprintf(printBuffer+bufferUsage,",0x%08X,%d",timerConfig->lAdr[i],timerConfig->port[i]);
      bufferUsage += strlen(printBuffer+bufferUsage);
    }

    for(int i=0;i<6;i++)
    {
      sprintf(printBuffer+bufferUsage,",%02d,%02d",timerConfig->time[i].hour,timerConfig->time[i].minutes);
      bufferUsage += strlen(printBuffer+bufferUsage);
    }
   
    for(int i=0;i<4;i++)
    {
      if((timerConfig->lAdr[i] != 0xFFFFFFFF) && (timerConfig->lAdr[i] != 0))
      {
        LonConfigPage_st* config = database.ReadConfig(timerConfig->lAdr[i]);
        if(config != nullptr) 
        {
          LonPortData_st* portData = &(config->action[timerConfig->port[i]].portData);
          snprintf(printBuffer+bufferUsage,12,",%s",portData->name);
          printBuffer[bufferUsage+12] = 0;
        }
        else
        {
          strcpy(printBuffer+bufferUsage,",");
        }
      }
      else
      {
        strcpy(printBuffer+bufferUsage,",");
      }
      bufferUsage += strlen(printBuffer+bufferUsage);
    }

  }

  SendAnswer(socket,WEB_REPLY_OK,"Content-Type: multipart/form-data\r\n");
  socket->CloseAfterSend();
  socket->Send((uint8_t*)printBuffer,bufferUsage, 0 );

}

void LonHttpInterface_c::HandleSetTimer(char* argsStr,  SocketTcp_c* socket)
{
  char* tmpPtr = argsStr;

  uint32_t idx;
  uint32_t tmpPar;
  bool ok = FetchNextArg(&tmpPtr,&idx); 

  LonTimerConfig_st* config_p = new LonTimerConfig_st;

  LonDatabase_c database;
  uint16_t noOfTimers = database.GetNoOfTimers();

  if(idx >= noOfTimers)
  {
    ok = false;
  }


  if(ok)
  {
    char* namePtr = tmpPtr;
    uint8_t nameLen = FetchNextArgString(&tmpPtr,&tmpPar); 

    memset(config_p->name,0,12);
    if(nameLen >=12) { nameLen = 11;}
    memcpy(config_p->name,namePtr,nameLen);
    
    ok = FetchNextArg(&tmpPtr,&tmpPar); 
    config_p->ena = tmpPar;
  }

  if(ok)
  {
    ok = FetchNextArg(&tmpPtr,&tmpPar); 
    config_p->type = tmpPar;
  }

  if(ok)
  {
    ok = FetchNextArg(&tmpPtr,&tmpPar); 
    config_p->seq1 = tmpPar;
  }

  if(ok)
  {
    ok = FetchNextArg(&tmpPtr,&tmpPar); 
    config_p->seq2 = tmpPar;
  }

  for(int i=0;i<4 && ok;i++)
  {
    ok = FetchNextArg(&tmpPtr,&tmpPar); 
    config_p->lAdr[i] = tmpPar;
    if(ok)
    {
      ok = FetchNextArg(&tmpPtr,&tmpPar); 
      config_p->port[i] = tmpPar;
    }
  }

  for(int i=0;i<6 && ok;i++)
  {
    ok = FetchNextArg(&tmpPtr,&tmpPar); 
    config_p->time[i].hour = tmpPar;
    if(ok)
    {
      ok = FetchNextArg(&tmpPtr,&tmpPar); 
      config_p->time[i].minutes = tmpPar;
    }
  }

  if(ok)
  {
    database.SaveTimerConfig(idx,config_p);


    SendAnswer(socket,WEB_REPLY_OK,"Content-Length: 0\r\n");
  }
  else
  {
    SendAnswer(socket,WEB_BAD_REQUEST,nullptr);
  }



  delete config_p;

      

}

void LonHttpInterface_c::HandleGetPowerInfo(char* argsStr,  SocketTcp_c* socket)
{

  LonGetPowerInfo_c* sig_p = new LonGetPowerInfo_c;

  sig_p->task = xTaskGetCurrentTaskHandle();
  sig_p->Send();

  ulTaskNotifyTake(pdTRUE ,portMAX_DELAY );

  char extraLine[32];

  char* printBuffer = new char[256];
  printBuffer[0] = 0;
  int bufferUsage = 0;

  for(int i=0;i<14;i++)
  {
    sprintf(extraLine,"%.2f",sig_p->data[i]);
    int len = strlen(extraLine);
    if((bufferUsage + len+2) < 256)
    {
      strcpy(printBuffer,extraLine);
      bufferUsage += len;
      if(i<13)
      {
        strcat(printBuffer,",");
        bufferUsage++;
      }      
    }
  }
        
  sprintf(extraLine,"Content-Length: %d\r\n",bufferUsage);

  SendAnswer(socket,WEB_REPLY_OK,extraLine);

  socket->Send((uint8_t*)printBuffer,bufferUsage, 0 );

  delete[] printBuffer;

  delete sig_p;
}

bool LonHttpInterface_c::ParseInt(char* valStr, uint8_t strLen, uint32_t* valInt)
{
  uint32_t val = 0;

  if((strLen>2) && (valStr[0]='0') && (valStr[1] == 'x'))
  {
    /* parse hex */
    for(int i=2;i<strLen;i++)
    {
      val *= 16;
      char c = valStr[i];
      if(c >='0' && c <='9')
      {
        val += (c -'0');
      }
      else if(c >='A' && c <='F')
      {
        val += (10 + c -'A');
      }
      else
      {
        return false;
      }
    }
  }
  else if(strLen > 0)
  {
    for(int i=0;i<strLen;i++)
    {
      val *= 10;
      char c = valStr[i];
      if(c >='0' && c <='9')
      {
        val += (c -'0');
      }
      else
      {
        return false;
      }
    }
  }
  else
  {
    return false;
  }
  *valInt = val;
  return true;
}

bool LonHttpInterface_c::FetchNextArg(char** startStr,uint32_t *argVal)
{
  char* tmpPtr = *startStr;

  uint8_t length = FetchNextArgString(startStr,argVal);

  bool res = ParseInt(tmpPtr,length,argVal);

  return res;
}

int LonHttpInterface_c::FetchNextArgString(char** startStr,uint32_t *argVal)
{

  char* endPtr = strchr(*startStr,',');

  uint8_t length = 0;
  if(endPtr == nullptr)
  {
    length = strlen(*startStr);
    *startStr = nullptr;
  }
  else
  {
    length = endPtr - *startStr ;
    *startStr = endPtr + 1;
  }

  return length;

}

#endif