#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "GeneralConfig.h"

#include "LonInterfaceProcess.hpp"
#include "LonInterfacePhy.hpp"


LonInterfaceProcess_c::LonInterfaceProcess_c(uint16_t stackSize, uint8_t priority, uint8_t queueSize, HANDLERS_et procId) : process_c(stackSize,priority,queueSize,procId,"LON_ITFC")
{

  #ifdef HW_VER2
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOH, ENABLE);
  #endif
  
  for(uint8_t i=0;i<NO_OF_BASIC_CHANNELS;i++)
  {
    dev_phy_p[i] = new LonDeviceInterface_c( i);
  }


  LonInterfacePhy_c::InitInterfaceTick();


}

void LonInterfaceProcess_c::main(void)
{


  while(1)
  {
    releaseSig = true;
    RecSig();
    //RestartData_c::ownPtr->signalsHandled[HANDLE_PHY]++;
    switch(recSig_p->GetSigNo())
    {
      /*case SIGNO_WDT_TICK:
        releaseSig = false;
        break;*/
      case SIGNO_LON_INTERFACE_TICK:
          //IWDG_ReloadCounter();

          for(uint8_t i = 0; i<NO_OF_BASIC_CHANNELS;i++)
          {
            dev_phy_p[i]->proceed();
          }
          releaseSig = false;
          break;
      default:
      break;

    }
    if(releaseSig) { delete  recSig_p; }
 
  }


}



LonDeviceInterface_c::LonDeviceInterface_c(uint8_t portNo_): LonInterfacePhy_c(portNo_)
{


  ioState = IO_IDLE;



  setClk(UP);
  setDat(UP);

  actFrame = NULL;
  emptyFrame = NULL;
  TXblocked = false;

  QueueHandle_t QueueHandle;
  QueueHandle = xQueueCreate( DEV_PHY_PROCESS_QUEUE_SIZE,sizeof(Sig_c*) );
  setHandler((HANDLERS_et)(HANDLE_LON_PHY_0+portNo_),QueueHandle);
}

void LonDeviceInterface_c::setClk(bool data)
{
  /* write IO input port */
  setClkPhy(data);
  clk = data;
}

void LonDeviceInterface_c::setDat(bool data)
{
  /* write IO input port */
  setDatPhy(data);
  dat = data;
}

bool LonDeviceInterface_c::checkClk(void) 
{
  if(clk == getClk())
  {
    return true;
  }
  else
  {
    return false;
  }
} 

bool LonDeviceInterface_c::checkDat(void)
{
  return(dat == getDat());
}

bool LonDeviceInterface_c::getBit(void)
{
  int8_t byte = actFrame->data[byteCnt];
  bool ret = (byte & (0x80>>bitCnt));
  if(bitCnt==7)
  {
    bitCnt = 0;
    byteCnt++;
  }
  else
  {
    bitCnt++;
  }
  return ret;
}


void LonDeviceInterface_c::setBit(bool bitVal)
{
  if(bitVal) 
  {
    actFrame->data[byteCnt] |= (0x80>>bitCnt);
  }
  else
  {
    actFrame->data[byteCnt] &= ~(0x80>>bitCnt);
  } 
  if(bitCnt==7)
  {
    bitCnt = 0;
    checkSum ^= actFrame->data[byteCnt];
    if(byteCnt == 1) /*code has been received */
    {
      bytesToTransmit = actFrame->data[1] & 0x0F;
      if(bytesToTransmit > MAX_MESSAGE_SIZE) {bytesToTransmit = MAX_MESSAGE_SIZE;}
    }
    byteCnt++;        
  }
  else
  {
    bitCnt++;
  }
}


