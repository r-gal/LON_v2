#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#if LON_USE_TIMERS == 1

#include "SignalList.hpp"

#include "LonDevice.hpp"
#include "LonTrafficProcess.hpp"
#include "LonTimers.hpp"
#include "LonDatabase.hpp"



#if CONF_USE_RNG == 1
#include "RngClass.hpp"
#endif

LonTrafficProcess_c* LonTimer_c::trafProc_p = NULL;
LonTimer_c LonTimer_c::timer[TIMERSINPAGE];

const LonPwmTable_st pwmTable[] =
{
  {NULL,255,255,200,5,400}, /* RANDOM */
  {NULL,255,255,255,200,600}, /* RANDOM */
  {NULL,0,0,0,0,0}
};


LonTimer_c::LonTimer_c(void)
{
  counter = 0;
  state = IDLE;

}

void LonTimer_c::InitTimers(LonTrafficProcess_c* trafProc_p_)
{
  trafProc_p = trafProc_p_;

  for(int i =0;i<TIMERSINPAGE;i++)
  {
    timer[i].SetIdx(i);
  }
}

uint32_t LonTimer_c::GetRandom(void)
{
  #if CONF_USE_RNG == 1
  return RngUnit_c::GetRandomVal();
  #else
  uint32_t* systickptr = (uint32_t*)0xe000e018;
  return *systickptr;
  #endif

}




uint8_t LonTimer_c::GetNewValueFromRandom(uint8_t actValue, uint8_t maxVal, bool stepUp)
{
  uint8_t maxStep;
  uint32_t randVal;
  uint8_t step;
  uint8_t newValue;

  if(actValue < 5) { maxStep = 1;}
  else if(actValue < 10) { maxStep = 2;}
  else if(actValue < 20) { maxStep = 4;}
  else if(actValue < 40) { maxStep = 6;}
  else if(actValue < 70) { maxStep = 9;}
  else if(actValue < 100) { maxStep = 14;}
  else if(actValue < 150) { maxStep = 20;}
  else if(actValue < 200) { maxStep = 26;}
  else { maxStep = 32;}

  maxStep = (maxStep*maxVal)/255;
  if(maxStep == 0)
  {
    maxStep = 1;
  }

  randVal = GetRandom();
  step = randVal % (maxStep+1);

  if(stepUp)
  {
    if((actValue + step) > maxVal)
    {
      newValue = maxVal;
    }
    else
    {
      newValue = actValue + step;
    }

  }
  else
  {
    if((actValue - step) < 0)
    {
      newValue = 0;
    }
    else
    {
      newValue = actValue - step;
    }
  }
  return newValue;


}

void LonTimer_c::CheckAll(LonTime_c* recSig_p)
{

  for(int i=0; i<TIMERSINPAGE;i++)
  {
    timer[i].Check(recSig_p);
  }

}

void LonTimer_c::Check(LonTime_c* recSig_p)
{
  LonDatabase_c database;
  LonTimerConfig_st* conf = database.ReadTimerConfig(idx);

  if(conf->ena == 1)
  {

    uint16_t actTimeInMin;
    bool wantedState=false;
    usedPattern = conf->seq1;

    actTimeInMin = recSig_p->time.Minute + 60*recSig_p->time.Hour;

    //printf("T_IDX=%d, time=%02d:%02d\n",idx,recSig_p->hour,recSig_p->minute);
    //printf("act=%d, on1=%d off1=%d on2=%d off2=%d\n",actTimeInMin,conf->timeOn1,conf->timeOff1,conf->timeOn2,conf->timeOff2);

    if(conf->timeOn1 < conf->timeOff1)
    {
      if((actTimeInMin >= conf->timeOn1) && (actTimeInMin < conf->timeOff1))
      {
        wantedState = true;
        usedPattern = conf->seq1;
      }
      else if((conf->timeOff1 < conf->timeOn2) && (conf->timeOn2 < conf->timeOff2))
      {
        if((actTimeInMin >= conf->timeOn2) && (actTimeInMin < conf->timeOff2))
        {
          wantedState = true;
          usedPattern = conf->seq2;
        }
        else if( actTimeInMin >= conf->timeOff2)
        {
          if((conf->timeOff2 < conf->timeOn3) && (conf->timeOn3 < conf->timeOff3))
          {
            if((actTimeInMin >= conf->timeOn3) && (actTimeInMin < conf->timeOff3))
            {
              wantedState = true;
            }
          }
          usedPattern = conf->seq2;
        }
      } 
      
      bool force = (recSig_p->time.Second == 0);   

      if(wantedState)
      {
        UpdateState(TIMER_STATE_ON, force);
      }
      else
      {
        UpdateState(TIMER_STATE_OFF, force);
      }
    }

    //printf("Wanted state = %d\n",wantedState);

    if(conf->type == PWM)
    {
      ScanCountingPWM();
    }
  }

}

