#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "LonDataDef.hpp"
#include "SignalList.hpp"

#include "LonChannel.hpp"
#include "LonTrafficProcess.hpp"

LonChannel_c::LonChannel_c(uint8_t chNo_,LonTrafficProcess_c* trafProc_p_): chNo(chNo_), trafProc_p(trafProc_p_)
{
  state = NEW;

  timeoutActive = false;
  timeoutCnt = 0;

  for(int i=0;i<NO_OF_DEV_IN_CHANNEL;i++)
  {
    devList[i] = NULL;
  }

  noOfScannedDevices = 0;
  noOfSearchedDevices = 0;
  noOfLostDevices = 0;
  noOfFoundDevices = 0;
}

void LonChannel_c::CheckTimeout(void)
{
  if(timeoutActive == true)
  {
    
    if(timeoutCnt == 0)
    {
      timeoutActive = false;
      switch(state)
      {
        case NEW:
          SendDetach();
        break;
        case INITIATING:
          SearchNewDevices(true);
        break;
        case ADR_CONQ_SENT_INITIAL:
          state = INITIATING;
          SearchNewDevices(true);
        break;
        case ADR_CONQ_SENT:
          state = RUNNING;
        break;
        case CONFIG_SENT_INITIAL:
          state = INITIATING;
          SearchNewDevices(true);          
        break;
        case CONFIG_SENT:
          state = RUNNING;
        break;
        default:
        break;


      }
      
    }
    else
    {
      timeoutCnt--;
    }

  }

}

void LonChannel_c::StartTimeout(void)
{
  timeoutActive = true;
  timeoutCnt = TIMEOUT_CNT_START;
}
void LonChannel_c::StopTimeout(void)
{
  timeoutActive = false;
  timeoutCnt = 0;
}

LonDevice_c* LonChannel_c::SearchDevice(uint32_t lAdr)
{
  for(int i=1;i<NO_OF_DEV_IN_CHANNEL;i++)
  {
    if((devList[i] != NULL ) && (devList[i]->GetLAdr() == lAdr))
    {
      return devList[i];
    }
  }
  return NULL;
}


void LonChannel_c::RunForAll(Function_type function, void* userPtr)
{
  for(int i=1;i<NO_OF_DEV_IN_CHANNEL;i++)
  {
    if((devList[i] != NULL ))
    {
      function(devList[i],userPtr);
    }
  }

}

#if LON_USE_COMMAND_LINK == 1
void LonChannel_c::GetDevList(LonGetDevList_c* recSig_p)
{

  recSig_p->chNo = chNo;
  recSig_p->count = 0;
  for(int i=1;i<NO_OF_DEV_IN_CHANNEL;i++)
  {
    if((devList[i] != NULL ))
    {
      recSig_p->lAdr[recSig_p->count] = devList[i]->GetLAdr();
      recSig_p->sAdr[recSig_p->count] = devList[i]->GetSadr();
      recSig_p->status[recSig_p->count] = devList[i]->GetState();
      recSig_p->count++;
    }
  }
}
#endif

LonDevice_c* LonChannel_c::GetDevice(uint8_t sAdr) 
{
  if( sAdr < NO_OF_DEV_IN_CHANNEL)
  {
    return devList[sAdr];
  }
  return NULL;

}


LonDevice_c* LonChannel_c::CreateNewDevice(uint32_t lAdr,LonTrafficProcess_c* trafProc_p)
{
  LonDevice_c* dev_p = NULL; 
  for(int i=1;i<NO_OF_DEV_IN_CHANNEL;i++)
  {
    if(devList[i] == NULL )
    {
      dev_p = new LonDevice_c(lAdr,chNo,i);
      devList[i] = dev_p;
      #if LON_USE_VIRTUAL_CHANNELS == 1
      if(dev_p->GetDevType() == VIR)
      {
        LonChannelVirtual_c* ch_p;
        ch_p = trafProc_p->AssignNewVirtualChannel(dev_p);
        LonPortDataVirtualChannel_c* portData =  (LonPortDataVirtualChannel_c*)(dev_p->GetPortData(0));
        portData->channel_p = ch_p;
      }
      #endif
      break; 
    }
  }
  noOfFoundDevices++;
  return dev_p;

}

void LonChannel_c::DeleteDevice(LonDevice_c* dev_p)
{  
  for(int i=1;i<NO_OF_DEV_IN_CHANNEL;i++)
  {
    if(devList[i] == dev_p )
    {      
      devList[i] = NULL;       
    }
  }
}

void LonChannel_c::ReceiveDetachAck(void)
{
  state = INITIATING;
  SearchNewDevices(true);
}

void LonChannel_c::SearchNewDevices(bool force)
{
  if((adrConqTimer == 0) || (force))
  {
    if((state == RUNNING) || (state = INITIATING))
    {      
      SendAdrConquest();
      adrConqTimer = ADR_CONQ_INTERVAL;

      if(state == INITIATING) 
      {
        state = ADR_CONQ_SENT_INITIAL;
      }
      else if(state == RUNNING) 
      {
        state = ADR_CONQ_SENT;
      }
    }
  }
  else
  {
    adrConqTimer--;
  }
}

