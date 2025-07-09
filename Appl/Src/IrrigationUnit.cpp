#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#if LON_USE_IRRIGATION_UNIT == 1

#include "IrrigationUnit.hpp"

#include "CommandHandler.hpp"

#include "LonDatabase.hpp"

#include "LonTrafficProcess.hpp"
#include "LonDevice.hpp"

#include "LoggingUnit.hpp"


#if LON_USE_WEATHER_UNIT == 1
#include "LonWeather.hpp"
#endif

CommandIrrigation_c irrigationCommands;

LonTrafficProcess_c* IrrigationUnit_c::trafProc_p = nullptr;
IrrigationUnit_c* IrrigationInstance_c::irrigUnit_p = nullptr;


void IrrigationUnit_c::TimeTick(LonTime_c* recSig_p)
{
  for(int idx=0;idx < IRRIG_NO_OF; idx++)
  {
    if(instance[idx] != nullptr)
    {
      IrrigationInstance_c* inst = instance[idx];
      uint32_t lAdr = 0x00010000 | idx;
      bool cont = true;

      if(inst->config_p->longAdr != lAdr)
      {
        /* something wrong */
        LonDatabase_c database;
        inst->config_p =  (IrigationConfig_st*)database.ReadConfig(lAdr);
        if(inst->config_p == nullptr)
        {
          cont = false;
        }
      }

      if(cont)
      {
        inst->Check(recSig_p,idx);
      }
      else
      {
        /* still wrong */
        delete inst;
        instance[idx] = nullptr;
      }
    }
  }
}

void IrrigationInstance_c::Check(LonTime_c* recSig_p, int idx)
{
  uint16_t actTime = recSig_p->time.Hour * 60 + recSig_p->time.Minute;

  bool changed = false;

  IRRIG_STATE_et newState = state;

  if(state == IRRIG_INACTIVE)
  {
    /*check if turn on */
    uint16_t startTime;
    uint16_t stopTime;    
    for(int i =0;i<2;i++)
    {
      if(config_p->durationTime[i] > 0)
      {
        startTime = config_p->startTime[i].hour *60 + config_p->startTime[i].minutes;
        stopTime = startTime + config_p->durationTime[i];
        if((actTime >= startTime) && (actTime <= stopTime))
        {
          admStopTime = stopTime;
          timeIdx = i;
          /* recalc stopTime */
          realStopTime = admStopTime; 
          #if LON_USE_WEATHER_UNIT == 1
            float rainStat = irrigUnit_p->GetTrafProc()->weatherUnit.GetRainStat(config_p->rainRange[i]);
            float tempStat = irrigUnit_p->GetTrafProc()->weatherUnit.GetTempStat(config_p->tempRange[i]);

            usedRainStat = rainStat;
            usedTempStat = tempStat;

            float rainDecreaseRatio = rainStat * (float)config_p->rainCorrection[i];

            
            float tempDecreaseRatio = 0;
            if(tempStat > -64)
            {
               tempDecreaseRatio = ((float)config_p->temperatureCentre[i] - tempStat) * (float)config_p->temperatureCorrection[i];
            }

            realStopTime -= (int) (rainDecreaseRatio + tempDecreaseRatio);
          #else
            usedRainStat = 0;
            usedTempStat = 0;
          #endif
          
           

          if(actTime < realStopTime)
          {            
            newState = IRRIG_ACTIVE;
          }
          else
          {
            newState = IRRIG_FINISHED;
          }
          break;
        }
      }
    }

  }
  else if(state == IRRIG_ACTIVE)
  {
    if(actTime >= realStopTime)
    {


      if(actTime >= admStopTime)
      {
        newState = IRRIG_INACTIVE;
      }
      else
      {
        newState = IRRIG_FINISHED;
      }
    }
  }
  else if(state == IRRIG_FINISHED)
  {
    if(actTime > admStopTime)
    {
      newState = IRRIG_INACTIVE;
    }
  }

  if(state != newState)
  {
    char tmpStr[64];
    sprintf(tmpStr,"IRRIG%d SET %d, uR=%0.2f uT=%0.2f %02d:%02d\n",
    idx,
    newState,
    usedRainStat,
    usedTempStat,
    realStopTime/60,
    realStopTime%60);

    LoggingUnit_c::Log(LOG_TIME_EVENT,
    tmpStr
    );


  }
  state = newState;



  SetOutputs(state == IRRIG_ACTIVE, recSig_p->time.Second == 0  );

}

