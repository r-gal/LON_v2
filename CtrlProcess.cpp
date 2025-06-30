 #include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"


#include "CtrlProcess.hpp"
#if CONF_USE_RUNTIME == 1
#include "RunTimeStats.hpp"
#endif

#if CONF_USE_SDCARD == 1
#include "FileSystem.hpp"
#endif



void vFunction100msTimerCallback( TimerHandle_t xTimer )
{
  //dispActTimSig.Send();

}

CtrlProcess_c::CtrlProcess_c(uint16_t stackSize, uint8_t priority, uint8_t queueSize, HANDLERS_et procId) : process_c(stackSize,priority,queueSize,procId,"CONFIG")
{

  

}

void CtrlProcess_c::main(void)
{
  #if CONF_USE_RUNTIME == 1
  RunTime_c::Start();
  #endif
  #if CONF_USE_TIME == 1
  timeUnit.Init();
  #endif
  #if CONF_USE_BOOTUNIT == 1
  bootUnit.Init(TimeUnit_c::GetHrtc());
  #endif

  #if CONF_USE_ETHERNET == 1
  ethernet.Init();
  #endif

  #if CONF_USE_SDCARD == 1
  sdCard.Init();
  #endif

  #if CONF_USE_LOGGING == 1
  loggingUnit.Init();
  LoggingUnit_c::Log(LOG_FATAL_ERROR,"System restarted \n");
  #endif

  #if DEBUG_PROCESS > 0
  printf("Ctrl proc started \n");
  #endif



  while(1)
  {
    releaseSig = true;
    RecSig();
    uint8_t sigNo = recSig_p->GetSigNo();

    

    switch(sigNo)
    {
      #if CONF_USE_SDCARD == 1
      case SIGNO_SDIO_CardDetectEvent:
        sdCard.CardDetectEvent();
        releaseSig = false;
        break;
      #endif
      #if CONF_USE_LOGGING == 1
        case SIGNO_LOG:
        loggingUnit.HandleLog((logSig_c*)recSig_p);
      #endif
      default:
      break;

    }
    if(releaseSig)
    {
      delete  recSig_p;
    } 
  }
}

