#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "LonDataDef.hpp"
#include "SignalList.hpp"

#include "LonDevice.hpp"
#include "LonTrafficProcess.hpp"

#include "LonDatabase.hpp"

#include "LonChannel.hpp"
#if LON_USE_VIRTUAL_CHANNELS == 1
#include "LonChannelVirtual.hpp"
#endif

LonTrafficProcess_c* LonDevice_c::trafProc_p;

LonDevice_c::LonDevice_c(uint32_t lAdr_,uint8_t chNo_,uint8_t sAdr_): lAdr(lAdr_), chNo(chNo_), sAdr(sAdr_)
{
  state = NEW;

  //printf("create device, ladr=%08X, chNo = %d, sAdr = %d \n",lAdr,chNo,sAdr);

  lastSeqNo = 0xFF;
  timeToNextScan = 0;
  lastCheckTime.second = 0;
  lastCheckTime.minute = 0;
  lastCheckTime.hour = 0;

  noOfLost = 0;

  for(int i=0;i<8;i++)
  {
    portData_p[i] = NULL;
  }

  devType_et devType = GetDevType();
  /* phase 1 - calculate size */
  
  uint16_t size[8];
  uint16_t totalSize = 0;
  for(int i=0;i<8;i++)
  {
    uint16_t portSize;
    uint8_t portType = portMasc[devType][i];
    portType >>=4;

    switch(portType)
    {
      case 0: 
      portSize = sizeof(LonPortDataSw_c);
      break;
      case 1: 
      portSize = sizeof(LonPortDataRel_c);
      break;
      case 2: 
      portSize = sizeof(LonPortDataHigro_c);
      break;
      case 3: 
      portSize = sizeof(LonPortDataPress_c);
      break;
      case 4: 
      portSize = sizeof(LonPortDataPWM_c);
      break;
      #if LON_USE_VIRTUAL_CHANNELS == 1
      case 5: 
      portSize = sizeof(LonPortDataVirtualChannel_c);
      break;
      #endif
      default:
      portSize = 0;
      break;

    }
    if((portSize %4) != 0)
    {
      portSize = (portSize & 0xFFFC) + 4;
    }
    size[i] = portSize;

    totalSize += size[i];
  }

  portDataMemSpace = new uint8_t[totalSize];
  //printf("alloc %d bytes, adr = %08X\n",totalSize,(uint32_t) portDataMemSpace);

  uint32_t offset = 0;

  for(int i=0;i<8;i++)
  {
    if(size[i] > 0)
    {
      uint16_t portSize;
      uint8_t portType = portMasc[devType][i];
      portType >>=4;

      portData_p[i] = (LonPortData_c*) (portDataMemSpace + offset);

      //printf("port %d, size = %d, adr = %08X\n",i,size[i],(uint32_t) portData_p[i]);

      switch(portType)
      {
        case 0: 
        *portData_p[i] = LonPortDataSw_c(this,i);
        ((LonPortDataSw_c*)portData_p[i])->Init();
        break;
        case 1: 
        *portData_p[i] = LonPortDataRel_c(this,i);
        ((LonPortDataRel_c*)portData_p[i])->Init();
        break;
        case 2: 
        *portData_p[i] = LonPortDataHigro_c(this,i);
        ((LonPortDataHigro_c*)portData_p[i])->Init();
        break;
        case 3: 
        *portData_p[i] = LonPortDataPress_c(this,i);
        ((LonPortDataPress_c*)portData_p[i])->Init();
        break;
        case 4: 
        *portData_p[i] = LonPortDataPWM_c(this,i);
        ((LonPortDataPWM_c*)portData_p[i])->Init();
        break;
        #if LON_USE_VIRTUAL_CHANNELS == 1
        case 5: 
        *portData_p[i] = LonPortDataVirtualChannel_c(this,i);
        ((LonPortDataVirtualChannel_c*)portData_p[i])->Init();
        break;
        #endif
        default:        
        break;

      }

      offset += size[i];

    }

  }
  
  /*
  for(int i=0;i<8;i++)
  {
    uint16_t portSize;
    uint8_t portType = portMasc[devType][i];
    portType >>=4;

    switch(portType)
    {
      case 0: 
      portData_p[i] = new portDataSw_c(this,i);
      break;
      case 1: 
      portData_p[i] = new portDataRel_c(this,i);
      break;
      case 2: 
      portData_p[i] = new portDataHigro_c(this,i);
      break;
      case 3: 
      portData_p[i] = new portDataPress_c(this,i);
      break;
      case 4: 
      portData_p[i] = new portDataPWM_c(this,i);
      break;
      default:
      portData_p[i] = NULL;
      break;

    }
  }
  */
  

}

