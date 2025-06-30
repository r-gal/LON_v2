#ifndef CTRL_PROCESS_H
#define CTRL_PROCESS_H

#include "SignalList.hpp"
#include "GeneralConfig.h"

#if CONF_USE_ETHERNET == 1
#include "Ethernet.hpp"
#endif

#if CONF_USE_SDCARD == 1
#include "sdCard_stm32h7xx.hpp"
#endif

#include "CommandSystem.hpp"

#if CONF_USE_RNG == 1
#include "RngClass.hpp"
#endif
#if CONF_USE_TIME == 1
#include "TimeClass.hpp"
#endif

#if CONF_USE_BOOTUNIT == 1
#include "BootUnit.hpp"
#endif

#include "LoggingUnit.hpp"


class CtrlProcess_c : public process_c
{
  #if CONF_USE_COMMANDS == 1
  CommandSystem_c commandSystem;
  #endif
  #if CONF_USE_ETHERNET == 1
  Ethernet_c ethernet;
  #endif
  #if CONF_USE_SDCARD == 1
  SdCard_c sdCard;
  #endif
  #if CONF_USE_RNG == 1
  RngUnit_c rng;
  #endif
  #if CONF_USE_TIME == 1
  TimeUnit_c timeUnit;
  #endif
  #if CONF_USE_BOOTUNIT == 1
  BootUnit_c bootUnit;
  #endif
  
  #if CONF_USE_LOGGING == 1
  LoggingUnit_c loggingUnit;
  #endif
  

  public :

  CtrlProcess_c(uint16_t stackSize, uint8_t priority, uint8_t queueSize, HANDLERS_et procId);

  void main(void);

  

};



#endif