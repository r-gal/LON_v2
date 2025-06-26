
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#if CONF_USE_COMMANDS == 1

#include "LonCommands.hpp"
#include "LonDatabase.hpp"
#include "LonDevice.hpp"

#if CONF_USE_SDCARD == 1
#include "FileSystem.hpp"
#endif

CommandLon_c commandLon;

const char* devStateStrings[] = 
{
    "NEW",
    "CONFIGURING",
    "CONFIGURED",
    "RUNNING",
    "CONFIGURING_DUMMY",
    "CONFIGURED_DUMMY",   
    "RUNNING_DUMMY",
    "LOST"
};

const char* chStateSTrings[] = 
{
  "NEW",
  "INITIATING",
  "RUNNING",
  "ADR_CONQ_SENT_INITIAL",
  "ADR_CONQ_SENT",
  "CONFIG_SENT_INITIAL",
  "CONFIG_SENT"
};

comResp_et Com_getdevlist::Handle(CommandData_st* comData)
{

  uint32_t chNo;
  bool chnoValid = FetchParameterValue(comData,"CHNO",&chNo,0, NO_OF_CHANNELS-1);

  uint8_t startIdx, stopIdx;

  if(chnoValid)
  {
    startIdx = chNo;
    stopIdx = chNo;
  }
  else
  {
    startIdx = 0;
    stopIdx = NO_OF_CHANNELS-1;

  }

  Print(comData->commandHandler,"LON DEVICES LIST:\n");

  char* strBuf = new char[128];

  LonGetDevList_c* sig_p = new LonGetDevList_c;

  for(int idx = startIdx;idx <= stopIdx; idx++)
  {

    sig_p->chNo = idx;
    sig_p->task = xTaskGetCurrentTaskHandle();
    sig_p->Send();

    ulTaskNotifyTake(pdTRUE ,portMAX_DELAY );

    if(sig_p->chValid)
    {

      sprintf(strBuf,"CHANNEL %d has %d  devices\n",sig_p->chNo,sig_p->count);
      Print(comData->commandHandler,strBuf);
  
      for(int i=0; i<sig_p->count;i++)
      {
        LonDatabase_c database;
        LonConfigPage_st* config = database.ReadConfig(sig_p->lAdr[i]);
    
        sprintf(strBuf,"LADR = 0x%08X, TYPE=%s , SADR=%d, STATE=%s CONFIG=%s\n",
        sig_p->lAdr[i],
        LonDevice_c::GetDevTypeString(sig_p->lAdr[i]),
        sig_p->sAdr[i],
        devStateStrings[sig_p->status[i]],
        (config != NULL) ? "YES" : "NO");
        Print(comData->commandHandler,strBuf);
        for(uint8_t port = 0; port < 8; port++)
        {
          uint8_t portType = LonDevice_c::GetPortType(sig_p->lAdr[i],port);
          if(portType != 0x0F)
          {
            uint8_t ena = 0;
            uint8_t bis = 0;
            uint8_t inv  = 0;
            LonPortData_st* portData = NULL;
            if(config != NULL)
            {
              portData = &(config->action[port].portData);
              ena = (config->enabled & (1<<port)) ? 1 : 0;
              bis = (config->bistable & (1<<port)) ? 1 : 0;
              inv = (config->inverted & (1<<port)) ? 1 : 0;
            }

            switch(portType)
            {
              case 0: /*SW*/
                {              
                  sprintf(strBuf,"PORT %d,SWH,MODE=%d,NAME=%s,EBI=%d%d%d,OUT1=0x%08X:%d,OUT2=0x%08X:%d",port,
                  (config != NULL) ? portData->type : 0,
                  (config != NULL) ? portData->name : "unnamed",
                  ena,bis,inv,
                  (config != NULL) ? portData->out1LAdr : 0,
                  (config != NULL) ? portData->out1Port : 0,
                  (config != NULL) ? portData->out2LAdr : 0,
                  (config != NULL) ? portData->out2Port : 0
                  );  
                  Print(comData->commandHandler,strBuf);
                    
                  if((config != NULL) && (portData->type == 3))
                  {
                    sprintf(strBuf,",TMOCNT=%d\n",portData->cntMax);
                  }
                  else
                  {
                    sprintf(strBuf,"\n");
                  }
                }        
                break; 
              case 1: /*REL */
                {
                  sprintf(strBuf,"PORT %d,REL NAME=%s\n",
                  port,
                  (config != NULL) ? portData->name : "unnamed");
                }
                break;

             case 2: /*HIGRO */
                {         
                  sprintf(strBuf,"PORT %d,HIG E=%d  NAME=%s\n",port,
                  ena,
                  (config != NULL) ? portData->name : "unnamed");
                }
                break;

             case 3: /*PRESS */
                {     
                  sprintf(strBuf,"PORT %d,PRS E=%d NAME=%s TLADR=0x%X RPORT=%d ALT=%d\n",port,
                  ena,
                  (config != NULL) ? portData->name : "unnamed",
                  portData->out1LAdr,
                  portData->out1Port,
                  portData->altitude);
                }
                break;
              case 4: /* PWM */
                {
                  sprintf(strBuf,"PORT %d,PWM NAME=%s\n",
                  port,
                  (config != NULL) ? portData->name : "unnamed");
                }
                break;
              case 5: /* SPI */
                {
                  sprintf(strBuf,"PORT %d,SPI NAME=%s\n",
                  port,
                  (config != NULL) ? portData->name : "unnamed");
                }
                break;
              default:
                sprintf(strBuf,"PORT %d,UNN\n",port);
                break;
            }

            Print(comData->commandHandler,strBuf);

          }
        }
      }
    }    
  }

  Print(comData->commandHandler,"END.\n");

  delete[] strBuf;

  delete sig_p;

  return COMRESP_OK;
}




