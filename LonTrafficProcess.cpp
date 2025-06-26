#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"


#include "LonDataDef.hpp"
#include "SignalList.hpp"

#include "LonDevice.hpp"

#include "LonChannel.hpp"
#include "LonChannelBasic.hpp"

#if LON_USE_VIRTUAL_CHANNELS == 1
#include "LonChannelVirtual.hpp"
#endif

#if LON_USE_TIMERS == 1
#include "LonTimers.hpp"
#endif
#if LON_USE_SENSORS == 1
#include "LonSensors.hpp"
#endif

#if LON_USE_COMMAND_LINK == 1
#include "commandHandler.hpp"
#endif

#include "LonDatabase.hpp"

#include "TimeClass.hpp"

#include "LonTrafficProcess.hpp"
/*

#include "FaultHandler.hpp"
*/

LonDevCheckTick_c devCheckTick_sig; /* static signal */
LonTmoScanTick_c TmoScanTick_sig;
LonTime_c lonTimeSig;

#if CONF_USE_TIME == 1
Lon5MinTick_c lon5MinTick;
#endif
extern IWDG_HandleTypeDef hiwdg1;

const char* eventString[] = {"NOEVENT","CHN_DOWN","CHG_UP","PRESS","RELEASE","CLICK","HOLD"};

LonTmoCounter_c* LonTmoCounter_c::firstTmo = NULL;

void vTimerCallback2( TimerHandle_t xTimer )
{
  devCheckTick_sig.Send();
}
void vTimerCallback3( TimerHandle_t xTimer )
{
  TmoScanTick_sig.Send();
}

#if CONF_USE_TIME == 1
void LonTimeEvent_c::Action(SystemTime_st* time)
{
  if(time->Second % 10 == 0)
  {
    memcpy(&(lonTimeSig.time),time,sizeof(SystemTime_st));
    lonTimeSig.SendISR();
  }

  if((time->Second == 0) && (time->Minute %5 == 0))
  {
    lon5MinTick.fullHourIndicator = (time->Minute == 0);
    lon5MinTick.SendISR();
  }
}
#endif

LonTrafficProcess_c::LonTrafficProcess_c(uint16_t stackSize, uint8_t priority, uint8_t queueSize, HANDLERS_et procId) : process_c(stackSize,priority,queueSize,procId,"TRAFFIC")
{
  for(int i = 0; i<NO_OF_CHANNELS;i++)
  {
    ch_p[i] = NULL;
  }


  for(int i = 0; i<NO_OF_BASIC_CHANNELS;i++)
  {
    ch_p[i] = new LonChannelBasic_c(i,this);
  }
  TimerHandle_t  timer = xTimerCreate("",configTICK_RATE_HZ,pdTRUE,( void * ) 0,vTimerCallback2);
  xTimerStart(timer,0);

  timer = xTimerCreate("",(configTICK_RATE_HZ / 10),pdTRUE,( void * ) 0,vTimerCallback3);
  xTimerStart(timer,0);

  #if LON_USE_SENSORS == 1
  sensors = new LonSensors_c(this);
  #endif

  actChecked = 0;

  LonDevice_c::trafProc_p = this;

}