void IrrigationInstance_c::SetOutputs(bool newPortState, bool force)
{
  for(int i=1;i<5;i++)
  {
    uint32_t lAdr = config_p->outputLadr[i];
    if(lAdr != 0)
    {
      LonDevice_c* outDev_p = irrigUnit_p->GetTrafProc()->GetDevByLadr(lAdr);
      if(outDev_p != NULL)
      {
        uint8_t portState = outDev_p->GetOutputVal(config_p->outputPort[i]);

        if((portState != newPortState) || force)
        {
          outDev_p->SetOutput(config_p->outputPort[i],newPortState);
        }      
      }
    }
  }
  irrigUnit_p->SetMaster(config_p->outputLadr[0],config_p->outputPort[0],  newPortState,force);
}





IrrigationInstance_c::IrrigationInstance_c(void)
{
  state = IRRIG_INACTIVE;
  usedRainStat = 0;
  admStopTime = 0;
  realStopTime = 0;
}
IrrigationInstance_c::~IrrigationInstance_c(void)
{


}

void IrrigationUnit_c::SetMaster(uint32_t lAdr, uint8_t port,bool newPortState, bool force)
{
  if(lAdr != 0)
  {
    bool cont = true;
    if(newPortState == true)
    {
      /* tunr on not need check */
    }
    else
    {
      /* check if master may be turned off */  
      for(int idx=0;idx < IRRIG_NO_OF; idx++)
      {
        if(instance[idx] != nullptr)
        {
          if((instance[idx]->config_p != nullptr) && (instance[idx]->config_p->outputLadr[0] == lAdr) && (instance[idx]->config_p->outputPort[0] == port) )
          {
            /* instance uses this same master */
            if(instance[idx]->GetState() == IRRIG_ACTIVE)
            {
              cont = false;
              break;
            }
          }   
        }
      }
    }
    if(cont)
    {
      LonDevice_c* outDev_p = trafProc_p->GetDevByLadr(lAdr);
      if(outDev_p != NULL)
      {
        uint8_t portState = outDev_p->GetOutputVal(port);

        if((portState != newPortState) || force)
        {
          outDev_p->SetOutput(port,newPortState);
        }      
      }
    }
  }
}

void IrrigationUnit_c::InitIrrig(LonTrafficProcess_c* trafProc_p_)
{
  trafProc_p = trafProc_p_;  
  IrrigationInstance_c::irrigUnit_p = this;
  UpdateLists();
}


void IrrigationUnit_c::UpdateLists(void)
{
  for(int idx=0;idx < IRRIG_NO_OF; idx++)
  {
    uint32_t lAdr = 0x00010000 | idx;

    LonDatabase_c database;

    LonConfigPage_st* configFile_p = database.ReadConfig(lAdr);

    if(configFile_p != nullptr)
    {
      if(instance[idx] == nullptr)
      {
        instance[idx] = new IrrigationInstance_c;
      }
      instance[idx]->config_p = (IrigationConfig_st*)configFile_p;
    }
    else
    {
      if(instance[idx] != nullptr)
      {
        delete instance[idx];
        instance[idx] = nullptr;
      }
    }
  }


}


void IrrigationUnit_c::Getstat(LonIrrigGetstat_c* recSig_p)
{
  for(int idx=0;idx < IRRIG_NO_OF; idx++)
  {
    if(instance[idx] != nullptr)
    {
      recSig_p->state[idx] = instance[idx]->GetState();
      recSig_p->usedRain[idx] = instance[idx]->GetUsedRain();
      recSig_p->usedTemp[idx] = instance[idx]->GetUsedTemp();
      recSig_p->realStopTime[idx] = instance[idx]->GetRealStopTime();
    }
    else
    {
      recSig_p->state[idx] = -1;
      recSig_p->usedRain[idx] = 0;
      recSig_p->realStopTime[idx] = 0;
    }

  }

  xTaskNotifyGive(recSig_p->task);
}


/***** commands *************/