comResp_et Com_configdevice::Handle(CommandData_st* comData)
{
  uint32_t lAdr;
  bool lAdrValid = FetchParameterValue(comData,"LADR",&lAdr,0x10000000, 0xFFFFFFFF);
  uint32_t port;
  bool portValid = FetchParameterValue(comData,"PORT",&port,0,7);

  uint32_t lAdrR1;
  bool lAdrValidR1 = FetchParameterValue(comData,"RLADR1",&lAdrR1,0x10000000, 0xFFFFFFFF);
  uint32_t portR1;
  bool portValidR1 = FetchParameterValue(comData,"RPORT1",&portR1,0,7);

  uint32_t lAdrR2;
  bool lAdrValidR2 = FetchParameterValue(comData,"RLADR2",&lAdrR2,0x10000000, 0xFFFFFFFF);
  uint32_t portR2;
  bool portValidR2 = FetchParameterValue(comData,"RPORT2",&portR2,0,7);

  uint8_t portMode = 0;
  bool modeValid = false;
  int modeIdx = FetchParameterString(comData,"MODE");
  if(modeIdx >= 0)
  {
    modeValid = true;
    char* paramString = &comData->buffer[comData->argValPos[modeIdx]];
    if(strcmp(paramString,"IDL") == 0 ) { portMode = 0; }
    else if(strcmp(paramString,"STD") == 0 ) { portMode = 1; }
    else if(strcmp(paramString,"2RL") == 0 ) { portMode = 2; }
    else if(strcmp(paramString,"TMO") == 0 ) { portMode = 3; }
    else if(strcmp(paramString,"AUTO") == 0 ) { portMode = 4; }
    else if(strcmp(paramString,"DHT11") == 0 ) { portMode = 0; }
    else if(strcmp(paramString,"DHT22") == 0 ) { portMode = 1; }
    else {modeValid = false;}
  }

  uint32_t inv;
  bool invValid = FetchParameterValue(comData,"INV",&inv,0,1);
  uint32_t ena;
  bool enaValid = FetchParameterValue(comData,"ENA",&ena,0,1);
  uint32_t bis;
  bool bisValid = FetchParameterValue(comData,"BIS",&bis,0,1);

  uint32_t tmo;
  bool tmoValid = FetchParameterValue(comData,"TMOCNT",&tmo,1,0xFFFFFFFF);
  uint32_t pcnt;
  bool pcntValid = FetchParameterValue(comData,"PCNT",&pcnt,1,0xFF);
  uint32_t wcnt;
  bool wcntValid = FetchParameterValue(comData,"WCNT",&wcnt,1,0xFF);

  uint32_t tLAdr;
  bool tLAdrValid = FetchParameterValue(comData,"TLADR",&tLAdr,0x10000000, 0xFFFFFFFF);
  uint32_t tPort;
  bool tPortValid = FetchParameterValue(comData,"TPORT",&tPort,0,7);
  uint32_t alt;
  bool altValid = FetchParameterValue(comData,"ALT",&alt,0,9000);

  uint8_t sensorSelect;
  bool sensorSelectValid = false;
  int sesorSelectIdx = FetchParameterString(comData,"SEL");
  if(sesorSelectIdx >= 0)
  {
    char* paramString = &comData->buffer[comData->argValPos[sesorSelectIdx]];
    if(strcmp(paramString,"H") == 0 ) { sensorSelect = 0; }
    else if(strcmp(paramString,"T") == 0 ) { sensorSelect = 1; }
    else { sensorSelectValid = false; }
  }

  uint32_t hist;
  bool histValid = FetchParameterValue(comData,"HIST",&hist,0,10);
  uint32_t level;
  bool levelValid = FetchParameterValue(comData,"LEVEL",&level,0,100);

  uint32_t alarmMode;
  bool alarmModeValid = FetchParameterValue(comData,"AMODE",&alarmMode,0,2);
  
  bool nameValid = false;
  int nameIdx = FetchParameterString(comData,"NAME");
  char* nameString;
  if(nameIdx >= 0)
  {
    nameString = &comData->buffer[comData->argValPos[nameIdx]];
    nameValid = true;
  }


  if(lAdrValid)
  {
    LonDatabase_c database;

    LonConfigPage_st* config_p = new LonConfigPage_st;
    LonConfigPage_st* orgConfig_p = database.ReadConfig(lAdr);
    if(orgConfig_p == NULL)
    {
      memset(config_p,0,sizeof(LonConfigPage_st));
      config_p->longAdr = lAdr;
      config_p->pressCounterMax = 20;
      config_p->waitCounterMax = 20;
      config_p->enabled = 0xFF;
      config_p->inverted = 0xFF;
    }
    else
    {
      //memcpy_fast(config_p,orgConfig_p,sizeof(ConfigPage_st));
      memcpy(config_p,orgConfig_p,sizeof(LonConfigPage_st));
    }

    if(pcntValid) { config_p->pressCounterMax = pcnt; }
    if(wcntValid) { config_p->waitCounterMax = wcnt; }

    if(portValid)
    {
      uint8_t portType = LonDevice_c::GetPortType(lAdr,port);
      if(enaValid) { config_p->enabled = (config_p->enabled & ~(1<<port)) | (ena<<port); }
      if(invValid) { config_p->inverted = (config_p->inverted & ~(1<<port)) | (inv<<port); }
      if(bisValid) { config_p->bistable = (config_p->bistable & ~(1<<port)) | (bis<<port); }

      LonPortData_st* portData = &(config_p->action[port].portData);

      if(modeValid) { portData->type = portMode; }
      if(lAdrValidR1) { portData->out1LAdr = lAdrR1; }
      if(portValidR1) { portData->out1Port = portR1; }
      if(lAdrValidR2) { portData->out2LAdr = lAdrR2; }
      if(portValidR2) { portData->out2Port = portR2; }

      if(portType == 0 ) /* SW*/
      {
        if(tmoValid) { portData->cntMax = tmo; }


      }
      else if(portType == 2 ) /* hig */
      {
        if(sensorSelectValid)
        {
          LonSensorAlm_st* conf;
          if(sensorSelect == 0)
          {
            conf = &(portData->hSensor);
          }
          else
          {
            conf = &(portData->tSensor);
          }

          if(histValid) { conf->hist = hist; }
          if(levelValid) { conf->level = level; }
          if(alarmModeValid) { conf->conf = alarmMode; }
        }
      }
      else if(portType == 3 ) /* press */
      {
        if(altValid) { portData->altitude = alt; }
        if(tLAdrValid) {portData->out1LAdr = tLAdr; }
        if(tPortValid) {portData->out1Port = tPort; }
      }

      

      if(nameValid)
      {
        memcpy(portData->name,nameString,12);

      }
    } 

    database.SaveConfig(config_p);

    delete config_p;
    /* inform traffic process that config has been changed */
    LonDeviceConfig_c* sig_p = new LonDeviceConfig_c;
    sig_p->lAdr = lAdr;
    sig_p->Send();
    /* inform RPI unit that config has been changed */

  }
  return COMRESP_OK;
}