LonDevice_c::~LonDevice_c()
{
/*
  for(int i=0;i<8;i++)
  {
    if(portData_p[i] != NULL)
    {
      delete portData_p[i] ;
    }
  }
  
  */
    #if LON_USE_VIRTUAL_CHANNELS == 1
    if(GetDevType() == VIR)
    {
      LonChannelVirtual_c* ch_p = ((LonPortDataVirtualChannel_c*)(portData_p[0]))->channel_p;
      delete ch_p;
    }
    #endif

    if(portDataMemSpace != NULL)
    {
      //printf("delete device, ladr=%08X, chNo = %d, sAdr = %d \n",lAdr,chNo,sAdr);
      delete portDataMemSpace ;
    }
   
}

void LonDevice_c::GetConfig(uint8_t* confBuffer)
{
  LonDatabase_c database;
 
  uint8_t enabled = 0xFF;
  uint8_t inverted = 0;
  uint8_t monostable = 0xFF;
  uint8_t pMax = 20;
  uint8_t wMax = 20;

  LonConfigPage_st* config = database.ReadConfig(GetLAdr());

  if(config != NULL)
  {

    enabled = config->enabled;
    inverted = config->inverted;
    monostable = ~config->bistable;
    pMax = config->pressCounterMax;
    wMax = config->waitCounterMax;  
    state = CONFIGURING;
  }
  else
  {
    state = CONFIGURING_DUMMY;
  }

  devType_et devType = GetDevType();
  uint8_t sAdr = GetSadr();


  confBuffer[0] = (lAdr>>24)&0xFF;
  confBuffer[1] = (lAdr>>16)&0xFF;
  confBuffer[2] = (lAdr>>8)&0xFF;
  confBuffer[3] = (lAdr>>0)&0xFF;

  confBuffer[4] = sAdr;
  confBuffer[5] = mascToPhyMasc( enabled);
  switch(devType)
  {  
    case HIG:
      confBuffer[6] = 140;
      confBuffer[7] = 140;
      confBuffer[8] = 0;
      confBuffer[9] = 0;
      break;

    default:
      
      confBuffer[6] = mascToPhyMasc(monostable);
      confBuffer[7] = mascToPhyMasc(inverted);
      confBuffer[8] = pMax;
      confBuffer[9] = wMax;
      break;
  }
  /*if(devType == SW3 || devType==SW6)
  {
  printf("Get config, LADR=0x%08X, ena=%X, mono=%x, inv=%x\n",GetLAdr(),confBuffer[5],confBuffer[6],confBuffer[7]);
  }*/
}


void LonDevice_c::CheckTMOConsistency(LonTrafficProcess_c* trafproc_p)
{
  LonDatabase_c database;
  LonConfigPage_st* config = database.ReadConfig(GetLAdr());
  /* check if TMO is running */
  for(uint8_t port=0;port<8;port++)
  {
    devType_et devType = GetDevType();
    uint8_t portType = portMasc[devType][port];
    LonPortData_st* pConf_p = &(config->action[port].portData);
    if((portType == 0) && (pConf_p->type == 3))
    {
      //printf("TMO check LADR=0x%08X PORT=%d\n",GetLAdr(),port);
      LonPortDataSw_c* pData_p = (LonPortDataSw_c* )(portData_p[port]);
      if((pData_p != NULL) && (pData_p->tmo_p == NULL))
      {
        LonDevice_c* dev_p = trafproc_p->GetDevByLadr(pConf_p->out1LAdr);
        if(dev_p != NULL)
        {
          dev_p->SetOutput(pConf_p->out1Port,0);
        }
        dev_p = trafproc_p->GetDevByLadr(pConf_p->out2LAdr);
        if(dev_p != NULL)
        {
          dev_p->SetOutput(pConf_p->out2Port,0);
        }
      }
    }

  }
}

