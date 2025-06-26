 #include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#if LON_USE_PWR_MGMT == 1


#include "PowerProcess.hpp"

pwrTimTick_c pwrTimSig;
pwrAdcReady_c adcReadySig;
uint32_t __attribute((section(".RAM1"))) adc1Data[8];
uint32_t __attribute((section(".RAM2"))) adc3Data[9];
volatile bool adc1Ready;
volatile bool adc3Ready;

volatile float batteryVoltage = 0;
volatile float cpuTemperature = 0;
volatile float acuVoltage = 0;
volatile float mainVoltage = 0;

extern IWDG_HandleTypeDef hiwdg1;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc3;
extern DTS_HandleTypeDef hdts;

CommandPower_c commandPower;

void VrefInit(void);

void PwrTimeEvent_c::Action(SystemTime_st* time)
{
  memcpy(&(pwrTimSig.time),time,sizeof(SystemTime_st));
  //printf("ISR time, sec =%d\n",time->Second);
  pwrTimSig.SendISR();
}


void vPwrTimerCallback( TimerHandle_t xTimer )
{
  //pwrTimSig.Send();
}

const char* postFix[] = {"mA","mA","mA","mA","mA","mA","mA","mA","mA","mA","mA","mA","V ","V "};
const uint8_t scale[] = {
SCALE_CHANNELS_CURRENT,
SCALE_CHANNELS_CURRENT,
SCALE_CHANNELS_CURRENT,
SCALE_CHANNELS_CURRENT,
SCALE_CHANNELS_CURRENT,
SCALE_CHANNELS_CURRENT,
SCALE_CHANNELS_CURRENT,
SCALE_CHANNELS_CURRENT,
SCALE_CORE_CURRENT,
SCALE_EXT_CURRENT,
SCALE_EXT_CURRENT,
SCALE_MAIN_CURRENT,
SCALE_MAIN_VOLTAGE,
SCALE_ACU_VOLTAGE};


PwrProcess_c::PwrProcess_c(uint16_t stackSize, uint8_t priority, uint8_t queueSize, HANDLERS_et procId) : process_c(stackSize,priority,queueSize,procId,"POWER")
{

  //TimerHandle_t timer = xTimerCreate("",pdMS_TO_TICKS(5000),pdTRUE,( void * ) 0,vPwrTimerCallback);
  //xTimerStart(timer,0);

  historyInitiated = false;
  lastIdx = 0;
  lastIdxMinutes = 0;
  vBatMeasure = false;

}

void PwrProcess_c::main(void)
{

  #if DEBUG_PROCESS > 0
  printf("Power proc started \n");
  #endif

  InitAdc();
  InitAcu();

  for(int j=0;j<14;j++)
  {
    for(int i=0;i<60;i++)
    {
      lastMinuteHistory[i][j] = 0;
    }
    for(int i=0;i<5;i++)
    {
      last5MinutesHistory[i][j] = 0;
    }
    lastMinuteSum[j] = 0;
    last5MinutesSum[j] = 0;
  }


  while(1)
  {
    releaseSig = true;
    RecSig();
    uint8_t sigNo = recSig_p->GetSigNo();
    

    switch(sigNo)
    {
      case SIGNO_PWR_TIM_TICK:
      releaseSig = false;
      Tick();
      break;

      case SIGNO_PWR_ADC_READY:
      releaseSig = false;
      AdcReady();      
      break;
#if LON_USE_SENSORS_DATABASE == 1
      case SIGNO_LON_GET_POWER_INFO:
      releaseSig = false;
      FillPowerInfo((LonGetPowerInfo_c*) recSig_p);      
      break;
#endif
      case SIGNO_CMD_POWERINFO:
      releaseSig = false;
      FillCmdPowerInfo((pwrGetInfo_c*) recSig_p);      
      break;

      default:
      break;

    }
    if(releaseSig)
    {
      delete  recSig_p;
    } 
  }
}



void PwrProcess_c::Tick(void)
{
  //printf("ADC tick\n");

  #if CONF_USE_WATCHDOG == 1
   __HAL_IWDG_RELOAD_COUNTER(&hiwdg1);
  #endif

  adc1Ready = false;
  adc3Ready = false;

  HAL_ADC_Start_DMA(&hadc1, adc1Data,8);

  if(vBatMeasure == true)
  {
    ADC3_COMMON->CCR |= ADC_CCR_VBATEN;
  }
  HAL_ADC_Start_DMA(&hadc3, adc3Data,9);
}