void LonTrafficProcess_c::main(void)
{
  #if LON_USE_TIMERS == 1 
  LonTimer_c::InitTimers(this);
  #endif

  for(uint8_t chNo = 0;chNo<NO_OF_BASIC_CHANNELS;chNo++)
  {
    ch_p[chNo]->SendDetach();

  }

  vTaskDelay(1000);

  #if LON_USE_IRRIGATION_UNIT == 1
  irrigationUnit.InitIrrig(this);
  #endif

  #if LON_USE_WEATHER_UNIT == 1
  weatherUnit.InitWeather();
  #endif

  while(1)
  {
    releaseSig = true;
    RecSig();
    //RestartData_c::ownPtr->signalsHandled[HANDLE_TRAFFIC]++;
    uint8_t sigNo = recSig_p->GetSigNo();
    switch(sigNo)
    {
      /*case SIGNO_WDT_TICK:
        releaseSig = false;
        break;*/
      case SIGNO_LON_DEVCHECK_TICK:
        SearchNewDevices();
        CheckDevFromConfig();
        releaseSig = false;
        break;
      case SIGNO_LON_IODATAIN_CTRL:
        IoCtrlSigHandle((LonIOdata_c*)recSig_p);
        break;
      case SIGNO_LON_IODATAIN_TRAF:
        IoTrafSigHandle((LonIOdata_c*)recSig_p);
        break;

      case SIGNO_LON_TMOSCAN_TICK:
        ScanTmo();
        StepChannelTimeouts();

        #if LON_USE_PWR_MGMT == 0
        #if CONF_USE_WATCHDOG == 1
        __HAL_IWDG_RELOAD_COUNTER(&hiwdg1);
        #endif
        #endif
        releaseSig = false;
        break;
      case SIGNO_LON_TIME:
        #if LON_USE_TIMERS == 1 
        LonTimer_c::CheckAll((LonTime_c*) recSig_p);
        #endif
        #if LON_USE_SENSORS == 1
        sensors->ScanSensors((LonTime_c*) recSig_p);
        #endif 
        #if LON_USE_IRRIGATION_UNIT == 1
        irrigationUnit.TimeTick((LonTime_c*) recSig_p);
        #endif 
        releaseSig = false;      
        break;

      #if LON_USE_COMMAND_LINK == 1
      case SIGNO_LON_DEVICECONFIG:
        HandleDeviceConfig((LonDeviceConfig_c*)recSig_p);
        break;
      case SIGNO_LON_SET_OUTPUT:
        HandleSetOutput((LonSetOutput_c*) recSig_p);        
        break;
      case SIGNO_LON_GET_OUTPUT:
        HandleGetOutput((LonGetOutput_c*) recSig_p);
        releaseSig = false;
        break;
      case SIGNO_LON_GETDEVLIST:
        GetDevList((LonGetDevList_c*) recSig_p);
        releaseSig = false;
        break;
      case SIGNO_LON_GETDEVINFO:
        GetDevInfo((LonGetDevInfo_c*) recSig_p);
        releaseSig = false;
        break;
        #if CONF_USE_TIME == 1
        case SIGNO_LON_5_MIN_TICK:
        {
        #if LON_USE_WEATHER_UNIT == 1
        bool fullHourIndicator = ((Lon5MinTick_c*) recSig_p)->fullHourIndicator;
        weatherUnit.Tick(fullHourIndicator);
        #endif
        releaseSig = false;
        }
        break;
        #endif
        #if LON_USE_SENSORS_DATABASE == 1
      case SIGNO_LON_GET_OUTPUTS_LIST:
        GetOutputsList((LonGetOutputsList_c*) recSig_p);
        releaseSig = false;
        break;
        #endif
        #if LON_USE_SENSORS == 1
      case SIGNO_LON_GET_SENSOR_VALUES:
        GetSensorsValues((LonGetSensorsValues_c*) recSig_p);
        releaseSig = false;
        break;
     #if LON_USE_WEATHER_UNIT == 1
      case SIGNO_LON_GET_WEATHER_STATS:
        {
        weatherUnit.GetRainStats((LonGetWeatherStats_c*) recSig_p);
        releaseSig = false;
        }
        break;
      #endif
        #endif
    /*  case SIGNO_GETSTAT:
        HandleGetStat((GetStat_c*) recSig_p);
        break;*/
      #endif
      #if LON_USE_IRRIGATION_UNIT == 1
      case SIGNO_LON_IRRIG_UPD_CONFIG:
        irrigationUnit.UpdateLists();
        break;
      case SIGNO_LON_IRRIG_GETSTAT:
        irrigationUnit.Getstat((LonIrrigGetstat_c*) recSig_p);
        releaseSig = false;
        break;
      #endif

      default:
      break;

    }
    if(releaseSig) { delete  recSig_p; }
 
  }


}

LonDevice_c* LonTrafficProcess_c::GetDevByLadr(uint32_t lAdr)
{
  for(int i = 0; i<NO_OF_CHANNELS;i++)
  {  
    LonDevice_c* dev = NULL;
    if(ch_p[i] != NULL)
    {
      dev = ch_p[i]->SearchDevice(lAdr);
    }
    if(dev != NULL)
    {
      return dev;
    }
  }
  return NULL;
}