devType_et LonDevice_c::GetDevType(void)
{
  uint8_t prefix = lAdr >> 24;
  for(int i = 0; i < DEV_TYPES_NO; i++)
  {
    if(devPrefix[i] == prefix)
    {
      return (devType_et) i;
    }
  }
  return UNN;


}

void LonDevice_c::SetOutput(uint8_t port, uint8_t value)
{
  devType_et devType = GetDevType();
  uint8_t portType = portMasc[devType][port];
  uint8_t phyPort = portType & 0x0F;
  portType >>=4;

  if((portType == 1) || (portType == 4))
  {

    uint8_t buffer[] = { FRAME_DATA_TR<<4 | 0x06, sAdr,phyPort,value };

    trafProc_p->SendFrame(this,4,buffer);

  }

}

void LonDevice_c::SetOutputConfirm(uint8_t port, uint8_t value)
{
  devType_et devType = GetDevType();
  uint8_t portType = portMasc[devType][port];
  portType >>=4;

  if(portType == 1) /* REL*/
  {
    LonPortDataRel_c* portData = (LonPortDataRel_c*)portData_p[port];
    portData->storedValue = portData->value;
    portData->value = value;
  }
  else if(portType == 4) /* PWM */
  {
    LonPortDataPWM_c* portData = (LonPortDataPWM_c*)portData_p[port];
    portData->value = value;
  }
}

void LonDevice_c::SetOutputFinish(void)
{
  switch(state)
  {
    case CONFIGURED:
    case RUNNING:
      state = RUNNING;
      break;
   case CONFIGURED_DUMMY:
   case RUNNING_DUMMY:
     state = RUNNING_DUMMY;
     break;
   default:
     break;



  }
}

uint8_t LonDevice_c::phyPortToPort(uint8_t phyPort)
{
  devType_et devType = GetDevType();
  for(int i =0; i<8;i++)
  {
    uint8_t portType = portMasc[devType][i];
    if((portType &0x0F) == phyPort)
    {
      return i;
    }

  }
  return 0;

}

uint8_t LonDevice_c::GetOutputVal(uint8_t port)
{
  uint8_t resp = 0;
  if( (portMasc[GetDevType()][port] & 0xF0) == 0x10)
  {
   LonPortDataRel_c* portData = (LonPortDataRel_c*) portData_p[port];
    resp = portData->value;
  }
  else  if( (portMasc[GetDevType()][port] & 0xF0) == 0x40)
  {
    LonPortDataPWM_c* portData = (LonPortDataPWM_c*) portData_p[port];
    resp = portData->value;
  }
  return resp;
}

uint8_t LonDevice_c::GetStoredRelVal(uint8_t port)
{
  uint8_t resp = 0;
  if( (portMasc[GetDevType()][port] & 0xF0) == 0x10)
  {
    LonPortDataRel_c* portData = (LonPortDataRel_c*) portData_p[port];
    resp = portData->storedValue;
  }
  return resp;
}

uint8_t LonDevice_c::mascToPhyMasc(uint8_t masc)
{
  uint8_t phyMasc = 0;
  for(int i =0; i<8;i++)
  {
    if(masc & (1<<i))
    {
      uint8_t phyBit = portMasc[GetDevType()][i] & 0x0F;
      phyMasc |= (1<< phyBit);
 

    }

  }
  return phyMasc;

}

