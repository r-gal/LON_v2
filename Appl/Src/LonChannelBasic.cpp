#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "LonDataDef.hpp"
#include "SignalList.hpp"

#include "LonTrafficProcess.hpp"

#include "LonChannel.hpp"
#include "LonChannelBasic.hpp"

#if LON_USE_VIRTUAL_CHANNELS == 1
#include "LonChannelVirtual.hpp"
#endif


LonChannelBasic_c::LonChannelBasic_c (uint8_t chNo_,LonTrafficProcess_c* trafProc_p_) : LonChannel_c(chNo_,trafProc_p_)
{
}

void LonChannelBasic_c::HandleReceivedFrame(LonIOdata_c* recSig_p)
{
  uint8_t chNo = recSig_p->GetChNo();
  frameCode_et frameCode = (frameCode_et)(recSig_p->data[1]>>4);


  switch(frameCode)
  {
    case FRAME_ADRCONQUEST: /* ACK */
    {
      StopTimeout();
      uint32_t lAdr = (recSig_p->ack[0]<<24)|(recSig_p->ack[1]<<16)|(recSig_p->ack[2]<<8)|(recSig_p->ack[3]);    
      IOact_et ack = recSig_p->ackOk;
      if(lAdr != 0xFFFFFFFF)
      {
        //printf("CH_%d found dev 0x%08X, ack=%d\n",chNo,lAdr,ack);
      } 
      if(ack == IOACK_NOK) 
      {
        lAdr = 0;
      }
      HandleDeviceFound(lAdr);
      break;
    }
    case FRAME_DETACH_ALL:
    {
      StopTimeout();
      ReceiveDetachAck();
      break;
    }
    case FRAME_DEVCONFIG: /* ACK*/
    {
      StopTimeout();
      uint32_t lAdr =  (recSig_p->data[2]<<24) | (recSig_p->data[3]<<16) | (recSig_p->data[4]<<8) | (recSig_p->data[5]); 
      IOact_et ack = recSig_p->ackOk;
      bool result = false;
      if(ack == IOACK_OK) 
      {
        result = true;
      }
      ConfirmDeviceConfig(lAdr,result);
      break;
    }    
    case FRAME_CHECKDEVICE:
    {
      StopTimeout();
      uint32_t lAdr =  (recSig_p->data[2]<<24) | (recSig_p->data[3]<<16) | (recSig_p->data[4]<<8) | (recSig_p->data[5]);     
      IOact_et ack = recSig_p->ackOk;
      bool result = false;
      if(ack == IOACK_OK) 
      {
        result = true;
      }
      CheckDevFromConfigAck(lAdr,result);
      break;
    }
    case FRAME_DATA_SW:
    {
      uint8_t sAdr = recSig_p->data[0];
      LonDevice_c* dev_p = GetDevice(sAdr);
      if(dev_p != NULL)
      {
        uint8_t noOfEvents = (recSig_p->data[1] & 0x0F) - 3;
        trafProc_p->HandleDataSw(dev_p,noOfEvents,recSig_p->data + 2);
      }      
      break;
    }
    case FRAME_DATA_TR:
    {
      if(recSig_p->ackOk == IOACK_OK)
      {
        uint8_t sAdr = recSig_p->data[2];
        uint8_t phyPort = recSig_p->data[3];
        uint8_t value = recSig_p->data[4];    
        LonDevice_c* dev_p = GetDevice(sAdr);
        uint8_t port = dev_p->phyPortToPort(phyPort);
        
        if(dev_p != NULL)
        {
          HandleDataTrAck(dev_p,port,value); 
        }
      }
      break;
    }

    case FRAME_READ_IO:
    case FRAME_READ_IO_EXT:
    {
      if(recSig_p->ackOk == IOACK_OK)
      {
        uint8_t sAdr = recSig_p->data[2];
        LonDevice_c* dev_p = GetDevice(sAdr);
        if(dev_p != NULL)
        {
          HandleIoReadAck(dev_p,frameCode,recSig_p->ack);
        }
      }
      break;
    }





    case FRAME_DATA:
    {
      LonDevice_c* dev_p;
      uint8_t sAdr = recSig_p->data[0];
      uint8_t port = recSig_p->data[3];
      dev_p = GetDevice(sAdr);   
      if((recSig_p->usedIndicator == 1) && (dev_p != NULL))
      {
        #if LON_USE_SENSORS == 1
        trafProc_p->sensors->HandleSensorData(dev_p,port, recSig_p->data + 4 );
        #endif
      }
      break;
    }
    case FRAME_CONTAINER:
    {
      RouteFrame(recSig_p);
      break;
    }
    default:
    break;

  }

}