comResp_et Com_irrigconfig::Handle(CommandData_st* comData)
{
  uint32_t idx;
  bool idxValid = FetchParameterValue(comData,"IDX",&idx,0,IRRIG_NO_OF-1);



  if(idxValid)
  {
    bool allOk = true;
    uint32_t lAdr = 0x00010000 | idx;

    bool nameValid = false;
    int nameIdx = FetchParameterString(comData,"NAME");
    char* nameString;
    if(nameIdx >= 0)
    {
      nameString = &comData->buffer[comData->argValPos[nameIdx]];
      nameValid = true;
    }

    bool outLadrValid[5] = {false,false,false,false,false};
    bool outPortValid[5] = {false,false,false,false,false};
    uint32_t outPort[5];
    uint32_t outLadr[5];

    for(int i=0;i<5; i++)
    {
       char parName[16];
       if(i==0)
       {
          sprintf(parName,"MLADR");
       }
       else
       {
         sprintf(parName,"OUTLADR%d",i);
       }

       outLadrValid[i] = FetchParameterValue(comData,parName,&(outLadr[i]),0, 0xFFFFFFFF);
       if(( outLadr[i] !=0 ) && (outLadr[i]<0x10000000)) { outLadrValid[i]= false; }

       if(i==0)
       {
          sprintf(parName,"MPORT");
       }
       else
       {
         sprintf(parName,"OUTPORT%d",i);
       }
       outPortValid[i] = FetchParameterValue(comData,parName,&(outPort[i]),0, 7);
    }

    uint32_t ena;
    bool enaValid = FetchParameterValue(comData,"ENA",&ena,0,1);    

    bool timeValid[2];
    uint32_t hour[2];
    uint32_t min[2];

    bool durationValid[2];
    uint32_t duration[2];

    bool rainCorrValid[2];
    uint32_t rainCorr[2];

    bool tempCorrValid[2];
    uint32_t tempCorr[2];

    bool tempCentreValid[2];
    uint32_t tempCentre[2];

    bool rainRangeValid[2];
    uint32_t rainRange[2];

    bool tempRangeValid[2];
    uint32_t tempRange[2];

    for(int i=0;i<2; i++)
    {
      char parName[16];
      sprintf(parName,"STIME%d",i+1);
      timeValid[i] = FetchParameterTime(comData,parName,&hour[i],&min[i],nullptr);

      sprintf(parName,"DUR%d",i+1);
      durationValid[i] = FetchParameterValue(comData,parName,&(duration[i]),0,255 );

      sprintf(parName,"RCORR%d",i+1);
      rainCorrValid[i] = FetchParameterValue(comData,parName,&(rainCorr[i]),0,255 );

      sprintf(parName,"TCORR%d",i+1);
      tempCorrValid[i] = FetchParameterValue(comData,parName,&(tempCorr[i]),0,255 );

      sprintf(parName,"TCENT%d",i+1);
      tempCentreValid[i] = FetchParameterValue(comData,parName,&(tempCentre[i]),0,40 );

      sprintf(parName,"RRANGE%d",i+1);
      rainRangeValid[i] = FetchParameterValue(comData,parName,&(rainRange[i]),1,48 );

      sprintf(parName,"TRANGE%d",i+1);
      tempRangeValid[i] = FetchParameterValue(comData,parName,&(tempRange[i]),1,48 );
    }

    if(allOk == true)
    {
      LonDatabase_c database;

      IrigationConfig_st* config_p = new IrigationConfig_st;
      IrigationConfig_st* orgConfig_p = (IrigationConfig_st*)database.ReadConfig(lAdr);
      if(orgConfig_p == NULL)
      {
        memset(config_p,0,sizeof(IrigationConfig_st));
        config_p->longAdr = lAdr;
      }
      else
      {
        memcpy(config_p,orgConfig_p,sizeof(IrigationConfig_st));
      }

      if(nameValid)
      {
        memcpy(config_p->name,nameString,12);
      }

      for(int i=0;i<4; i++)
      {
        if(outLadrValid[i]) { config_p->outputLadr[i] = outLadr[i]; }
        if(outPortValid[i]) { config_p->outputPort[i] = outPort[i]; }
      }
      if(enaValid) { config_p->enable = ena; }

      for(int i=0;i<2; i++)
      {
        if(timeValid[i]) { config_p->startTime[i].hour = hour[i]; config_p->startTime[i].minutes = min[i]; }
        if(durationValid[i]) { config_p->durationTime[i] = duration[i]; }
        if(rainCorrValid[i]) { config_p->rainCorrection[i] = rainCorr[i]; }
        if(tempCorrValid[i]) { config_p->temperatureCorrection[i] = tempCorr[i]; }
        if(tempCentreValid[i]) { config_p->temperatureCentre[i] = tempCentre[i]; }
        if(rainRangeValid[i]) { config_p->rainRange[i] = rainRange[i]; }
        if(tempRangeValid[i]) { config_p->tempRange[i] = tempRange[i]; }
      }

      database.SaveConfig((LonConfigPage_st*)config_p);
      LonIrrigConfUpdate_c* sig_p = new LonIrrigConfUpdate_c;
      sig_p->Send();
    }    
  }
  return COMRESP_OK;
}

