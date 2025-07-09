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

#include "LonMain.hpp"

  extern RestartData_st restartData;


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



  uint32_t RSRreg = RCC->RSR;
  RCC->RSR = 0x10000; /* reset flags */

  loggingUnit.Init();

  char* str = new char[128];

  sprintf(str,"System restarted RCR = 0x%08X \n",RSRreg);
  LoggingUnit_c::Log(LOG_FATAL_ERROR,str);

  sprintf(str,"PC=0x%08X SP=0x%08X, LR=0x%08X\n", restartData.PCreg, restartData.SPreg, restartData.LRreg);
  LoggingUnit_c::Log(LOG_FATAL_ERROR,str);

  sprintf(str,"R0=0x%08X R1=0x%08X, R2=0x%08X R3=0x%08X\n", restartData.rReg[0], restartData.rReg[1], restartData.rReg[2], restartData.rReg[3]);
  LoggingUnit_c::Log(LOG_FATAL_ERROR,str);

  sprintf(str,"R4=0x%08X R5=0x%08X, R6=0x%08X R7=0x%08X\n", restartData.rReg[4], restartData.rReg[5], restartData.rReg[6], restartData.rReg[7]);
  LoggingUnit_c::Log(LOG_FATAL_ERROR,str);

  sprintf(str,"CFSR=0x%08X HFSR=0x%08X, MMFAR=0x%08X BFAR=0x%08X\n", restartData.CFSRreg, restartData.HFSRreg, restartData.MMFARreg, restartData.BFAR);
  LoggingUnit_c::Log(LOG_FATAL_ERROR,str);

  
  memset(&restartData,0,sizeof(RestartData_st));
  


  delete[] str;

  #endif

  #if DEBUG_PROCESS > 0
  printf("Ctrl proc started \n");
  #endif

  /*
  // Hard Fault trigger 
  uint32_t* p = (uint32_t*)0x44444444;
  *p  = 5;
  */



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