void LonDeviceInterface_c::proceed(void)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE; 
 
  switch(ioState)
  {
    case IO_IDLE:

      if(TXblocked == true)
      {
        if(emptyFrame == NULL)
        {
          Sig_c* recsig_p = new LonIOdata_c;
          if(recsig_p != NULL)
          {
            actFrame = (LonIOdata_c*)recsig_p;
            actFrame->SetChNo(portNo);
            TXblocked = false;
            ioState = IO_STARTING;
          }
        }
        else
        {
          actFrame = emptyFrame;
          emptyFrame = NULL;
          TXblocked = false;
          ioState = IO_STARTING;
        }

      }
      else
      {
        Sig_c* recsig_p;
        if(CheckSig(&recsig_p,(HANDLERS_et)(HANDLE_LON_PHY_0 + portNo)) == pdTRUE )
        {
          actFrame = (LonIOdata_c*)recsig_p;
          actFrame->usedIndicator = 1;
          TXblocked = true;
          ioState = IO_STARTING;
        }
        else
        {
          if(emptyFrame == NULL)
          {
            Sig_c* recsig_p = new LonIOdata_c;
            
            if(recsig_p != NULL)
            {
              actFrame = (LonIOdata_c*)recsig_p;
              actFrame->SetChNo(portNo);
              TXblocked = false;
              ioState = IO_STARTING;
            }
          }
          else
          {
            actFrame = emptyFrame;
            emptyFrame = NULL;
            TXblocked = false;
            ioState = IO_STARTING;
          }

        }
      }
      break;
    case IO_STARTING:

      if(checkClk())
      {
        setDat(DOWN); /* trigger START bit */
        ioState = IO_START0;
      }
      break;
    case IO_START0:
      setClk(DOWN);
      ioState = IO_START1;
      break;
    case IO_START1:
      byteCnt = 0;
      bitCnt = 0;
      if(actFrame->usedIndicator == 1)
      {
        /*send frame */
        bytesToTransmit = actFrame->data[1] &0x0F;
        ioState = IO_TR0;
        setDat(getBit());
      }
      else
      {
        /*wait for frame */
        bytesToTransmit = MAX_MESSAGE_SIZE; /* set to max, will be
        updated after byte 1 has been received */
        setDat(true);/*set as input*/
        checkSum = 0;
        ioState = IO_RC0;
      }
      break;
    case IO_TR0:
      ioState = IO_TR1;
      setClk(UP);
      break;
    case IO_TR1:
      if(checkClk())
      {
        setClk(DOWN);
        if(byteCnt >= bytesToTransmit )
        {
          ioState = IO_TRACK0;
          setDat(UP); /*start receiving */
          bitCnt = 0;
          byteCnt = 0;
          checkSum = 0;
          switch(actFrame->data[1] >>4)
          {
            case FRAME_ADRCONQUEST: 
            case FRAME_READ_IO_EXT:
              ackSize = 5;
              break;
            case FRAME_READ_IO:
              ackSize = 2;
              break;
            case FRAME_DEVCONFIG :
            case FRAME_CHECKDEVICE :
            case FRAME_DATA_TR :
            case FRAME_CONTAINER:
            default:
              ackSize = 1;
            break;
          }                        
        }
        else
        {
          setDat(getBit());
          ioState = IO_TR0;
        }        
      }
      break;
    case IO_RC0:
      setClk(UP);
      setBit(getDat());
      ioState = IO_RC1;
      break;
    case IO_RC1:
      if(checkClk())
      {
        if(byteCnt >= bytesToTransmit )
        {
          /*here ACK should be set*/
          if(checkSum == 0)
          {
            actFrame->ack[0] = ACK_NORMAL;
          }
          else
          {
            actFrame->ack[0] = ACK_INVALID_CHECKSUM;
          }
          actFrame->ack[0] |= (actFrame->ack[0]<<4); /*simple CS */          
          bitCnt = 0;
          ioState = IO_RCACK1;
        }
        else if(byteCnt == 1)
        {
          if(actFrame->data[0] == 0xFF)
          {
            /*no transmision, return to idle */
            emptyFrame = actFrame;
            ioState = IO_IDLE;
            setDat(UP);
            setClk(UP);
            break;
          }
          else
          {
            ioState = IO_RC0;
          }
        }
        else if(byteCnt == 2)
        {
            bytesToTransmit = actFrame->data[1] & 0x0F;
            ioState = IO_RC0;
        }
        else
        {
          ioState = IO_RC0;
        }
      setClk(DOWN);
      }
      break;
    case IO_TRACK0:
      setClk(UP);
      /* read ACK bit */
      if(getDat())
      {
        actFrame->ack[byteCnt] |= (0x80>>bitCnt);
      }
      else
      {
        actFrame->ack[byteCnt] &= ~(0x80>>bitCnt);
      } 
      bitCnt++;
      ioState = IO_TRACK1;
      break;
    case IO_TRACK1:
      if(checkClk())
      {
        setClk(DOWN);
        if(bitCnt > 7)
        {
          byteCnt++;
          bitCnt = 0;
          if(byteCnt == ackSize)
          {
            actFrame->usedIndicator = 2;
            ioState = IO_END0;
            setDat(DOWN);
          }
          else
          {
            ioState = IO_TRACK0;
          } 
        }
        else
        {
          ioState = IO_TRACK0;
        }        
      }
      break;
    case IO_RCACK0:
      setClk(UP);
      ioState = IO_RCACK1;
      break;
    case IO_RCACK1:
      if(checkClk())
      {
        setClk(DOWN);
        if(bitCnt<8)
        {
          setDat(actFrame->ack[0] & (0x80 >> bitCnt));
          bitCnt++;
          ioState = IO_RCACK0;
        }
        else
        {
          setDat(DOWN);
          if(checkSum == 0)
          {
            actFrame->usedIndicator = 1;
          }
          ioState = IO_END0;
        }        
      }
      break;
    case IO_END0:
      setClk(UP);
      ioState = IO_END1;
      break;
    case IO_END1:
      if(checkClk())
      {
        ioState_et nextState = IO_IDLE;
        setDat(UP); /*trigger STOP bit */
        if(actFrame->usedIndicator == 1)
        {
          uint8_t frameCode = (actFrame->data[1]>>4);
          switch(frameCode)
          {
            case FRAME_ADRCONQUEST:
            case FRAME_DEVCONFIG:
            case FRAME_CHECKDEVICE:
            case FRAME_READ_STATUS:
            case FRAME_DETACH_ALL:
            default:
              actFrame->translate(SIGNO_LON_IODATAIN_CTRL);
              break;
            case FRAME_DATA_SW:
            case FRAME_DATA_TR:
            case FRAME_READ_IO:
            case FRAME_REQUEST:
            case FRAME_DATA:
            case FRAME_READ_IO_EXT:
            case FRAME_CONTAINER:
              actFrame->translate(SIGNO_LON_IODATAIN_TRAF);
              break;
          }
          actFrame->Send();
          actFrame = NULL;
        }
        else if(actFrame->usedIndicator == 2)
        {
          /*check ACK checksum */
          actFrame->ackOk = actFrame->checkACKCS(LonIOdata_c::getAckLen( (frameCode_et)(actFrame->data[1] >>4))) ;
          if(actFrame->ackOk == IOACK_NOK)
          {
            if(actFrame->triesCount>0)
            {
              actFrame->triesCount--;
              actFrame->usedIndicator = 1;
              nextState = IO_STARTING;
            }
            else
            {
              uint8_t frameCode = (actFrame->data[1]>>4);
              switch(frameCode)
              {
                case FRAME_ADRCONQUEST:
                case FRAME_DEVCONFIG:
                case FRAME_CHECKDEVICE:
                case FRAME_READ_STATUS:
                case FRAME_DETACH_ALL:
            
                  actFrame->translate(SIGNO_LON_IODATAIN_CTRL);
                  actFrame->Send();
                  actFrame = NULL;
                  break;
                default:
                case FRAME_DATA_SW:
                case FRAME_DATA_TR:
                case FRAME_READ_IO:
                case FRAME_REQUEST:
                case FRAME_DATA:
                case FRAME_READ_IO_EXT:
                case FRAME_CONTAINER:
                 // frame->translate(SIGNO_LON_IODATAIN_TRAF);
                  break;
              }
            }
          }
          else
          {

            uint8_t frameCode = (actFrame->data[1]>>4);
            switch(frameCode)
            {
              case FRAME_ADRCONQUEST:
              case FRAME_DEVCONFIG:
              case FRAME_CHECKDEVICE:
              case FRAME_READ_STATUS:
              case FRAME_DETACH_ALL:
              default:
                actFrame->translate(SIGNO_LON_IODATAIN_CTRL);
                break;
              case FRAME_DATA_SW:
              case FRAME_DATA_TR:
              case FRAME_READ_IO:
              case FRAME_REQUEST:
              case FRAME_DATA:
              case FRAME_READ_IO_EXT:
              case FRAME_CONTAINER:
                actFrame->translate(SIGNO_LON_IODATAIN_TRAF);
                break;
            }
            actFrame->Send();
            actFrame = NULL;
          }
        }
        else
        {
          emptyFrame = actFrame;
        }




        ioState = nextState;                    
      }
      break;      
    default:
      ioState = IO_IDLE;
      break;
    }
}