void  LonDevice_c::ClearTmo(uint8_t port)
{
  if( (portMasc[GetDevType()][port] & 0xF0) == 0x00)
  {
    LonPortDataSw_c* portData = (LonPortDataSw_c*)portData_p[port];
    portData->tmo_p = NULL;

  }

}

void LonDevice_c::HandleSwEvent(LonTrafficProcess_c* trafproc_p, uint8_t port,uint8_t event)
{
  if( (portMasc[GetDevType()][port] & 0xF0) == 0x00)
  {
    LonPortDataSw_c* portData = (LonPortDataSw_c*)portData_p[port];

    LonDatabase_c database;

    LonConfigPage_st* config = database.ReadConfig(GetLAdr());

    if(config != NULL)
    {
      LonPortData_st* pData = &(config->action[port].portData);
  
      switch(pData->type)
      {
        case 1: /* STD */
        {
          LonDevice_c* rDev1_p = trafproc_p->GetDevByLadr(pData->out1LAdr);
          LonDevice_c* rDev2_p = trafproc_p->GetDevByLadr(pData->out2LAdr);
          uint8_t rPort1 = pData->out1Port;
          uint8_t rPort2 = pData->out2Port;
          uint8_t value1 = 0;
          uint8_t value2 = 0;
          bool sendData = false;
          if(rDev1_p != NULL)
          {
            value1 = rDev1_p->GetOutputVal(rPort1);
          }
          if(rDev2_p != NULL)
          {
            value2 = rDev2_p->GetOutputVal(rPort2);
          }

          switch(event)
          {
            case EVENT_PRESSED:
              if(value1 + value2 )
              {
                value1 = 0;
                value2 = 0;
                sendData = true;
              }
              else
              {
                value1 = 1;
                value2 = 1;
                sendData = true;
              }              
              break;
            case EVENT_CHANGE_UP:
              value1 = 1;
              value2 = 1;
              sendData = true;
              break;
            case EVENT_CHANGE_DOWN:
              value1 = 0;
              value2 = 0;
              sendData = true;
              break;
            
              break;
            default:
            break;
          }

          if(sendData) 
          {
            if(rDev1_p != NULL)
            {
              rDev1_p->SetOutput(rPort1,value1);
            }
            if(rDev2_p != NULL)
            {
              rDev2_p->SetOutput(rPort2,value2);
            }
          }
        }
        break;
        case 2: /*2RL */
        {
          LonDevice_c* rDev1_p = trafproc_p->GetDevByLadr(pData->out1LAdr);
          LonDevice_c* rDev2_p = trafproc_p->GetDevByLadr(pData->out2LAdr);
          uint8_t rPort1 = pData->out1Port;
          uint8_t rPort2 = pData->out2Port;
          uint8_t value1 = 0;
          uint8_t value2 = 0;
          uint8_t storedValue1 = 0;
          uint8_t storedValue2 = 0;
          bool sendData = false;
          if(rDev1_p != NULL)
          {
            value1 = rDev1_p->GetOutputVal(rPort1);
            storedValue1 = rDev1_p->GetStoredRelVal(rPort1);
          }
          if(rDev2_p != NULL)
          {
            value2 = rDev2_p->GetOutputVal(rPort2);
            storedValue2 = rDev2_p->GetStoredRelVal(rPort2);
          }

          switch(event)
            {
              case EVENT_PRESSED:
                if(value1 + value2 )
                {
                  portData->ignoreNextRelease = 0;
                }
                else
                {
                  if((storedValue1 + storedValue2) == 0)
                  {
                    storedValue1 = 1;
                    storedValue2 = 1;
                  }
                  value1 = storedValue1;
                  value2 = storedValue2;                

                   
                  portData->ignoreNextRelease = 1;
                  sendData = true;
                } 
                
                break;
              case EVENT_RELEASED:
                if((value1 + value2 ) && (portData->ignoreNextRelease ==0))

                {
                  value1 = 0;
                  value2 = 0;
                  sendData = true;
                }
                portData->ignoreNextRelease = 0;
                break;
              case EVENT_HOLD:
                if((value1 == 1) && (value2 == 1))
                {
                  value1 = 0;
                  value2 = 1;
                  sendData = true;
                }
                else if((value1 == 0) && (value2 == 1))
                {
                  value1 = 1;
                  value2 = 0;
                  sendData = true;
                }
                else if((value1 == 1) && (value2 == 0))
                {
                  value1 = 1;
                  value2 = 1;
                  sendData = true;
                }
              
                break;
              default:
              break;
            }

            if(sendData) 
            {
              if(rDev1_p != NULL)
              {
                rDev1_p->SetOutput(rPort1,value1);
              }
              if(rDev2_p != NULL)
              {
                rDev2_p->SetOutput(rPort2,value2);
              }
            }


        }
        break;
        case 3: /*TMO*/
        {
          LonDevice_c* rDev_p = trafproc_p->GetDevByLadr(pData->out1LAdr);
          LonDevice_c* rDev2_p = trafproc_p->GetDevByLadr(pData->out2LAdr);
          if(rDev_p != NULL)
          {
            uint8_t rPort = pData->out1Port;
            uint8_t value = rDev_p->GetOutputVal(rPort);
            bool sendData = false;
            switch(event)
            {
              case EVENT_PRESSED:
                if(value == 0)
                { 
                  value = 1; 
                  if(portData->tmo_p == NULL)
                  {
                    portData->tmo_p = new LonTmoCounter_c;

                  }
                  portData->tmo_p->rLAdr = pData->out1LAdr;
                  portData->tmo_p->rPort = pData->out1Port;
                  portData->tmo_p->rLAdr2 = pData->out2LAdr;
                  portData->tmo_p->rPort2 = pData->out2Port;
                  portData->tmo_p->lAdr = lAdr;
                  portData->tmo_p->port = port;
                  portData->tmo_p->counter = pData->cntMax;
                
                } 
                else 
                {
                  value = 0;
                  if(portData->tmo_p != NULL)
                  {
                    delete (portData->tmo_p);
                    portData->tmo_p = NULL;
                  }
                }
                sendData = true;
                break;
              default:
              break;
            }
            if(sendData) 
            {
              rDev_p->SetOutput(rPort,value);
              if(rDev2_p != NULL)
              {
                rDev2_p->SetOutput(pData->out2Port,value);
              }
            }
          }


        }
        break;
        case 4:
        {
          /* Auto (rain sensor ... ) */

         #if LON_USE_WEATHER_UNIT == 1
         if(event ==  EVENT_PRESSED)
         {
           trafProc_p->weatherUnit.HandleEvent(GetLAdr(), port); /* rain sensor event */
         }
         #endif


        }
        break;
                  
        default:
        break;
      }
  
    }
    else
    {

    }
  }

}