LonDevice_c* LonTrafficProcess_c::GetDevice(uint8_t chNo,uint8_t sAdr)
{
  if(ch_p[chNo] != NULL)
  {
    return ch_p[chNo]->GetDevice(sAdr);
  } 
  else
  {
    return NULL;
  }
}

void LonTrafficProcess_c::SearchNewDevices(void)
{
  for(int i = 0; i<NO_OF_CHANNELS;i++)
  {
    if(ch_p[i] != NULL) 
    {
      uint8_t state = ch_p[i]->GetState();
      if(state == RUNNING)
      {
        ch_p[i]->SearchNewDevices(false);
      }
    }
  }
}

void LonTrafficProcess_c::IoCtrlSigHandle(LonIOdata_c* recSig_p)
{
  uint8_t chNo = recSig_p->GetChNo();
  ch_p[chNo ]->HandleReceivedFrame(recSig_p);

}

void LonTrafficProcess_c::IoTrafSigHandle(LonIOdata_c* recSig_p)
{
  uint8_t chNo = recSig_p->GetChNo();
  ch_p[chNo ]->HandleReceivedFrame(recSig_p);
}



void LonTrafficProcess_c::HandleDataSw(LonDevice_c* dev_p, uint8_t noOfEvents, uint8_t* eventsArray)
{

  uint32_t lAdr = dev_p->GetLAdr();

  for(int i=0;i<noOfEvents;i++)
  {
    uint8_t event = eventsArray[i];
    uint8_t port;

    uint8_t eventCode = event>>4;
    event &= 0x0F;

    if(eventCode < 8)
    {
      port = dev_p->phyPortToPort(eventCode);

      dev_p->HandleSwEvent(this,port,event);

      #if LON_USE_COMMAND_LINK == 1
      char* strBuf = new char[64];
      sprintf(strBuf,"SW event, ladr=%08X port=%d,event =%s \n",lAdr,port,eventString[event]);
      CommandHandler_c::SendToAll(strBuf);
      delete[] strBuf;

      #endif

    }
    else
    {
      port = eventCode;
      if(eventCode == 8)
      {
        if(event == dev_p->lastSeqNo)
        {
          /* ignore signal */ 
          break;
        }
        dev_p->lastSeqNo = event;
      }
    }




  }
}







LonTmoCounter_c::LonTmoCounter_c(void )
{
  if(firstTmo != NULL)
  {
    firstTmo ->prev = this;
  }
  next = firstTmo;
  prev = NULL;
  firstTmo = this;
}
LonTmoCounter_c::~LonTmoCounter_c(void )
{
  if(prev != NULL)
  {
    prev->next = next;
  }
  else
  {
    firstTmo = next;
  }

  if(next != NULL)
  {
    next->prev = prev;
  }

}
LonTmoCounter_c* LonTmoCounter_c::Step(LonTrafficProcess_c* traf_p)
{
  if(counter > 0)
  {
    counter --;
    return next;
  }
  else
  {
    LonTmoCounter_c* ret_p = next;

    LonDevice_c* dev_p = traf_p->GetDevByLadr(rLAdr);
    if(dev_p != NULL)
    {
      dev_p->SetOutput(rPort,0);
    }
    dev_p = traf_p->GetDevByLadr(lAdr);
    if(dev_p != NULL)
    {
      dev_p->ClearTmo(port);
    }
        
    delete  this;
    return ret_p;


  }

}

void LonTrafficProcess_c::StepChannelTimeouts(void)
{
  for(int i = 0; i<NO_OF_CHANNELS;i++)
  {
    if(ch_p[i] != NULL)
    {
      ch_p[i]->CheckTimeout();
    }
  }

}

void LonTrafficProcess_c::ScanTmo(void)
{
  LonTmoCounter_c* tmo_p = LonTmoCounter_c::firstTmo;
  while(tmo_p != NULL)
  {
    tmo_p = tmo_p->Step(this);
 
  }
}



void StepDevCounters(LonDevice_c* dev_p,void* userPtr)
{
  if(dev_p->timeToNextScan > 0)
  {
    dev_p->timeToNextScan--;
    //printf("DEV 0x%08X:%d\n",dev_p->GetLAdr(),dev_p->timeToNextScan);
  }
}

void ClearStats(LonDevice_c* dev_p)
{
  dev_p->noOfLost = 0;
}


