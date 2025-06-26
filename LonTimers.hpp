#ifndef TIMER_CLASS_H
#define TIMER_CLASS_H

#define PWM_TABLE_SIZE 8
#define PWMTABLESIZE 4


#include "LonDatabase.hpp"

class LonTrafficProcess_c;

struct LonPwmElement_st
{
  uint8_t R;
  uint8_t G;
  uint8_t B;
};

struct LonPwmTable_st
{
  const LonPwmElement_st* pwmElements;
  uint8_t        noOfElements;
  uint8_t maxR;
  uint8_t maxG;
  uint8_t maxB;
  uint16_t maxSum;
};

class LonTimer_c : public SignalLayer_c
{


  enum TimerState_et
  {
    IDLE,
    CNT_UP,
    CNT_DN
  };

  #define MAX_PWM_VAL 255

  enum TimDevState_et
  {
    TIMER_STATE_OFF,
    TIMER_STATE_ON
  };

  enum Type_et
  {
    STD,
    PWM
  } type;

  static LonTrafficProcess_c* trafProc_p;
  static LonTimer_c timer[TIMERSINPAGE];

  uint8_t idx;
  TimerState_et state;
  uint8_t counter;
  uint8_t usedPattern;


  uint8_t GetNewValueFromRandom(uint8_t actValue, uint8_t maxVal, bool stepUp);
  void UpdateState(TimDevState_et timDevState, bool force);
  uint32_t GetRandom(void);

  void ScanCountingPWM(void);
  void StartCountingPWMup(void);
  void StartCountingPWMdown(void);
  void SetPWMstates(void);

  public: 

  LonTimer_c(void);
  void Check(LonTime_c* recSig_p);
  void SetIdx(uint8_t idx_){idx = idx_;}

  

  static void InitTimers(LonTrafficProcess_c* trafProc_p);
  static void CheckAll(LonTime_c* recSig_p);


};



#endif