void LonDevice_c::ConfirmConfig(void)
{ 
  switch(state)
  {
    case CONFIGURING:
    case CONFIGURED:
    case RUNNING:
      state = CONFIGURED;
      #if LON_USE_VIRTUAL_CHANNELS == 1
      if(GetDevType()==VIR)
      {
        LonChannelVirtual_c* ch_p = ((LonPortDataVirtualChannel_c*)(portData_p[0]))->channel_p;
        if(ch_p->GetState() == state_et(NEW))
        {
          ch_p->SendDetach();
        }
      }
      #endif
      break;
    default:
      state = CONFIGURED_DUMMY;
      break;
    }
}

void LonDevice_c::CheckOutputStates(void)
{

  uint8_t code;
  if(GetDevType() == PWM)
  {

    uint8_t buffer[] = { FRAME_READ_IO_EXT<<4 | 5, sAdr,0};

    trafProc_p->SendFrame(this,3,buffer);
  }
  else
  {

    uint8_t buffer[] = { FRAME_READ_IO<<4 | 4, sAdr};

    trafProc_p->SendFrame(this,2,buffer);
  }


}

uint8_t LonPortDataHigro_c::GetType(void ) 
{ 
  LonDatabase_c database;
  

  uint8_t type = 0;

  LonConfigPage_st* config = database.ReadConfig(dev_p->GetLAdr());

  if(config != NULL)
  {
    type = config->action[portNo].portData.type;

    if(type > 1) { type = 0; }
  }
  return type; 
}