comResp_et Com_setoutput::Handle(CommandData_st* comData)
{
  uint32_t lAdr;
  bool parFetchRes = FetchParameterValue(comData,"LADR",&lAdr,0x10000000, 0xFFFFFFFF);
  uint32_t port;
  bool portValid = FetchParameterValue(comData,"PORT",&port,0, 7);
  uint32_t val;
  bool valValid = FetchParameterValue(comData,"VAL",&val,0, 255);

  if(parFetchRes && portValid && valValid)
  {
    LonSetOutput_c* sig_p = new LonSetOutput_c;
    sig_p->lAdr = lAdr;
    sig_p->port = port;
    sig_p->value = val;
    sig_p->Send();
    return COMRESP_OK;
  }
  else
  {
    return COMRESP_NOPARAM;
  }
  
}

comResp_et Com_replacedevice::Handle(CommandData_st* comData)
{
  uint32_t oldLAdr;
  bool oldLAdrValid = FetchParameterValue(comData,"OLDLADR",&oldLAdr,0x10000000, 0xFFFFFFFF);
  uint32_t newLAdr;
  bool newLAdrValid = FetchParameterValue(comData,"NEWLADR",&newLAdr,0x10000000, 0xFFFFFFFF);

  
  if(oldLAdrValid && newLAdrValid)
  {
    LonDatabase_c database;
    if(database.ReplaceDevice(oldLAdr,newLAdr))
    {
      return COMRESP_OK;
    }
    else
    {
      return COMRESP_FAILED;
    }
  }
  else
  {
    return COMRESP_NOPARAM;
  }  
}