void LonTrafficProcess_c::RunForAll(Function_type function, void* userPtr)
{
  for(int i = 0; i<NO_OF_CHANNELS;i++)
  {
    if(ch_p[i] != NULL)
    {
      ch_p[i]->RunForAll(function,userPtr);
    }
  }
}

void LonTrafficProcess_c::CheckDevFromConfig(void)
{
  uint8_t newIdx = 0xFF;
  uint8_t idxCnt = actChecked;
  LonDatabase_c database;
  LonDevice_c* dev_p;
  uint32_t lAdr;

  /* step down all counters */

  RunForAll(StepDevCounters,nullptr);



  if(checkDevTimer == 0)
  {
    bool deviceFound = false;
    do
    {
      idxCnt++;
      dev_p = NULL;
      newIdx = 0xFF;
      if(database.GetNoOfDevices() <= idxCnt) { idxCnt = 0; }

      lAdr = database.GetConfig(idxCnt)->longAdr;
      if((lAdr >= 0x10000000) && (lAdr < 0xFFFFFFFF)) 
      {
        newIdx = idxCnt;
        
        dev_p =  GetDevByLadr(lAdr);

        if((dev_p != NULL) && (dev_p->IsLost() == false))
        {

          if(dev_p->timeToNextScan==0)
          {
            if((ch_p[dev_p->GetChNo()] != NULL)&&(ch_p[dev_p->GetChNo()]->GetState() == RUNNING))
            {
              deviceFound = true;
              break; /* got it ;) */
            }
          }
        }
        else
        {
          deviceFound = true;
          break;  /* got it ;) */
        }

      }
    }
    while(idxCnt != actChecked);

    if(deviceFound)
    {
      if((dev_p != NULL) && (ch_p[dev_p->GetChNo()] != NULL) && (dev_p->IsLost() == false) )
      {
        ch_p[dev_p->GetChNo()]->SendCheckDevice(lAdr);
        ch_p[dev_p->GetChNo()]->noOfScannedDevices++;
        dev_p->UpdateCheckTime();
      }
      else
      {
        for(int chNo = 0; chNo < NO_OF_CHANNELS; chNo++)
        {
          if((ch_p[chNo] != NULL) && (ch_p[chNo]->GetState() == RUNNING))
          {
            ch_p[chNo]->SendCheckDevice(lAdr);
            ch_p[chNo]->noOfSearchedDevices++;
            if(dev_p != NULL)
            {
              dev_p->UpdateCheckTime();
            }
          }
        }

      }
      actChecked = newIdx;

    }
    checkDevTimer = CHECK_DEV_INTERVAL;
  }
  else
  {
    checkDevTimer--;
  }
}

#if LON_USE_VIRTUAL_CHANNELS == 1
LonChannelVirtual_c* LonTrafficProcess_c::AssignNewVirtualChannel(LonDevice_c* dev_p)
{
  for(int i = NO_OF_BASIC_CHANNELS; i<NO_OF_CHANNELS; i++)
  {
    if(ch_p[i] == NULL)
    {
      LonChannelVirtual_c* newChannel = new LonChannelVirtual_c(i,dev_p,this);
      ch_p[i] = newChannel;
      return newChannel;
    }
  }
  return NULL;
}
#endif

void LonTrafficProcess_c::DeleteDevice(LonDevice_c* dev_p)
{
  for(int chNo = 0; chNo < NO_OF_CHANNELS; chNo++)
  {
    if(ch_p[chNo] != NULL)
    {
      ch_p[chNo]->DeleteDevice(dev_p);
    }
  }
  delete dev_p;
}

void LonTrafficProcess_c::SendFrame(LonDevice_c* dev_p, uint8_t bytesNo, uint8_t* buffer)
{
  uint8_t chNo = dev_p->GetChNo();

  if(ch_p[chNo] != NULL)
  {
    ch_p[chNo]->SendFrame(dev_p,bytesNo,buffer);
  }

}

#if LON_USE_COMMAND_LINK == 1


void LonTrafficProcess_c::GetDevList(LonGetDevList_c* recSig_p)
{

  uint8_t chNo = recSig_p->chNo;
  recSig_p->count = 0;
  recSig_p->moreData = false;
  recSig_p->chValid = false;
  if(chNo < NO_OF_CHANNELS)
  {
    if(ch_p[chNo] != NULL)
    {
      ch_p[chNo]->GetDevList(recSig_p);
      recSig_p->chValid = true;
    }
  }
  for(int i=chNo+1;i < NO_OF_CHANNELS;i++)
  {
    if(ch_p[chNo] != NULL)
    {
      recSig_p->moreData = true;
    }
  }

  xTaskNotifyGive(recSig_p->task);

}