void LonPortDataHigro_c::SetMeasurement(const uint8_t* buffer)
{
  int16_t tVar,hVar;
  float tVar_f,hVar_f;
  uint8_t helpVar;
  if( GetType() == 0 )
  {
    hVar = 10*buffer[0];    
    tVar = 10*buffer[2];    
  }
  else if( GetType() == 1 )
  {
    hVar = (buffer[0]<<8) | buffer[1];

    if(buffer[2] & 0x80 )
    {
      helpVar = buffer[2];
      helpVar &= ~0x80;
      tVar = ((helpVar<<8) | buffer[3]);
      tVar = -tVar;
    }
    else
    {
      tVar = ((buffer[2]<<8) | buffer[3]);

    }
  }
  else
  {
    return;
  }
  //printf("tVar=%d, hVar=%d\n",tVar,hVar);

  hig[nextIdx] = hVar;
  term[nextIdx] = tVar;
  nextIdx++;
  if(nextIdx >= SENSOR_ARRAY_SIZE) { nextIdx = 0; }
}
float LonPortDataHigro_c::GetSensorValue(uint8_t subPort)
{
  float ret = 0;
  int noOfProbes = 0;
  for(int i =0; i<SENSOR_ARRAY_SIZE;i++)
  {
    int16_t probe = subPort==0 ? hig[i] : term[i];
    if(probe > -1000)
    {
      ret+=probe;
      noOfProbes++;
    }
  }

  if(noOfProbes>0)
  {
    return 0.1*ret/noOfProbes;
  }
  else
  {
    return -100.0;
  }
}

void LonPortDataPress_c::SetMeasurement(const uint8_t* buffer)
{
  uint32_t recValue = (buffer[0]<<8) | buffer[1];

  float pressure = (recValue * 1111);
  pressure /= 65536.0;
  pressure += 105.4;
  pressure *=10;
  recValue = (uint32_t) pressure;

  //printf("rec press data = %d\n",recValue);

  press[nextIdx] = recValue;
  nextIdx++;
  if(nextIdx >= SENSOR_ARRAY_SIZE) { nextIdx = 0; }
}
float LonPortDataPress_c::GetSensorValue(uint8_t subPort)
{
  int32_t ret = 0;
  int noOfProbes = 0;
  for(int i =0; i<SENSOR_ARRAY_SIZE;i++)
  {
    int32_t probe = press[i];
    if(probe > -1000)
    {
      ret+=probe;
      noOfProbes++;
    }
  }

  if(noOfProbes>0)
  {
    LonDatabase_c database;
    float pressure = 0.1*ret/noOfProbes;
    float altitude = 0;
    float temperature = 20;

    LonConfigPage_st* config = database.ReadConfig(dev_p->GetLAdr());

    if(config != NULL)
    {
      altitude = config->action[portNo].portData.altitude;      
      uint32_t tLadr = config->action[portNo].portData.out1LAdr;
      uint8_t tPort = config->action[portNo].portData.out1Port;

      LonDevice_c* termDev_p = LonDevice_c::trafProc_p->GetDevByLadr(tLadr);
      if(termDev_p!= nullptr)
      {
        float tmp =  ((LonPortDataHigro_c*)(termDev_p->GetPortData(tPort)))->GetSensorValue(1);
        if((tmp > -64.0) && (tmp < 64.0))
        {
          temperature = tmp;
        }
      }
    }

    pressure = GetReducedPressure(pressure,altitude,temperature);


    return pressure;
  }
  else
  {
    return -100.0;
  }

}