comResp_et Com_irrigdelete::Handle(CommandData_st* comData)
{
  uint32_t idx;
  bool idxValid = FetchParameterValue(comData,"IDX",&idx,0,IRRIG_NO_OF-1);



  if(idxValid)
  {
    bool allOk = true;
    uint32_t lAdr = 0x00010000 | idx;
    LonDatabase_c database;
    database.DeleteConfig(lAdr);
    LonIrrigConfUpdate_c* sig_p = new LonIrrigConfUpdate_c;
    sig_p->Send();
  }
  return COMRESP_OK;
}

const char* irrigStateStr[] = {"MISSING","INACTIVE","ACTIVE","FINISHED","UNKNOWN"};


comResp_et Com_irrigprint::Handle(CommandData_st* comData)
{
  char* textBuf  = new char[256];

  LonIrrigGetstat_c* sig_p = new LonIrrigGetstat_c;
  sig_p->task = xTaskGetCurrentTaskHandle();
  sig_p->Send();

  ulTaskNotifyTake(pdTRUE ,portMAX_DELAY );


  for(int idx=0;idx < IRRIG_NO_OF; idx++)
  {
    uint32_t lAdr = 0x00010000 | idx;
    LonDatabase_c database;
    IrigationConfig_st* configFile_p = (IrigationConfig_st*)database.ReadConfig(lAdr);

    int state =  sig_p->state[idx] +1;
    if((state<0) && (state>3)) {state = 4;}

    if(configFile_p != nullptr)
    {  
      sprintf(textBuf,"Instance %d: NAME=%s ENA=%d STATE=%s \n", idx,  configFile_p->name, configFile_p->enable, irrigStateStr[state]);
      Print(comData->commandHandler,textBuf);

      #if LON_USE_WEATHER_UNIT == 1
      
      uint8_t hour = sig_p->realStopTime[idx] /60;
      uint8_t min = sig_p->realStopTime[idx] %60;

      sprintf(textBuf,"Used rain=%0.2f Used temp = %0.2f Used stop time = %02d:%02d \n", sig_p->usedRain[idx], sig_p->usedTemp[idx], hour,min);
      Print(comData->commandHandler,textBuf);

      #endif


      for(int i=0;i<2;i++)
      {
        sprintf(textBuf,"STIME%d=%02d:%02d DUR=%d RCORR=%d TCORR=%d TCENT=%d RRANGE=%d TRANGE=%d\n",i+1,
         configFile_p->startTime[i].hour,
         configFile_p->startTime[i].minutes,
         configFile_p->durationTime[i],
         configFile_p->rainCorrection[i],
         configFile_p->temperatureCorrection[i],
         configFile_p->temperatureCentre[i],
         configFile_p->rainRange[i],
         configFile_p->tempRange[i]);
         Print(comData->commandHandler,textBuf);
      }

      sprintf(textBuf,"Master output: 0x%08X:%d\n",configFile_p->outputLadr[0], configFile_p->outputPort[0]);
      Print(comData->commandHandler,textBuf);

      for(int i=1;i<5;i++)
      {
        sprintf(textBuf,"OUT%d: 0x%08X:%d\n",i , configFile_p->outputLadr[i], configFile_p->outputPort[i]);
        Print(comData->commandHandler,textBuf);
      }
    }
  }
  delete[] textBuf;
  delete sig_p;

  return COMRESP_OK;
}

#endif