void LonChannel_c::HandleDeviceFound(uint32_t lAdr)
{
  if(lAdr == 0)
  {
    /*bad frame */
    SearchNewDevices(true);
  }
  else if(lAdr == 0xFFFFFFFF)
  {
    /* no device found */
    state = RUNNING;
  } 
  else
  {
    /*device found */
    LonDevice_c* dev_p = trafProc_p->GetDevByLadr(lAdr);
    if(dev_p == NULL)
    {
      dev_p = CreateNewDevice(lAdr,trafProc_p);
    } 
    else if(dev_p->GetChNo() != chNo)
    {
      //printf("chNo mismatch, act = %d\n",dev_p->GetChNo());      
      trafProc_p->DeleteDevice(dev_p);
      //DeleteDevice(dev_p);
      dev_p = CreateNewDevice(lAdr,trafProc_p);
    }
    else
    {
      //printf("already exists\n");
    }    
    ConfigDevice(dev_p);
        
    if(state == ADR_CONQ_SENT)
    {
      state = CONFIG_SENT;
    }
    else
    {
      state = CONFIG_SENT_INITIAL;
    }


  }
}

void LonChannel_c::ConfigDevice(LonDevice_c* dev_p)
{
  SendDevConfig(dev_p);
  if(state == ADR_CONQ_SENT_INITIAL)
  {
    state = CONFIG_SENT_INITIAL;
  }
  else
  {
    state = CONFIG_SENT;
  }
}

void LonChannel_c::ConfirmDeviceConfig(uint32_t lAdr,bool result)
{
  if(result == true)
  {
    LonDevice_c* dev_p = SearchDevice(lAdr);
    if(dev_p != NULL)
    {
      dev_p->ConfirmConfig();
      dev_p->CheckOutputStates();
    }
  }

  if(state == CONFIG_SENT_INITIAL)
  {
    state = INITIATING;
    SearchNewDevices(true); /* state = ADR_CONQ_SENT or ADR_CONQ_SENT_INITIAL */
  }
  else
  {
    state = RUNNING;
  }
}

void LonChannel_c::CheckDevFromConfigAck(uint32_t lAdr,bool result)
{
  LonDevice_c* dev_p =  trafProc_p->GetDevByLadr(lAdr);

  if(dev_p !=NULL) /* device already defined */
  {
    if(result == true) /* device has been found in current channel */
    {
      if(dev_p->GetChNo() != chNo) /* device defined in another channel */
      { 
        trafProc_p->DeleteDevice(dev_p);
        dev_p = CreateNewDevice(lAdr,trafProc_p);    
        ConfigDevice(dev_p);       
      }
      else
      {
        ConfigDevice(dev_p);
      }
      dev_p->timeToNextScan=CHECK_ONCE_DEV_INTERVAL;
    }
    else
    {
      if(dev_p->GetChNo() == chNo)
      {
        dev_p->SetLost();
        noOfLostDevices++;
      }
    }
    dev_p->CheckTMOConsistency(trafProc_p);
  }
  else
  {
    if(result  == true)
    {
      dev_p = CreateNewDevice(lAdr,trafProc_p);    
      ConfigDevice(dev_p);
      dev_p->timeToNextScan = CHECK_ONCE_DEV_INTERVAL;
      
    }
  }
}

void LonChannel_c::HandleDataTrAck(LonDevice_c* dev_p, uint8_t port, uint8_t value)
{
  dev_p->SetOutputConfirm(port,value);
  uint32_t lAdr = dev_p->GetLAdr();
  


}

void LonChannel_c::HandleIoReadAck(LonDevice_c* dev_p, uint8_t frameCode, uint8_t* ioData)
{

  uint8_t value = 0;

  if(frameCode == FRAME_READ_IO)
  {
    value = ioData[0];    
  }
  else if(frameCode == FRAME_READ_IO_EXT)
  {
    value = ioData[3]; 
  }


  for(int i =0;i<8;i++)
  {
    uint8_t pType = portMasc[dev_p->GetDevType()][i];
    uint8_t phyPortIdx = pType & 0x0F;
    uint8_t portVal = 0;
    bool portValid = false;
    if((pType & 0xF0) == 0x10)
    {    
      if(value & (1<<phyPortIdx))
      {
        portVal = 1;
      }
      portValid = true;
    }
    else if(((pType & 0xF0) == 0x40) && (frameCode == FRAME_READ_IO_EXT))
    {
      uint8_t idx = phyPortIdx-11;
      portVal = ioData[idx];
      portValid = true;
    }
    if(portValid)
    {
      dev_p->SetOutputConfirm(i,portVal);

    }
    dev_p->SetOutputFinish();

  }

}

