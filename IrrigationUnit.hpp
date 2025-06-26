#ifndef IRRIGATION_UNIT_H
#define IRRIGATION_UNIT_H

#include "SignalList.hpp"
#include "commandHandler.hpp"



class LonTrafficProcess_c;
class IrrigationUnit_c;

struct IrrigTime_st
{
  uint8_t hour;
  uint8_t minutes;  
};

struct IrigationConfig_st
{
 uint32_t longAdr;    /*B 0-3 */
 char name[12];       /*B 4-15*/
 uint32_t outputLadr[5]; /*B 16-31*/
 uint8_t outputPort[5]; /* B 32-35 */
 uint8_t enable; /* B41 */ 
 uint8_t spare1;    /* B42 */
 uint8_t spare2;  /* B43 */
 IrrigTime_st startTime[2]; /* B44 - B57 */
 uint8_t durationTime[2];  /* B48 - B51 */
 uint8_t rainCorrection[2]; /* B52 - B53 */
 uint8_t temperatureCorrection[2]; /* B54 -B55 */
 uint8_t temperatureCentre[2]; /* B56 -B57 */
 uint8_t rainRange[2];
 uint8_t tempRange[2];

 //uint32_t temperatureLadr; /* B44- B47 */
 //uint8_t temperaturePort; 

};

enum IRRIG_STATE_et
{
  IRRIG_INACTIVE,
  IRRIG_ACTIVE,
  IRRIG_FINISHED
};

class IrrigationInstance_c
{

  IRRIG_STATE_et state;
  uint8_t timeIdx;
  uint16_t admStopTime;
  uint16_t realStopTime;
  float usedRainStat;
  float usedTempStat;
 


  public:

  IRRIG_STATE_et GetState(void) { return state; }
  float GetUsedRain(void) { return usedRainStat; }
  float GetUsedTemp(void) { return usedTempStat; }
  uint16_t GetRealStopTime(void) { return realStopTime; }

  static IrrigationUnit_c* irrigUnit_p;
  IrigationConfig_st* config_p;

  IrigationConfig_st* GetConfig(void);


  void Check(LonTime_c* recSig_p);

  void SetOutputs(bool newState, bool force);

  IrrigationInstance_c(void);
  ~IrrigationInstance_c(void);



};



class IrrigationUnit_c
{

  IrrigationInstance_c* instance[IRRIG_NO_OF];
  static LonTrafficProcess_c* trafProc_p;


  public:
  LonTrafficProcess_c* GetTrafProc(void) { return trafProc_p; }

  void TimeTick(LonTime_c* recSig_p);
  void UpdateLists(void);

  void InitIrrig(LonTrafficProcess_c* trafProc_p_);

  void SetMaster(uint32_t lAdr, uint8_t port,bool state, bool force);
  void Getstat(LonIrrigGetstat_c* recSig_p);



};




class Com_irrigconfig : public Command_c
{
  public:
  char* GetComString(void) { return (char*)"irrigconfig"; }
  void PrintHelp(CommandHandler_c* commandHandler ){}
  comResp_et Handle(CommandData_st* comData_);
};

class Com_irrigprint : public Command_c
{
  public:
  char* GetComString(void) { return (char*)"irrigprint"; }
  void PrintHelp(CommandHandler_c* commandHandler ){}
  comResp_et Handle(CommandData_st* comData_);
};

class Com_irrigdelete : public Command_c
{
  public:
  char* GetComString(void) { return (char*)"irrigdelete"; }
  void PrintHelp(CommandHandler_c* commandHandler ){}
  comResp_et Handle(CommandData_st* comData_);
};

class CommandIrrigation_c :public CommandGroup_c
{

  Com_irrigconfig irrigconfig;
  Com_irrigprint irrigprint;
  Com_irrigdelete irrigdelete;

};



#endif