void LonTrafficProcess_c::GetDevInfo(LonGetDevInfo_c* recSig_p)
{
  LonDevice_c* dev_p = GetDevByLadr(recSig_p->lAdr);
  if(dev_p != NULL)
  {
    dev_p->GetDevInfo(recSig_p);
  }
  else
  {
    recSig_p->result = false;
  }
  xTaskNotifyGive(recSig_p->task);
}

void LonTrafficProcess_c::HandleDeviceConfig(LonDeviceConfig_c* recSig_p)
{
  uint32_t lAdr = recSig_p->lAdr;

  LonDevice_c* dev_p = GetDevByLadr(lAdr);

  if(dev_p != NULL)
  {
    uint8_t chNo = dev_p->GetChNo();
    if(ch_p[chNo] != NULL)
    {
      ch_p[chNo]->ConfigDevice(dev_p);
    }

    //printf("config %X chNo=%d\n",lAdr,dev_p->GetChNo() );
    
  }  
}

void LonTrafficProcess_c::HandleSetOutput(LonSetOutput_c* recSig_p)
{
  LonDevice_c* dev_p = GetDevByLadr(recSig_p->lAdr);

  if(dev_p != NULL)
  {
    dev_p->SetOutput(recSig_p->port,recSig_p->value);    
  }  
}
void LonTrafficProcess_c::HandleGetOutput(LonGetOutput_c* recSig_p)
{
  xTaskNotifyGive(recSig_p->task);

}

#if LON_USE_SENSORS == 1
void LonTrafficProcess_c::GetSensorsValues(LonGetSensorsValues_c* recSig_p)
{
  for(int i=0; i < recSig_p->noOfDevices; i++)
  {
    LonDevice_c* dev_p = GetDevByLadr(recSig_p->sensorList[i].lAdr);

    if(dev_p != NULL)
    {
      dev_p->GetValues(recSig_p->sensorList[i].port,&recSig_p->sensorList[i].value1, &recSig_p->sensorList[i].value2); 
      recSig_p->sensorList[i].valid = 1;
    }  
    else
    {
      recSig_p->sensorList[i].valid = 0;
    }
  }
  xTaskNotifyGive(recSig_p->task);

}
#endif

void GetNoOfOutputTypeDevices(LonDevice_c* device,void* noOfDevices)
{
  devType_et devType = device->GetDevType();

  for(int port=0;port<8;port++)
  {
    uint8_t masc = portMasc[devType][port] & 0xF0;
    if((masc == 0x10 || masc == 0x10))
    {
      uint32_t* data_p = (uint32_t*) noOfDevices;
      (*data_p)++;
      return;
    }
  }
}

#if LON_USE_SENSORS_DATABASE == 1
void GetOutputsValues(LonDevice_c* device,void* vsig_p)
{
  devType_et devType = device->GetDevType();
  LonGetOutputsList_c* sig_p = (LonGetOutputsList_c*) vsig_p;

  if(sig_p->noOfDevices < sig_p->arraySize)
  {  
    bool devAdded = false;
    sig_p->outputList[sig_p->noOfDevices].validBitmap = 0;
    for(int port=0;port<8;port++)
    {
      uint8_t masc = portMasc[devType][port] & 0xF0;
      if((masc == 0x10 || masc == 0x40))
      {
        devAdded = true;
        sig_p->outputList[sig_p->noOfDevices].values[port] = device->GetOutputVal(port);
        if(masc == 0x10)
        {
          sig_p->outputList[sig_p->noOfDevices].validBitmap |= (1<< (2*port));
        }
        else if(masc == 0x40)
        {
          sig_p->outputList[sig_p->noOfDevices].validBitmap |= (2<< (2*port));
        } 
        sig_p->noOfOutputs++;  
      }
    }
    if(devAdded)
    {
      sig_p->outputList[sig_p->noOfDevices].lAdr = device->GetLAdr();
      sig_p->noOfDevices++;
    }
  }
}