comResp_et Com_printdevice::Handle(CommandData_st* comData)
{
  uint32_t lAdr;
  bool parFetchRes = FetchParameterValue(comData,"LADR",&lAdr,0x10000000, 0xFFFFFFFF);

  if(parFetchRes)
  {
    LonGetDevInfo_c* sig_p = new LonGetDevInfo_c;
    sig_p->lAdr = lAdr;
    sig_p->task = xTaskGetCurrentTaskHandle();
    sig_p->Send();

    ulTaskNotifyTake(pdTRUE ,portMAX_DELAY );

    if(sig_p->result)
    {
      char* textBuf  = new char[128];

      LonDatabase_c database;
      LonConfigPage_st* config = database.ReadConfig(sig_p->lAdr);

      int pMax=0;
      int wMax =0;

      if(config != NULL)
      {
        pMax = config->pressCounterMax;
        wMax = config->waitCounterMax;
      }

      sprintf(textBuf,"Device info: LADR=0x%08X, SADR=%d STATE=%s CONFIG=%s PMAX=%d WMAX=%d\n",
      sig_p->lAdr,
      sig_p->sAdr,
      devStateStrings[sig_p->status],
      (config != NULL) ? "YES" : "NO",
      pMax,wMax
      );
      Print(comData->commandHandler,textBuf);



      if(config != NULL)
      {

        for(uint8_t port = 0; port < 8; port++)
        {
          uint8_t portType = LonDevice_c::GetPortType(sig_p->lAdr,port);
          if(portType != 0x0F)
          {
            uint8_t ena = (config->enabled & (1<<port)) ? 1 : 0;
            LonPortData_st* portData = &(config->action[port].portData);
      
            switch(portType)
            {
              case 0: /*SW*/
              {
                uint8_t bis = (config->bistable & (1<<port)) ? 1 : 0;
                uint8_t inv = (config->inverted & (1<<port)) ? 1 : 0;            
                sprintf(textBuf,"PORT %d,SWH,MODE=%d,NAME=%s,EBI=%d%d%d,OUT1=0x%08X:%d,OUT2=0x%08X:%d TMO=%d\n",
                port,
                portData->type,
                portData->name,
                ena,bis,inv,
                portData->out1LAdr,portData->out1Port,
                portData->out2LAdr,portData->out2Port,
                portData->cntMax
                );       
                break;
              }
              case 1: /*REL */
                sprintf(textBuf,"PORT %d,REL VALUE=%.0f\n",port,sig_p->value[port]);
                break;

              case 2: /*HIG */

                sprintf(textBuf,"PORT %d,HIG E=%d MODE=%s NAME=%s Higro=%.1f Temp=%.1f\n",port,
                ena,
                (portData->type == 0) ? "DHT11" : "DHT22",
                portData->name,
                sig_p->value[port],
                sig_p->value2[port]);
                if((portData->hSensor.conf != 0) || (portData->tSensor.conf != 0))
                {
                  Print(comData->commandHandler,textBuf);            

                  sprintf(textBuf," HLEVEL=%d, HHIST=%d, HCONF=%d, TLEVEL=%d, THIST=%d, TCONF=%d\n ",
                  portData->hSensor.level,
                  portData->hSensor.hist,
                  portData->hSensor.conf,
                  portData->tSensor.level,
                  portData->tSensor.hist,
                  portData->tSensor.conf);
                  Print(comData->commandHandler,textBuf);
              
                  sprintf(textBuf," RLADR1 = 0x%X, RLPORT1 = %d, RLADR2 = 0x%X, RLPORT2 =%d \n",
                  portData->out1LAdr,
                  portData->out1Port,
                  portData->out2LAdr,
                  portData->out2Port);

                }
                break;
              case 3: /*PRESS */
              {
         
                sprintf(textBuf,"PORT %d,PRS E=%d  NAME=%s TLADR=0x%X RPORT=%d ALT=%d Pressure=%.1f\n",port,
                ena,
                portData->name,
                portData->out1LAdr,
                portData->out1Port,
                portData->altitude,
                sig_p->value[port]);
                break;
              }
         
              case 4: /* PWM */
                sprintf(textBuf,"PORT %d,PWM VALUE=%.0f\n",port,sig_p->value[port]);
                break;

              default:
                sprintf(textBuf,"PORT %d,UNN\n",port);
                break;

            }

            Print(comData->commandHandler,textBuf);
          }
        }
      }

      Print(comData->commandHandler,"END.\n");
      delete[] textBuf;
    }
    else
    {
      Print(comData->commandHandler,"Not found\n");
    }

    delete sig_p;
    return COMRESP_OK;
  }
  else
  {
    return COMRESP_NOPARAM;
  }
    
}