void LonChannelBasic_c::RouteFrame(LonIOdata_c* recSig_p)
{
  uint8_t sAdr = recSig_p->data[0];
  if(sAdr != 0)
  {
    LonDevice_c* dev_p = GetDevice(sAdr);
    if(dev_p != NULL)
    {
      #if LON_USE_VIRTUAL_CHANNELS == 1
      if(dev_p->GetDevType() == VIR)
      {
        LonChannelVirtual_c* chV_p;
        chV_p = ((LonPortDataVirtualChannel_c*)(dev_p->GetPortData(0)))->channel_p;
        chV_p->HandleReceivedFrame(recSig_p);
      }
      #endif
    }

  }
}



void LonChannelBasic_c::SendAdrConquest(void)
{

  LonIOdata_c* ioSig_p;
  ioSig_p = new LonIOdata_c;

  ioSig_p->data[0] = 0;
  ioSig_p->data[1] = FRAME_ADRCONQUEST<<4 | 0x03;
  ioSig_p->calculateCS(2);
  ioSig_p->SetChNo(chNo);
  ioSig_p->translate(SIGNO_LON_IODATAOUT);
  ioSig_p->Send();

  //printf("search devices, chNo=%d\n",chNo);
  StartTimeout();
}

void LonChannelBasic_c::SendCheckDevice(uint32_t lAdr)
{
  LonIOdata_c* ioSig_p;
  ioSig_p = new LonIOdata_c;

  ioSig_p->data[0] = 0;
  ioSig_p->data[1] = FRAME_CHECKDEVICE<<4 | 0x07;
  ioSig_p->data[2] = (lAdr>>24)&0xFF;
  ioSig_p->data[3] = (lAdr>>16)&0xFF;
  ioSig_p->data[4] = (lAdr>>8)&0xFF;
  ioSig_p->data[5] = (lAdr>>0)&0xFF;
  ioSig_p->calculateCS(6);
  ioSig_p->SetChNo(chNo);
  ioSig_p->translate(SIGNO_LON_IODATAOUT);
  ioSig_p->Send();

  StartTimeout();
}

void LonChannelBasic_c::SendDevConfig(LonDevice_c* dev_p)
{
  uint8_t confBuffer[10];
  dev_p->GetConfig(confBuffer);

  LonIOdata_c* confSig;
  confSig = new LonIOdata_c;

  confSig->data[0] = 0;
  confSig->data[1] = FRAME_DEVCONFIG<<4 | 0x0D;

  for(int i=0;i<10;i++)
  {
    confSig->data[i+2] = confBuffer[i];
  } 

  confSig->calculateCS(12);
  confSig->SetChNo(chNo);
  confSig->translate(SIGNO_LON_IODATAOUT);

  StartTimeout();
/*
  printf("BCH send config frame: ");
  for(int i =0; i< MAX_MESSAGE_SIZE; i++)
  {
    printf("%02X,",confSig->data[i]);
  }
  printf("\n");*/
  confSig->Send();
}

void LonChannelBasic_c::SendDetach(void)
{
  LonIOdata_c* testSig;
  testSig = new LonIOdata_c;

  testSig->data[0] = 0;
  testSig->data[1] = FRAME_DETACH_ALL<<4 | 0x03;
  testSig->calculateCS(2);
  testSig->SetChNo(chNo);
  testSig->translate(SIGNO_LON_IODATAOUT);
  testSig->Send();

  StartTimeout();
}

void LonChannelBasic_c::SendFrame(LonDevice_c* dev_p, uint8_t bytesNo, uint8_t* buffer)
{

  LonIOdata_c* sig;
  sig = new LonIOdata_c;
 
  sig->data[0] = 0;

  for(int i=0;i<bytesNo;i++)
  {
    sig->data[1+i] = buffer[i];
  }
  sig->calculateCS(1+bytesNo);
  sig->SetChNo(chNo);
  sig->translate(SIGNO_LON_IODATAOUT);
  sig->Send();

}