void LonTimer_c::UpdateState(TimDevState_et timDevState,bool force)
{
  LonDatabase_c database;
  LonTimerConfig_st* conf = database.ReadTimerConfig(idx);

  if(conf->type == STD)
  {
    for(int i=0;i<4;i++)
    {
      LonDevice_c* outDev_p = trafProc_p->GetDevByLadr(conf->lAdr[i]);
      if(outDev_p != NULL)
      {
        uint8_t portState = outDev_p->GetOutputVal(conf->port[i]);

        if((portState != timDevState) || force)
        {
          outDev_p->SetOutput(conf->port[i],timDevState);
        }      
      }
    }


  }
  else if(conf->type == PWM)
  { 
    LonDevice_c* outDev_p = trafProc_p->GetDevByLadr(conf->lAdr[0]);

    if(outDev_p != NULL)
    {
      if(((outDev_p->GetState() == LonDevice_c::RUNNING) || (outDev_p->GetState() == LonDevice_c::RUNNING_DUMMY)))
      { 
        uint8_t tR,tG,tB;
    
        tR = outDev_p->GetOutputVal(3); 
        tG = outDev_p->GetOutputVal(4); 
        tB = outDev_p->GetOutputVal(5); 

        uint16_t statesSum = tR+tG+tB;
        if( timDevState == TIMER_STATE_OFF)
        {
          if((statesSum > 0) && (state != CNT_DN))
          {
            StartCountingPWMdown();
          }
        }
        else if( timDevState == TIMER_STATE_ON)
        {
          if((statesSum < pwmTable[usedPattern].maxSum) && (state != CNT_UP))
          {
            StartCountingPWMup();
          }
        }
      }
     /* startCountingPWMup();*/
    }
  }
}



void LonTimer_c::SetPWMstates(void)
{
  LonDatabase_c database;
  LonTimerConfig_st* conf = database.ReadTimerConfig(idx);

  LonDevice_c* outDev_p = trafProc_p->GetDevByLadr(conf->lAdr[0]);
  uint8_t tR,tG,tB;
  uint8_t maxR,maxG,maxB;

  bool setNewState = false;
  if(outDev_p != NULL)
  {
    if(pwmTable[usedPattern].pwmElements != NULL)
    {
      tR = pwmTable[usedPattern].pwmElements[counter].R; 
      tG = pwmTable[usedPattern].pwmElements[counter].G;
      tB = pwmTable[usedPattern].pwmElements[counter].B;

      setNewState = true;
    }
    else
    {
      if(((outDev_p->GetState() == LonDevice_c::RUNNING) || (outDev_p->GetState() == LonDevice_c::RUNNING_DUMMY)))
      {             
      
        tR = outDev_p->GetOutputVal(3); 
        tG = outDev_p->GetOutputVal(4); 
        tB = outDev_p->GetOutputVal(5); 

        maxR = pwmTable[usedPattern].maxR;
        maxG = pwmTable[usedPattern].maxG;
        maxB = pwmTable[usedPattern].maxB;

        tR = GetNewValueFromRandom(tR,maxR,(state == CNT_UP));
        tG = GetNewValueFromRandom(tG,maxG,(state == CNT_UP));
        tB = GetNewValueFromRandom(tB,maxB,(state == CNT_UP));

        setNewState = true;
      

      }
    }

    if(setNewState)
    {
      outDev_p->SetOutput(3,tR);
      outDev_p->SetOutput(4,tG);
      outDev_p->SetOutput(5,tB);

      LonDevice_c* relDev_p = trafProc_p->GetDevByLadr(conf->lAdr[1]);

      if(relDev_p != NULL)
      {
        if(tR+tR+tB > 0)
        {
          outDev_p->SetOutput(conf->port[1],1);
        }
        else
        {
          outDev_p->SetOutput(conf->port[1],0);
        }  
      }
    }
  }

}

void LonTimer_c::ScanCountingPWM(void)
{
  LonDatabase_c database;
  LonTimerConfig_st* conf = database.ReadTimerConfig(idx);

  LonDevice_c* outDev_p = trafProc_p->GetDevByLadr(conf->lAdr[0]);
  uint8_t tR,tG,tB;
  if(outDev_p != NULL)
  {
    LonDevice_c::STATE_et devState = outDev_p->GetState() ;
    if(((outDev_p->GetState() == LonDevice_c::RUNNING) || (outDev_p->GetState() == LonDevice_c::RUNNING_DUMMY)))
    {  
      tR = outDev_p->GetOutputVal(3); 
      tG = outDev_p->GetOutputVal(4); 
      tB = outDev_p->GetOutputVal(5); 
      if(pwmTable[usedPattern].pwmElements != NULL)
      {
        if(state == CNT_UP)
        {
          counter++;
          SetPWMstates();

          if(counter >= pwmTable[usedPattern].noOfElements-1)
          {
            state = IDLE;
          }
        }
        else if(state == CNT_DN)
        {
          if(counter>0)
          {
            counter--;
          }
          SetPWMstates();

          if((counter == 0)&&(tR==0)&&(tG==0)&&(tB==0))
          {
            state = IDLE;
          }
        }
      }
      else
      {
        if(state == CNT_UP)
        {
          SetPWMstates();

          if(tR+tG+tB > pwmTable[usedPattern].maxSum)
          {
            state = IDLE;
          }
        }
        else if(state == CNT_DN)
        {
          SetPWMstates();

          if(tR+tG+tB == 0)
          {
            state = IDLE;
          }
        }
      }
    }
  }
}

void LonTimer_c::StartCountingPWMup(void)
{
  if(pwmTable[usedPattern].noOfElements > 1)
  {
    counter = 1;
    state = CNT_UP;
  }
  else
  {
    counter = 0;
    state = IDLE;
  }
  SetPWMstates();
}

void LonTimer_c::StartCountingPWMdown(void)
{
  counter = pwmTable[usedPattern].noOfElements-1;
  if( counter > 0)
  {
     counter--;
  }
  if(counter > 1)
  {
    state = CNT_DN;
  }
  else
  {
    state = IDLE;
  }
  SetPWMstates();


}

#endif