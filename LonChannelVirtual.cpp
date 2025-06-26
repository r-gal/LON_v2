#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#if LON_USE_VIRTUAL_CHANNELS == 1

#include "LonDataDef.hpp"

#include "LonChannelVirtual.hpp"
#include "LonChannel.hpp"
#include "LonTrafficProcess.hpp"



LonChannelVirtual_c::LonChannelVirtual_c (uint8_t chNo_,LonDevice_c* dev_p_,LonTrafficProcess_c* trafProc_p_) : LonChannel_c(chNo_,trafProc_p_) ,ownerDevice_p(dev_p_)
{


}

LonChannelVirtual_c::~LonChannelVirtual_c(void)
{
  trafProc_p->CleanVirtualChannel(chNo);
}




void LonChannelVirtual_c::HandleReceivedFrame(LonIOdata_c* recSig_p)
{
  uint8_t frameCodeExt = recSig_p->data[1] >> 4;

  if(frameCodeExt == FRAME_CONTAINER)
  {
    enum FRAME_DIR_et
    {
      DIR_UL,
      DIR_DL
    } frameDir;

    if(recSig_p->data[0] == 0)
    {
      frameDir = DIR_UL;
    }
    else
    {
      frameDir = DIR_DL;
    }
    
    uint8_t frameCode = recSig_p->data[2] >> 4;

    if(frameDir == DIR_DL)
    {

      switch( frameCode)
      {
        case FRAME_ADRCONQUEST: /* ACK */
        {
          uint32_t lAdr = (recSig_p->data[3]<<24)|(recSig_p->data[4]<<16)|(recSig_p->data[5]<<8)|(recSig_p->data[6]); 
          //printf("VCH found lAdr = 0x%08X\n",lAdr);
          StopTimeout();
          HandleDeviceFound(lAdr);          
          break;
        }
        case FRAME_DETACH_ALL: /* ACK*/
        {
          StopTimeout();
          ReceiveDetachAck();
          break;
        }
        case FRAME_DEVCONFIG: /* ACK*/
        {
          StopTimeout();
          uint32_t lAdr = (recSig_p->data[3]<<24)|(recSig_p->data[4]<<16)|(recSig_p->data[5]<<8)|(recSig_p->data[6]); 
          bool result = recSig_p->data[7];
          ConfirmDeviceConfig(lAdr,result);
          break;
        }  
        case FRAME_CHECKDEVICE:
        {
          StopTimeout();
          uint32_t lAdr = (recSig_p->data[3]<<24)|(recSig_p->data[4]<<16)|(recSig_p->data[5]<<8)|(recSig_p->data[6]);   
          bool result = recSig_p->data[7];
          CheckDevFromConfigAck(lAdr,result);
          break;
        }
        case FRAME_DATA_SW:
        {
          uint8_t sAdr = recSig_p->data[3];
          LonDevice_c* dev_p = GetDevice(sAdr);
          if(dev_p != NULL)
          {
            uint8_t noOfEvents = (recSig_p->data[1] & 0x0F) - 5;
            trafProc_p->HandleDataSw(dev_p,noOfEvents,recSig_p->data + 4);
          }      
          break;
        }

        case FRAME_DATA_TR:
        {
          if(recSig_p->ackOk == IOACK_OK)
          {
            uint8_t sAdr = recSig_p->data[3];
            uint8_t phyPort = recSig_p->data[4];
            uint8_t value = recSig_p->data[5];    
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
            uint8_t sAdr = recSig_p->data[3];
            LonDevice_c* dev_p = GetDevice(sAdr);
            if(dev_p != NULL)
            {
              HandleIoReadAck(dev_p,frameCode,recSig_p->data+5);
            }
          }
          break;
        }

        case FRAME_DATA:
        {
          LonDevice_c* dev_p;
          uint8_t sAdr = recSig_p->data[3];
          uint8_t port = recSig_p->data[5];
          dev_p = GetDevice(sAdr);   
          if((recSig_p->ackOk == IOACK_OK) && (dev_p != NULL))
          {
            trafProc_p->sensors->HandleSensorData(dev_p,port, recSig_p->data + 6 );
          }
          break;
        }
   

        default:
        break;




      }
    }
  }

}