void LonTrafficProcess_c::GetOutputsList(LonGetOutputsList_c* recSig_p)
{


  RunForAll(GetOutputsValues,recSig_p);



  xTaskNotifyGive(recSig_p->task);
}
#endif
/*
void LonTrafficProcess_c::HandleGetStat(GetStat_c* recSig_p)
{
  LonDatabase_c database;
  LonDevice_c* dev_p;

  if(recSig_p->detCode == GetStat_c::CH)
  {
    if(recSig_p->orderCode == GetStat_c::PRINT)
    {
      int chNoOf = 0;
      for(int chNo = 0; chNo < NO_OF_CHANNELS; chNo++)
      {
        if(ch_p[chNo] != NULL)
        {
          recSig_p->ch.state[chNo] = ch_p[chNo]->GetState();
          recSig_p->ch.devScanned[chNo] = ch_p[chNo]->noOfScannedDevices;
          recSig_p->ch.devSearched[chNo] = ch_p[chNo]->noOfSearchedDevices;
          recSig_p->ch.devLost[chNo] = ch_p[chNo]->noOfLostDevices;
          recSig_p->ch.devFound[chNo] = ch_p[chNo]->noOfFoundDevices;
          recSig_p->ch.timeout[chNo] = ch_p[chNo]->GetTimeoutCnt();
          chNoOf++;

        }
      }
      recSig_p->ch.noOf=chNoOf;


    }
    else if(recSig_p->orderCode == GetStat_c::CLEAR)
    {

      for(int chNo = 0; chNo < NO_OF_CHANNELS; chNo++)
      {
        if(ch_p[chNo] != NULL)
        {
          ch_p[chNo]->noOfScannedDevices = 0;
          ch_p[chNo]->noOfSearchedDevices = 0;
          ch_p[chNo]->noOfLostDevices = 0;
          ch_p[chNo]->noOfFoundDevices = 0;
        }
      }

    }
    recSig_p->SetAck();
    recSig_p->Send();
    releaseSig = false;
  }
  if(recSig_p->detCode == GetStat_c::DEV)
  {
    if(recSig_p->orderCode == GetStat_c::PRINT)
    {
      int noOf = 0;
      uint32_t lAdr;

      int endIdx = database.GetNoOfDevices();     

      for(int idx=0;idx<endIdx;idx++)
      {
        lAdr = database.GetConfig(idx)->longAdr;
        if((lAdr >= 0x10000000) && (lAdr < 0xFFFFFFFF)) 
        {
          dev_p =  GetDevByLadr(lAdr);
          if(dev_p!= NULL)
          {
            recSig_p->dev.lAdr[noOf] = lAdr;
            recSig_p->dev.chNo[noOf] = dev_p->GetChNo();
            recSig_p->dev.sAdr[noOf] = dev_p->GetSadr();
            recSig_p->dev.state[noOf] = dev_p->GetState();
            recSig_p->dev.noOfLost[noOf] = dev_p->noOfLost;
            recSig_p->dev.lastCheck[noOf] = dev_p->lastCheckTime;
          }
          else
          {
            recSig_p->dev.lAdr[noOf] = lAdr;
            recSig_p->dev.chNo[noOf] = 0xFF;
            recSig_p->dev.sAdr[noOf] = 0xFF;
            recSig_p->dev.state[noOf] = 0;
            recSig_p->dev.noOfLost[noOf] = 0;
            //recSig_p->dev.lastCheck[noOf] = {0,0,0,0};
          }
          noOf++;

          if(noOf == NO_OF_DEVICES_IN_DEVSTAT)
          {
            recSig_p->dev.noOf = noOf;
            recSig_p->dev.endOfList = false;
            recSig_p->SetAck();
            recSig_p->Send();

            recSig_p = new GetStat_c(GetStat_c::DEV);
            noOf = 0;
          }

        }

      }
      recSig_p->dev.noOf = noOf;
      recSig_p->dev.endOfList = true;
      recSig_p->SetAck();
      recSig_p->Send();
      releaseSig = false;

    }
    else if(recSig_p->orderCode == GetStat_c::CLEAR)
    {
      RunForAll(ClearStats);
      recSig_p->SetAck();
      recSig_p->Send();
      releaseSig = false;
    }

  }
}
*/
#endif