float LonPortDataPress_c::GetReducedPressure(float absPressure, float altitude, float temperature)
{


  float reducedPressure = absPressure;
  float tempK = temperature + 273.15;

  reducedPressure = absPressure * expf( (0.0289644 * 9.806 * altitude) / (8.314462 * tempK)  );

  //printf("GetReducedPressure, presIn=%.1f, alt=%.1f temp=%.1f presOut=%.1f\n", absPressure, altitude ,temperature, reducedPressure);

  



  return reducedPressure;
}


uint32_t LonDevice_c::get_u32(uint8_t* buf, uint8_t pos)
{
   uint32_t data = 0;
   for(int i = 0;i<4;i++) 
   {
      data <<= 8;
      data |= buf[pos];
      pos++;
   }
   return data;
}

uint16_t LonDevice_c::get_u16(uint8_t* buf, uint8_t pos)
{
   uint16_t data = 0;
   for(int i = 0;i<2;i++) 
   {
      data <<= 8;
      data |= buf[pos];
      pos++;
   }
   return data;
}

uint8_t LonDevice_c::get_u8(uint8_t* buf, uint8_t pos)
{
   return buf[pos];
}

devType_et LonDevice_c::GetDevType(uint32_t lAdr)
{
  devType_et devType = UNN;
  uint8_t prefix = lAdr >> 24;
  for(int i = 0; i < DEV_TYPES_NO; i++)
  {
    if(devPrefix[i] == prefix)
    {
      devType = (devType_et) i;
      break;
    }
  }
  return devType;

}

uint8_t LonDevice_c::GetPortType(uint32_t lAdr,uint8_t port)
{
  devType_et devType = GetDevType(lAdr);
  uint8_t pMasc = portMasc[devType][port];

  return (pMasc>>4);
}

const char* LonDevice_c::GetDevTypeString(uint32_t lAdr)
{
  devType_et devType = GetDevType(lAdr);
  return devTypeString[devType];
}

#if LON_USE_COMMAND_LINK == 1
void LonDevice_c::GetDevInfo(LonGetDevInfo_c* recSig_p)
{
  recSig_p->lAdr = lAdr;
  recSig_p->result = true;
  recSig_p->sAdr = GetSadr();
  recSig_p->status = GetState();
  for(int i = 0; i<8; i++)
  {
    
    uint16_t portSize;

    GetValues(i,&recSig_p->value[i], &recSig_p->value2[i]);

    
  }
}
#endif



void LonDevice_c::GetValues(uint8_t port, float* val1, float* val2)
{
  uint32_t val;
  uint8_t portType = portMasc[GetDevType()][port];
    portType >>=4;

    switch(portType)
    {
      case 0: 
      /*sw*/
      break;
      case 1: 
      /*rel*/
      val = GetOutputVal(port);
      *val1 = (float)val;
      break;
      #if LON_USE_SENSORS == 1
      case 2: 
      /*hig*/
      *val1 = ((LonPortDataHigro_c*)GetPortData(port))->GetSensorValue(0);
      *val2 = ((LonPortDataHigro_c*)GetPortData(port))->GetSensorValue(1);
      break;
      case 3: 
      /*press*/
      *val1 = ((LonPortDataPress_c*)GetPortData(port))->GetSensorValue(0);
      break;
      #endif
      case 4: 
      /*pwm*/
      val = GetOutputVal(port);
      *val1 = (float)val;
      break;
      default:
      *val1 = 0;
      //portData_p[i] = NULL;
      break;
    }
}


bool LonDevice_c::IsEnabled(uint8_t port)
{
  LonDatabase_c database;
  

  uint8_t enabled = 0;

  LonConfigPage_st* config = database.ReadConfig(GetLAdr());

  if(config != NULL)
  {
    enabled = config->enabled;
  }
  return (enabled & (1<<port)) > 0;
  
}

void LonDevice_c::UpdateCheckTime(void)
{
  #if CONF_USE_TIME == 1
  TimeUnit_c::GetSystemTime(&lastCheckTime);
  #endif

}