void LonChannelVirtual_c::SendAdrConquest(void)
{
  LonIOdata_c* ioSig_p;
  ioSig_p = new LonIOdata_c;

  ioSig_p->data[0] = 0;
  ioSig_p->data[1] = FRAME_CONTAINER<<4 | 0x5;
  ioSig_p->data[2] = ownerDevice_p->GetSadr();
  ioSig_p->data[3] = FRAME_ADRCONQUEST<<4;
  ioSig_p->calculateCS(4);
  ioSig_p->SetChNo(ownerDevice_p->GetChNo());
  ioSig_p->translate(SIGNO_LON_IODATAOUT);
  ioSig_p->Send();

  StartTimeout();
}



void LonChannelVirtual_c::SendDevConfig(LonDevice_c* dev_p)
{
  uint8_t confBuffer[10];
  dev_p->GetConfig(confBuffer);

  LonIOdata_c* confSig;
  confSig = new LonIOdata_c;

  confSig->data[0] = 0;
  confSig->data[1] = FRAME_CONTAINER<<4 | 0x0F;
  confSig->data[2] = ownerDevice_p->GetSadr();
  confSig->data[3] = FRAME_DEVCONFIG<<4 ;

  for(int i=0;i<10;i++)
  {
    confSig->data[i+4] = confBuffer[i];
  } 

  confSig->calculateCS(14);
  confSig->SetChNo(ownerDevice_p->GetChNo());
  confSig->translate(SIGNO_LON_IODATAOUT);
/*
    printf("VCH send config frame: ");
  for(int i =0; i< MAX_MESSAGE_SIZE; i++)
  {
    printf("%02X,",confSig->data[i]);
  }
  printf("\n");*/

  confSig->Send();

  StartTimeout();

}

void LonChannelVirtual_c::SendDetach(void)
{
  LonIOdata_c* ioSig_p;
  ioSig_p = new LonIOdata_c;

  ioSig_p->data[0] = 0;
  ioSig_p->data[1] = FRAME_CONTAINER<<4 | 0x5;
  ioSig_p->data[2] = ownerDevice_p->GetSadr();
  ioSig_p->data[3] = FRAME_DETACH_ALL<<4;
  ioSig_p->calculateCS(4);
  ioSig_p->SetChNo(ownerDevice_p->GetChNo());
  ioSig_p->translate(SIGNO_LON_IODATAOUT);
  ioSig_p->Send();

  StartTimeout();

}

void LonChannelVirtual_c::SendFrame(LonDevice_c* dev_p, uint8_t bytesNo, uint8_t* buffer)
{
  LonIOdata_c* ioSig_p;
  ioSig_p = new LonIOdata_c;

  ioSig_p->data[0] = 0;
  ioSig_p->data[1] = FRAME_CONTAINER<<4 | (4+bytesNo);
  ioSig_p->data[2] = ownerDevice_p->GetSadr();
  for(int i=0;i<bytesNo;i++)
  {
    ioSig_p->data[3+i] = buffer[i];
  }
  ioSig_p->calculateCS(3+bytesNo);
  ioSig_p->SetChNo(ownerDevice_p->GetChNo());
  ioSig_p->translate(SIGNO_LON_IODATAOUT);
  ioSig_p->Send();
}

void LonChannelVirtual_c::SendCheckDevice(uint32_t lAdr)
{
  LonIOdata_c* ioSig_p;
  ioSig_p = new LonIOdata_c;

  ioSig_p->data[0] = 0;
  ioSig_p->data[1] = FRAME_CONTAINER<<4 | 9;
  ioSig_p->data[2] = ownerDevice_p->GetSadr();
  ioSig_p->data[3] = FRAME_CHECKDEVICE<<4 | 0x07;
  ioSig_p->data[4] = (lAdr>>24)&0xFF;
  ioSig_p->data[5] = (lAdr>>16)&0xFF;
  ioSig_p->data[6] = (lAdr>>8)&0xFF;
  ioSig_p->data[7] = (lAdr>>0)&0xFF;
  ioSig_p->calculateCS(8);
  ioSig_p->SetChNo(ownerDevice_p->GetChNo());
  ioSig_p->translate(SIGNO_LON_IODATAOUT);
  ioSig_p->Send();

  StartTimeout();
}

#endif