comResp_et Com_setpwm::Handle(CommandData_st* comData)
{
  uint32_t lAdr;
  bool parFetchRes = FetchParameterValue(comData,"LADR",&lAdr,0x10000000, 0xFFFFFFFF);
  uint32_t port;
  bool portValid = FetchParameterValue(comData,"PORT",&port,0, 7);
  uint32_t val;
  bool valValid = FetchParameterValue(comData,"VAL",&val,0, 255);

  if(parFetchRes && portValid && valValid)
  {
    LonSetOutput_c* sig_p = new LonSetOutput_c;
    sig_p->lAdr = lAdr;
    sig_p->port = port;
    sig_p->value = val;
    sig_p->Send();
    return COMRESP_OK;  
  }
  else
  {
    return COMRESP_NOPARAM;
  }  
}

#if LON_USE_TIMERS == 1
comResp_et Com_printtimers::Handle(CommandData_st* comData)
{

  LonDatabase_c database;

  uint32_t timIdx;
  bool timIdxValid = FetchParameterValue(comData,"IDX",&timIdx,0, TIMERSINPAGE-1);

  uint8_t startIdx, stopIdx;

  if(timIdxValid)
  {
    startIdx = timIdx;
    stopIdx = timIdx;
  }
  else
  {
    startIdx = 0;
    stopIdx = TIMERSINPAGE-1;

  }

  char* textBuf  = new char[256];

  char timName[16];

  for(uint8_t idx = startIdx; idx <= stopIdx; idx++)
  {
    LonTimerConfig_st* conf = database.ReadTimerConfig(idx);

    strncpy(timName,conf->name,16);
    timName[15] = 0;

    sprintf(textBuf,"T%02d: ENA=%d, TYPE=%d, NAME=%s, ",
    idx,
    conf->ena,
    conf->type,
    timName);
    Print(comData->commandHandler,textBuf);
    sprintf(textBuf,"T0=%02d:%02d, T1=%02d:%02d, T2=%02d:%02d, T3=%02d:%02d, T4=%02d:%02d, T5=%02d:%02d SEQ1=%d, SEQ2=%d\n",    
    conf->time[0].hour,conf->time[0].minutes,
    conf->time[1].hour,conf->time[1].minutes,
    conf->time[2].hour,conf->time[2].minutes,
    conf->time[3].hour,conf->time[3].minutes,
    conf->time[4].hour,conf->time[4].minutes,
    conf->time[5].hour,conf->time[5].minutes,
    conf->seq1,conf->seq2);

    Print(comData->commandHandler,textBuf);
    sprintf(textBuf," OUT1=%08X:%d, OUT2=%08X:%d, OUT3=%08X:%d, OUT4=%08X:%d\n",
    conf->lAdr[0], conf->port[0],
    conf->lAdr[1], conf->port[1],
    conf->lAdr[2], conf->port[2],
    conf->lAdr[3], conf->port[3]);
    Print(comData->commandHandler,textBuf);
  }
  delete[] textBuf;

  return COMRESP_OK;  
}