void PwrProcess_c::InitAdc(void)
{
  VrefInit();
  //MX_DTS_Init();

  //MX_DMA_Init();
  //MX_BDMA_Init();

  //MX_ADC1_Init();
  //MX_ADC3_Init();

  

}

void PwrProcess_c::InitAcu(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
 __HAL_RCC_GPIOE_CLK_ENABLE();

  GPIO_InitStruct.Pin = PWR_ACU_LOAD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(PWR_ACU_LOAD_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pin =  PWR_ACU_ENA_Pin;
  HAL_GPIO_Init(PWR_ACU_ENA_GPIO_Port, &GPIO_InitStruct);

  AcuEnable();
  AcuLoadDisable();
}

void PwrProcess_c::AcuEnable(void)
{
  HAL_GPIO_WritePin(PWR_ACU_ENA_GPIO_Port,PWR_ACU_ENA_Pin,GPIO_PIN_SET);
}
void PwrProcess_c::AcuDisable(void)
{
  HAL_GPIO_WritePin(PWR_ACU_ENA_GPIO_Port,PWR_ACU_ENA_Pin,GPIO_PIN_RESET);
}
void PwrProcess_c::AcuLoadEnable(void)
{
  HAL_GPIO_WritePin(PWR_ACU_LOAD_GPIO_Port,PWR_ACU_LOAD_Pin,GPIO_PIN_SET);
}
void PwrProcess_c::AcuLoadDisable(void)
{
  HAL_GPIO_WritePin(PWR_ACU_LOAD_GPIO_Port,PWR_ACU_LOAD_Pin,GPIO_PIN_RESET);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if(hadc->Instance == ADC1)
  {
    //printf("ADC1 ready\n");
    adc1Ready = true;
    
  }
  else if(hadc->Instance == ADC3)
  {
    //printf("ADC3 ready\n");
    ADC3_COMMON->CCR &= ~ADC_CCR_VBATEN;
    adc3Ready = true;
  }

  if((adc1Ready == true) && (adc3Ready== true))
  {
    adcReadySig.SendISR();
  }
}

float PwrProcess_c::GetValue(uint32_t rawData, float scale)
{
  float voltage = rawData;
  voltage *=  (2.5 / 0x10000);

  float outData = voltage * scale;
  return outData;
}





void PwrProcess_c::AdcReady(void)
{
  uint8_t idx = pwrTimSig.time.Second;
  //printf("idx = %d\n",idx);

  uint8_t phase = idx % ACU_LOAD_INTERVAL;

  if(phase == 0)
  {
    AcuLoadEnable();
  }
  else if(phase == ACU_LOAD_INTERVAL -1)
  {
    AcuLoadDisable();
  }

  for(int i=0;i<14 ;i++)
  {
    if(i<8)
    {
      lastData[i] = adc1Data[i];
    }
    else
    {
      if((i != 13) || (phase == 0) || (historyInitiated == false))
      {
        lastData[i]  = (adc3Data[i-8])*16;
      }      
    }
  }

  
  if(historyInitiated == false)
  {
    for(int j=0;j<14;j++)
    {
      uint32_t initData = lastData[j];

      for(int i=0;i<60;i++)
      {
        lastMinuteHistory[i][j] = initData;
      }
      for(int i=0;i<5;i++)
      {
        last5MinutesHistory[i][j] = initData;
      }
      lastMinuteSum[j] = initData * 60;
      last5MinutesSum[j] = initData * 5;
    }

    historyInitiated = true;
  }

  uint32_t temperature = __HAL_ADC_CALC_TEMPERATURE(2500,adc3Data[7],ADC_RESOLUTION_16B);
  cpuTemperature = (float)temperature;

  if(vBatMeasure == true)
  {
    batteryVoltage = GetValue(adc3Data[6]*16,4);
    vBatMeasure = false;
  }

  if(phase == 0)
  {
    vBatMeasure = true;
  }

  mainVoltage = GetValue(lastData[12],SCALE_MAIN_VOLTAGE);
  acuVoltage = GetValue(lastData[13],SCALE_ACU_VOLTAGE);

  CheckPowerState(acuVoltage, mainVoltage);

  for(int i=0;i<14;i++) 
  { 
    lastMinuteSum[i] -= lastMinuteHistory[idx][i];
    lastMinuteHistory[idx][i] = lastData[i];
    lastMinuteSum[i] += lastData[i];
  }

  if(idx == 0)
  {
    uint8_t idx2 = pwrTimSig.time.Minute % 5;

    for(int i=0;i<14;i++) 
    { 
      last5MinutesSum[i] -= last5MinutesHistory[idx2][i];
      last5MinutesHistory[idx2][i] = lastMinuteSum[i] / 60;
      last5MinutesSum[i] += last5MinutesHistory[idx2][i];
    }
    lastIdxMinutes = idx2;
  }
  lastIdx = idx;

  #if LON_USE_SENSORS_DATABASE == 1
  LonSensorData_c* sig_p = new LonSensorData_c;
  sig_p->lAdr = 0;
  sig_p->port = 0;

  for(int i=0;i<8;i++) 
  { 
    sig_p->data[i] = GetValue(last5MinutesSum[i]/5,SCALE_CHANNELS_CURRENT);
  }
  sig_p->data[8] = GetValue(last5MinutesSum[8]/5,SCALE_CORE_CURRENT);
  sig_p->data[9] = GetValue(last5MinutesSum[9]/5,SCALE_EXT_CURRENT);
  sig_p->data[10] = GetValue(last5MinutesSum[10]/5,SCALE_EXT_CURRENT);
  sig_p->data[11] = GetValue(last5MinutesSum[11]/5,SCALE_MAIN_CURRENT);
  sig_p->data[12] = GetValue(last5MinutesSum[12]/5,SCALE_MAIN_VOLTAGE);
  sig_p->data[13] = GetValue(last5MinutesSum[13]/5,SCALE_ACU_VOLTAGE);

  sig_p->type = 2;
  sig_p->Send();
  #endif


/*

  for(int i=0;i<8;i++) 
  { 
    float current = GetValue(adc1Data[i],SCALE_CHANNELS_CURRENT);
    printf("%.2fmA, ",current);
  }
  printf("\n");

  for(int i=0;i<8;i++) 
  { 
    float current = GetValue(lastMinuteSum[i]/60,SCALE_CHANNELS_CURRENT);
    printf("lastMinSum=%.2fmA, ",current);
  }

  printf("\n");

  for(int i=0;i<8;i++) 
  { 
    float current = GetValue(last5MinutesSum[i]/5,SCALE_CHANNELS_CURRENT);
    printf("last5MinSum=%.2fmA, ",current);
  }

  printf("\n");

  printf("C_core = %.2fmA\n",GetValue(adc3Data[0]*16,SCALE_CORE_CURRENT));
  printf("C_ex1 = %.2fmA\n",GetValue(adc3Data[1]*16,SCALE_EXT_CURRENT));
  printf("C_ex2 = %.2fmA\n",GetValue(adc3Data[2]*16,SCALE_EXT_CURRENT));
  printf("C_main = %.2fmA\n",GetValue(adc3Data[3]*16,SCALE_MAIN_CURRENT));
  printf("V_main = %.2fV\n",GetValue(adc3Data[4]*16,SCALE_MAIN_VOLTAGE));
  printf("V_acu = %.2fV\n",GetValue(adc3Data[5]*16,SCALE_ACU_VOLTAGE));

  printf("V_bat = %.2fV\n",GetValue(adc3Data[6]*16,4));
  printf("V_temp = %.2fV\n",GetValue(adc3Data[7]*16,1));
  printf("V_ref = %.2fV\n",GetValue(adc3Data[8]*16,1));

  uint32_t rawValue = adc3Data[7];

  temperature = __HAL_ADC_CALC_TEMPERATURE(2500,adc3Data[7],ADC_RESOLUTION_16B);

  batteryVoltage = GetValue(adc3Data[6]*16,4);
  cpuTemperature = (float)temperature;

   uint32_t temperature2 = ((TEMPSENSOR_CAL2_TEMP - TEMPSENSOR_CAL1_TEMP  )  * (((rawValue*25)/33) - *TEMPSENSOR_CAL1_ADDR) )/ (*TEMPSENSOR_CAL2_ADDR - *TEMPSENSOR_CAL1_ADDR) + 30;

  printf("Temperature = %d raw = %d\n",temperature,rawValue);
  printf("Temperature = %d raw = %d\n",temperature2,rawValue);
*/
}

#if LON_USE_SENSORS_DATABASE == 1
void PwrProcess_c::FillPowerInfo(LonGetPowerInfo_c* sig_p)
{

  for(int i=0;i<8;i++) 
  { 
    sig_p->data[i] = GetValue(lastData[i],SCALE_CHANNELS_CURRENT);
  }
  sig_p->data[8] = GetValue(lastData[8],SCALE_CORE_CURRENT);
  sig_p->data[9] = GetValue(lastData[9],SCALE_EXT_CURRENT);
  sig_p->data[10] = GetValue(lastData[10],SCALE_EXT_CURRENT);
  sig_p->data[11] = GetValue(lastData[11],SCALE_MAIN_CURRENT);
  sig_p->data[12] = GetValue(lastData[12],SCALE_MAIN_VOLTAGE);
  sig_p->data[13] = GetValue(lastData[13],SCALE_ACU_VOLTAGE);

  xTaskNotifyGive(sig_p->task);

}
#endif

void PwrProcess_c::FillCmdPowerInfo(pwrGetInfo_c* sig_p)
{
  sig_p->data_p = lastMinuteHistory[0];
  sig_p->data2_p = last5MinutesHistory[0];
  sig_p->lastIdx = lastIdx;
  sig_p->lastIdxMinutes = lastIdxMinutes;
  xTaskNotifyGive(sig_p->task);
}

void PwrProcess_c::CheckPowerState(float acuV, float mainV)
{

  if((mainV < 15.0) && (acuV <= 10.5))
  {
    AcuDisable(); /* power off in result */
  }


}

void PrintHeader(char* strBuf)
{
  strcpy(strBuf,"   ");
  for(int i=0;i<8;i++)
  {
    sprintf(strBuf+3+(10*i),"    PORT_%d",i);
  }
  strcpy(strBuf+3+80,"    C_CORE    C_EXT1    C_EXT2    C_MAIN    V_MAIN     V_ACU\n");

}

void PrintLine(char* strBuf,uint8_t idx, uint32_t* data)
{
    sprintf(strBuf,"%2d ",idx);
    for(int i=0;i<14;i++)
    {
      sprintf(strBuf+3+(10*i)," %7.2f%s",PwrProcess_c::GetValue(data[ i],scale[i]),postFix[i]);
    }
    strcat(strBuf,"\n");

}

comResp_et Com_powerinfo::Handle(CommandData_st* comData)
{
  char* strBuf  = new char[256];

  pwrGetInfo_c* sig_p = new pwrGetInfo_c;
  
  sig_p->task = xTaskGetCurrentTaskHandle();
  sig_p->Send();

  ulTaskNotifyTake(pdTRUE ,portMAX_DELAY );

  PrintHeader(strBuf);
  Print(comData->commandHandler,strBuf);

  uint8_t idx = sig_p->lastIdx;

  for(int j=0;j<60;j++)
  {
    idx++;
    if(idx >= 60)
    {
      idx = 0;
    }
    PrintLine(strBuf,j,sig_p->data_p+ (14*idx));
    Print(comData->commandHandler,strBuf);
  }

  PrintHeader(strBuf);
  Print(comData->commandHandler,strBuf);

  idx = sig_p->lastIdxMinutes;

  for(int j=0;j<5;j++)
  {
    idx++;
    if(idx >= 5)
    {
      idx = 0;
    }
    PrintLine(strBuf,j,sig_p->data2_p+ (14*idx));
    Print(comData->commandHandler,strBuf);
  } 

  delete[] strBuf;
  return COMRESP_OK;
}

void VrefInit(void)
{
  __HAL_RCC_VREF_CLK_ENABLE();

  /** Configure the internal voltage reference buffer voltage scale
  */
  HAL_SYSCFG_VREFBUF_VoltageScalingConfig(SYSCFG_VREFBUF_VOLTAGE_SCALE0);

  /** Enable the Internal Voltage Reference buffer
  */
  HAL_SYSCFG_EnableVREFBUF();

  /** Configure the internal voltage reference buffer high impedance mode
  */
  HAL_SYSCFG_VREFBUF_HighImpedanceConfig(SYSCFG_VREFBUF_HIGH_IMPEDANCE_DISABLE);

}

#endif