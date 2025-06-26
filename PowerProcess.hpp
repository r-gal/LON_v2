#ifndef PWR_PROCESS_H
#define PWR_PROCESS_H

#include "SignalList.hpp"
#include "GeneralConfig.h"

#include "commandHandler.hpp"

#define ACU_LOAD_INTERVAL 10

#define SCALE_CHANNELS_CURRENT (100)
#define SCALE_MAIN_CURRENT (200)
#define SCALE_EXT_CURRENT (200)
#define SCALE_CORE_CURRENT (100)
#define SCALE_MAIN_VOLTAGE (10)
#define SCALE_ACU_VOLTAGE (10)




class PwrTimeEvent_c : public TimeEvent_c
{
  public:

  void Action(SystemTime_st* time);
};

class PwrProcess_c : public process_c
{

  PwrTimeEvent_c timeEvent;
  bool vBatMeasure;

  void Tick(void);

  void InitAdc(void);

  void InitAcu(void);


  void AcuEnable(void);
  void AcuDisable(void);
  void AcuLoadEnable(void);
  void AcuLoadDisable(void);

  void CheckPowerState(float acuV, float mainV);

  #if LON_USE_SENSORS_DATABASE == 1
  void FillPowerInfo(LonGetPowerInfo_c* sig_p);
  #endif
  void FillCmdPowerInfo(pwrGetInfo_c* sig_p);

  //uint32_t adc1Data[8];
  //uint32_t adc3Data[6];

  uint32_t lastMinuteHistory[60][14];
  uint32_t lastMinuteSum[14];
  uint32_t last5MinutesHistory[5][14];
  uint32_t last5MinutesSum[14];
  bool historyInitiated;

  uint32_t lastData[17];
  uint8_t lastIdx;
  uint8_t lastIdxMinutes;

  void AdcReady(void);
 

  public :

  static float GetValue(uint32_t rawData, float scale);

  PwrProcess_c(uint16_t stackSize, uint8_t priority, uint8_t queueSize, HANDLERS_et procId);

  void main(void);

  

};

class Com_powerinfo : public Command_c
{
  public:
  char* GetComString(void) { return (char*)"powerinfo"; }
  void PrintHelp(CommandHandler_c* commandHandler ){}
  comResp_et Handle(CommandData_st* comData_);
};

class CommandPower_c :public CommandGroup_c
{

  Com_powerinfo powerinfo;
};

#endif