comResp_et Com_settimer::Handle(CommandData_st* comData)
{
  LonDatabase_c database;
  uint32_t idx;
  bool idxValid = FetchParameterValue(comData,"IDX",&idx,0,TIMERSINPAGE-1 );


  uint32_t lAdr[4];
  uint32_t port[4];
  bool lAdrValid[4];
  bool portValid[4];

  char tmpStr[8];

  for(int i=0;i<4;i++)
  {
    sprintf(tmpStr,"LADR%d",i+1);
    lAdrValid[i] = FetchParameterValue(comData,tmpStr,&lAdr[i],0x10000000, 0xFFFFFFFF);
    sprintf(tmpStr,"PORT%d",i+1);
    portValid[i] = FetchParameterValue(comData,tmpStr,&port[i],0,7); 
  }

  uint32_t ena;
  bool enaValid = FetchParameterValue(comData,"ENA",&ena,0,1);

  uint32_t seq1;
  bool seq1Valid = FetchParameterValue(comData,"SEQ1",&seq1,0,1);
  uint32_t seq2;
  bool seq2Valid = FetchParameterValue(comData,"SEQ2",&seq2,0,1);
  uint32_t type;
  bool typeValid = FetchParameterValue(comData,"TYPE",&type,0,1);


  bool nameValid = false;
  int nameIdx = FetchParameterString(comData,"NAME");
  char* nameString;
  if(nameIdx >= 0)
  {
    nameString = &comData->buffer[comData->argValPos[nameIdx]];
    nameValid = true;
  }


  
  bool timeValid[6];
  uint32_t hour[6];
  uint32_t min[6];

  for(int i=0;i<6;i++)
  {
    sprintf(tmpStr,"T%d",i);
    timeValid[i] = FetchParameterTime(comData,tmpStr,&hour[i],&min[i],nullptr);
  }

  if(idxValid)
  {
    LonTimerConfig_st* config_p = new LonTimerConfig_st;
    LonTimerConfig_st* orgConfig_p = database.ReadTimerConfig(idx);
    //memcpy_fast(config_p,orgConfig_p,sizeof(LonTimerConfig_st));
    memcpy(config_p,orgConfig_p,sizeof(LonTimerConfig_st));

    
    if(nameValid) { memcpy(config_p->name,nameString,12); }
    if(enaValid) { config_p->ena = ena; }

    for(int i=0;i<4;i++)
    {
      if(lAdrValid[i]) { config_p->lAdr[i] = lAdr[i]; }
      if(portValid[i]) { config_p->port[i] = port[i]; }
    }

    for(int i=0;i<6;i++)
    {
      if(timeValid[i])
      { 
        config_p->time[i].hour = hour[i]; 
        config_p->time[i].minutes = min[i];
      }
    }

    if(typeValid) { config_p->type = type; }
    if(seq1Valid) { config_p->seq1 = seq1; }
    if(seq2Valid) { config_p->seq2 = seq2; }
 
    database.SaveTimerConfig(idx,config_p);



    delete config_p;
    return COMRESP_OK; 
  }
  else
  {
    return COMRESP_NOPARAM;
  } 
   
}


