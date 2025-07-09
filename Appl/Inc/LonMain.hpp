#ifndef MAIN_H
#define MAIN_H

#include "GeneralConfig.h"



struct RestartData_st
{
  uint32_t PCreg;
  uint32_t SPreg;
  uint32_t LRreg;

  uint32_t rReg[8];

  uint32_t CFSRreg;
  uint32_t HFSRreg;
  uint32_t MMFARreg;
  uint32_t BFAR;




};

#ifdef __cplusplus
 extern "C" {
#endif
int applMain(void);
void HardFault_Handler(void);
#ifdef __cplusplus
}
#endif

#endif