comResp_et Com_formattimers::Handle(CommandData_st* comData)
{
  LonDatabase_c database;
  if(database.FormatTimers() == false)
  {
    return COMRESP_FAILED;
  }
  else
  {
    return COMRESP_OK;
  }  
}
#endif

comResp_et Com_formatconfig::Handle(CommandData_st* comData)
{
  LonDatabase_c database;
  if(database.FormatConfig() == false)
  {
    return COMRESP_FAILED;
  }
  else
  {
    return COMRESP_OK;
  }  
}

comResp_et Com_defragconfig::Handle(CommandData_st* comData)
{
  LonDatabase_c database;
  if(database.RunDefragmentation() == false)
  {
    return COMRESP_FAILED;
  }
  else
  {
    return COMRESP_OK;
  }  
}

comResp_et Com_printdatabase::Handle(CommandData_st* comData)
{
  LonDatabase_c database;

  char* strBuf  = new char[128];
  
  for(int i =0; i< database.GetNoOfDevices() ; i++)
  {
    uint32_t lAdr = database.GetConfig(i)->longAdr;

    if(lAdr == 0xFFFFFFFF)
    {
      sprintf(strBuf,"%03d:free\n",i);
    }
    else if(lAdr == 0)
    {
      sprintf(strBuf,"%03d:used\n",i);
    }
    else
    {
      sprintf(strBuf,"%03d:0x%08X\n",i,lAdr);
    }
    Print(comData->commandHandler,strBuf);
  }
  delete[] strBuf;
  return COMRESP_OK;
}

#if CONF_USE_SDCARD == 1
comResp_et Com_readconfig::Handle(CommandData_st* comData)
{
  LonDatabase_c database;

  bool fileNameValid = false;
  int fileNameIdx = FetchParameterString(comData,"FILE");
  char* fileNameString;

  File_c* file = nullptr;

  if(fileNameIdx >= 0)
  {
    fileNameString = &comData->buffer[comData->argValPos[fileNameIdx]];
    fileNameValid = true;

    file = FileSystem_c::OpenFile(fileNameString,"w");
  }
  else
  {
    return COMRESP_NOPARAM;
  }

  if(file == nullptr)
  {
    return COMRESP_FAILED;
  }

  file->Seek(0,File_c::SEEK_MODE_SET);

  for(int i =0; i< database.GetNoOfDevices() ; i++)
  {
    file->Write((uint8_t*)database.GetConfig(i),database.GetConfigSize());
  }
  file->Close();


  return COMRESP_OK;
}

comResp_et Com_writeconfig::Handle(CommandData_st* comData)
{
  LonDatabase_c database;

  bool fileNameValid = false;
  int fileNameIdx = FetchParameterString(comData,"FILE");
  char* fileNameString;

  File_c* file = nullptr;

  if(fileNameIdx >= 0)
  {
    fileNameString = &comData->buffer[comData->argValPos[fileNameIdx]];
    fileNameValid = true;

    file = FileSystem_c::OpenFile(fileNameString,"r");
  }
  else
  {
    return COMRESP_NOPARAM;
  }

  if(file == nullptr)
  {
    return COMRESP_FAILED;
  }

  database.FormatConfig();

  uint8_t* fileBuffer = new uint8_t[database.GetConfigSize()];

  for(int i =0; i< database.GetNoOfDevices() ; i++)
  {
    file->Read(fileBuffer,database.GetConfigSize());

    database.WriteFile(i,(LonConfigPage_st*)fileBuffer,0x03);
  }
  file->Close();

  delete[] fileBuffer;
  return COMRESP_OK;

}
#endif

#endif