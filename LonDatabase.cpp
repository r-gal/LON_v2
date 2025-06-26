#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "GeneralConfig.h"

#include "LonDataDef.hpp"
#include "LonDatabase.hpp"

LonConfigPage_st* ConfigA = (LonConfigPage_st*)SECTOR_A_ADR;
LonConfigPage_st* ConfigB = (LonConfigPage_st*)SECTOR_B_ADR;
#if LON_USE_TIMERS == 1
LonTimerPage_st*  ConfigTimer = (LonTimerPage_st*)SECTOR_TIMERS_ADR;
#endif

int32_t LonDatabase_c::SearchFile(uint32_t lAdr)
{
  for(int i =0; i< FILESINSECTOR ;i++)
  {
    LonConfigPage_st* conf_p = GetConfig(i);
    if((conf_p != NULL) && (conf_p->longAdr == lAdr))
    {
      return i;
    }
  }
  return -1;

}

LonConfigPage_st* LonDatabase_c::GetConfig(uint16_t idx)
{
  return &(ConfigA[idx]);
}

LonConfigPage_st* LonDatabase_c::ReadConfig(uint32_t lAdr)
{
  for(int i =0; i< FILESINSECTOR ;i++)
  {
    LonConfigPage_st* conf_p = GetConfig(i);
    if((conf_p != NULL) && (conf_p->longAdr == lAdr))
    {
      return conf_p;
    }
  }
  return NULL;
}

bool LonDatabase_c::CheckIfWritable(LonConfigPage_st* src, LonConfigPage_st* dest)
{
  bool writable = true;
  for(int w=0; w< FILESIZE/4;w++)
  {
    if (((src->rawData[w] ^ dest->rawData[w]) &  src->rawData[w]) > 0)
    {
      writable = false;
      break;
    }

  }
  return writable;


}

#ifdef STM32F446xx

#define FLASHWORD_SIZE 1
#define FLASF_WTITETYPE FLASH_TYPEPROGRAM_WORD
#endif

#ifdef STM32H725xx

#define FLASHWORD_SIZE FLASH_NB_32BITWORD_IN_FLASHWORD
#define FLASF_WTITETYPE FLASH_TYPEPROGRAM_FLASHWORD

#endif

bool LonDatabase_c::WriteFile(uint32_t idx, LonConfigPage_st* config, uint8_t masc)
{
  /*FLASH_ClearFlag(FLASH_FLAG_PGSERR);

  FLASH_Unlock();

  for(int w = 0; w < (FILESIZE / sizeof(uint32_t)); w++)
  {
    if(masc & 0x01) {FLASH_ProgramWord((uint32_t)(&(ConfigA[idx].rawData[w])),config->rawData[w]);}
    if(masc & 0x02) {FLASH_ProgramWord((uint32_t)(&(ConfigB[idx].rawData[w])),config->rawData[w]);}
  }

  FLASH_Lock();
*/

  HAL_FLASH_Unlock();

  HAL_StatusTypeDef status;

  int flashWordsToProgram = ( FILESIZE/4 ) / FLASHWORD_SIZE;

  for(int w = 0; w < flashWordsToProgram; w++)
  {
    uint32_t addrOffset = w * 4 * FLASHWORD_SIZE;
    uint32_t fileStart = (uint32_t)(&config->rawData[0]);
    uint32_t flashStartA = (uint32_t)(&(ConfigA[idx].rawData[0]));
    uint32_t flashStartB = (uint32_t)(&(ConfigB[idx].rawData[0]));


    if(masc & 0x01)
    {
      #ifdef STM32F4
      status = HAL_FLASH_Program(FLASF_WTITETYPE,flashStartA + addrOffset,config->rawData[w]);
      #endif
      #ifdef STM32H7
      status = HAL_FLASH_Program(FLASF_WTITETYPE,flashStartA + addrOffset,fileStart + addrOffset);
      #endif
      if(status != HAL_OK) { break; }
    }
    if(masc & 0x02) 
    {
      #ifdef STM32F4
      status = HAL_FLASH_Program(FLASF_WTITETYPE,flashStartB + addrOffset,config->rawData[w]);
      #endif
      #ifdef STM32H7
      status = HAL_FLASH_Program(FLASF_WTITETYPE,flashStartB + addrOffset,fileStart + addrOffset);
      #endif
      if(status != HAL_OK) { break; }
    }
  }

  HAL_FLASH_Lock();

  return (status == HAL_OK);

}

bool LonDatabase_c::SelectAsObsolete(uint32_t idx)
{
 /* FLASH_ClearFlag(FLASH_FLAG_PGSERR);
  FLASH_Unlock();
  FLASH_ProgramWord((uint32_t)(&(ConfigA[idx].rawData[0])),0);
  FLASH_ProgramWord((uint32_t)(&(ConfigB[idx].rawData[0])),0);
  FLASH_Lock();*/
  bool result = false;

  #ifdef STM32H7
  int refArray[FLASHWORD_SIZE];
  memset(refArray,0,sizeof(refArray));
  #endif

  HAL_FLASH_Unlock();

  HAL_StatusTypeDef status;
  #ifdef STM32F4
  status = HAL_FLASH_Program(FLASF_WTITETYPE,(uint32_t)(&(ConfigA[idx].rawData[0])),0);
  #endif
  #ifdef STM32H7
  status = HAL_FLASH_Program(FLASF_WTITETYPE,(uint32_t)(&(ConfigA[idx].rawData[0])),(uint32_t)refArray);
  #endif

  if(status == HAL_OK)
  {
    #ifdef STM32F4
    status = HAL_FLASH_Program(FLASF_WTITETYPE,(uint32_t)(&(ConfigB[idx].rawData[0])),0);
    #endif
    #ifdef STM32H7
    status = HAL_FLASH_Program(FLASF_WTITETYPE,(uint32_t)(&(ConfigB[idx].rawData[0])),(uint32_t)refArray);
    #endif
    if(status == HAL_OK)
    {
      result = true;
    }
  }
  HAL_FLASH_Lock();
  return result;
}

bool LonDatabase_c::RunDefragmentation(void)
{
  bool defragmNeeded = false;
  bool result = false;
  for(int i =0; i< FILESINSECTOR; i++)
  {
    if(ConfigA[i].longAdr == 0)
    {
      defragmNeeded = true;
      break;
    }
  }
  if(defragmNeeded)
  {
    //FLASH_ClearFlag(FLASH_FLAG_PGSERR);
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef pEraseInit;
    pEraseInit.Banks = 1;
    pEraseInit.NbSectors = 1;
    pEraseInit.Sector = SECTOR_A;
    pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    uint32_t sectorError;
    HAL_FLASHEx_Erase(&pEraseInit, &sectorError);

    if(sectorError == 0xFFFFFFFF)
    {
      uint32_t newIdx = 0;

      for(int i =0; i< FILESINSECTOR; i++)
      {
        if((ConfigB[i].longAdr != 0) && (ConfigB[i].longAdr != 0xFFFFFFFF)) 
        {
          WriteFile(newIdx,&ConfigB[i],1);
          newIdx++;
        }
      }

      pEraseInit.Sector = SECTOR_B;
      HAL_FLASHEx_Erase(&pEraseInit, &sectorError);

      if(sectorError == 0xFFFFFFFF)
      {
        for(int i =0; i< newIdx; i++)
        {
          WriteFile(i,&ConfigA[i],2);
        }
        result = true;
      }
    }
    HAL_FLASH_Lock();
  }
  else
  {
    result = true;
  }
  return result;
}

bool LonDatabase_c::SaveConfig(LonConfigPage_st* config)
{
  int32_t actIdx = SearchFile(config->longAdr);
  int32_t newIdx = -1;

  bool writable = false;
  if(actIdx >= 0)
  {
    bool writable = CheckIfWritable(config,GetConfig(actIdx));
  }  

  if(writable == false)
  {
    newIdx = SearchFile(0xFFFFFFFF);
    if(newIdx < 0)
    {
      if(RunDefragmentation() == false)
      {
        return false;
      }
      newIdx = SearchFile(0xFFFFFFFF);
      if(newIdx < 0)
      {
        return false;
        /* failed */
      }
    }
    if((actIdx >= 0) && (SelectAsObsolete(actIdx) == false))
    {
      return false;
    }
    actIdx = newIdx;

  }
  return WriteFile(actIdx,config,3);
}

bool LonDatabase_c::DeleteConfig(uint32_t lAdr)
{
  int32_t actIdx = SearchFile(lAdr);
  if(actIdx >= 0)
  {
    return SelectAsObsolete(actIdx);
  }
  return false;
}

bool LonDatabase_c::UpdateConfig(LonConfigPage_st* config)
{
  bool updateNeeded = true;

  LonConfigPage_st* configFile;
  configFile = ReadConfig(config->longAdr);
  if(configFile != NULL)
  {
    if(memcmp(config,configFile,FILESIZE) == 0)
    {
      updateNeeded = false;
    }

  }


  if(updateNeeded)
  {
    return SaveConfig(config);
  }
  else
  {
    return true;
  }

}

bool LonDatabase_c::FormatConfig(void)
{  
  bool result = false;
  HAL_FLASH_Unlock();

  FLASH_EraseInitTypeDef pEraseInit;
  pEraseInit.Banks = 1;
  pEraseInit.NbSectors = 1;
  pEraseInit.Sector = SECTOR_A;
  pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
  pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  
  uint32_t sectorError;
  HAL_FLASHEx_Erase(&pEraseInit, &sectorError);

  if(sectorError == 0xFFFFFFFF)
  {
    pEraseInit.Sector = SECTOR_B;
    HAL_FLASHEx_Erase(&pEraseInit, &sectorError);

    if(sectorError == 0xFFFFFFFF)
    {
      result = true;
    }
  }
  HAL_FLASH_Lock();

  return result;

}

#if LON_USE_TIMERS == 1
bool LonDatabase_c::FormatTimers(void)
{  
  bool result = false;
  HAL_FLASH_Unlock();

  FLASH_EraseInitTypeDef pEraseInit;
  pEraseInit.Banks = 1;
  pEraseInit.NbSectors = 1;
  pEraseInit.Sector = SECTOR_TIMERS;
  pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
  pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;


  int flashWordsToErrase = (sizeof(LonTimerPage_st)/4 ) / FLASHWORD_SIZE;
  #ifdef STM32H7
  int refArray[FLASHWORD_SIZE];
  memset(refArray,0,sizeof(refArray));
  #endif

  uint32_t sectorError;
  HAL_FLASHEx_Erase(&pEraseInit, &sectorError);


  if(sectorError == 0xFFFFFFFF)
  {
    result = true;
    for(int i =0; i< flashWordsToErrase; i++)
    {
      
      #ifdef STM32F4
      if( HAL_FLASH_Program(FLASF_WTITETYPE,(uint32_t)(&(ConfigTimer->raw[i])),0) != HAL_OK)
      {
        result = false;
        break;
      }
      #endif
      #ifdef STM32H7
      uint32_t flashAddr = (uint32_t)(&(ConfigTimer->raw[0] )) + i*FLASHWORD_SIZE*4;
      if( HAL_FLASH_Program(FLASF_WTITETYPE,flashAddr,(uint32_t)refArray) != HAL_OK)
      {
        result = false;
        break;
      }
      #endif
    }    
  }

  HAL_FLASH_Lock();
  return result;
}


LonTimerConfig_st* LonDatabase_c::ReadTimerConfig(uint8_t idx)
{
  return &(ConfigTimer->timer[idx]);
}


bool LonDatabase_c::SaveTimerConfig(uint8_t idx, LonTimerConfig_st* config)
{

  LonTimerPage_st* timerPageNew = new LonTimerPage_st;

  *timerPageNew = *ConfigTimer;

  config->timeOn1 =  config->time[0].hour*60 + config->time[0].minutes;
  config->timeOff1 = config->time[1].hour*60 + config->time[1].minutes;
  config->timeOn2 =  config->time[2].hour*60 + config->time[2].minutes;
  config->timeOff2 = config->time[3].hour*60 + config->time[3].minutes;
  config->timeOn3 =  config->time[4].hour*60 + config->time[4].minutes;
  config->timeOff3 = config->time[5].hour*60 + config->time[5].minutes;

  if(config->timeOff1 == 0) { config->timeOff1 = 24*60; }
  if((config->timeOn2 != 0) && (config->timeOff2 == 0)) { config->timeOff2 = 24*60; }
  if((config->timeOn3 != 0) && (config->timeOff3 == 0)) { config->timeOff3 = 24*60; }

  timerPageNew->timer[idx] = *config;

  bool result = false;

  HAL_FLASH_Unlock();

  FLASH_EraseInitTypeDef pEraseInit;
  pEraseInit.Banks = 1;
  pEraseInit.NbSectors = 1;
  pEraseInit.Sector = SECTOR_TIMERS;
  pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
  pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

  uint32_t sectorError;
  HAL_FLASHEx_Erase(&pEraseInit, &sectorError);

  if(sectorError == 0xFFFFFFFF)
  {
    result = true;

    int flashWordsToProgram = (sizeof(LonTimerPage_st)/4 ) / FLASHWORD_SIZE;

    for(int i =0; i< flashWordsToProgram; i++)
    {

      #ifdef STM32F4
      if( HAL_FLASH_Program(FLASF_WTITETYPE,(uint32_t)(&(ConfigTimer->raw[i] )),timerPageNew->raw[i] ) != HAL_OK)
      {
        result = false;
        break;
      }
      #endif
      #ifdef STM32H7
      uint32_t addrOffset = i*FLASHWORD_SIZE*4;
      uint32_t flashAddr = (uint32_t)(&(ConfigTimer->raw[0] )) + addrOffset;
      uint32_t fileAddr = (uint32_t)(&(timerPageNew->timer[0] )) + addrOffset;
      if( HAL_FLASH_Program(FLASF_WTITETYPE,flashAddr,fileAddr) != HAL_OK)
      {
        result = false;
        break;
      }
      #endif
    }

  }
  HAL_FLASH_Lock();

  delete timerPageNew;
  return result;
}
#endif

bool LonDatabase_c::ReplaceDevice(uint32_t oldLAdr,uint32_t newLAdr)
{
  LonConfigPage_st* updatedConf_p = new LonConfigPage_st;
  for(int i =0; i< FILESINSECTOR ;i++)
  {
    LonConfigPage_st* conf_p = GetConfig(i);
    
    if((conf_p != NULL) && (conf_p->longAdr != 0xFFFFFFFF) && (conf_p->longAdr != 0 )) /* valid config */
    {
      bool configOpened = false;


      /* check own LADR */
      if (conf_p->longAdr == oldLAdr)
      {
        if(configOpened == false)
        {
          *updatedConf_p = *conf_p;
          configOpened = true;
        }
        updatedConf_p->longAdr = newLAdr;

      }

      for(int portIdx = 0; portIdx < 8; portIdx++)
      {
        LonPortData_st* portData = &(conf_p->action[portIdx].portData);

        if(portData->out1LAdr == oldLAdr)
        {
          if(configOpened == false)
          {
            *updatedConf_p = *conf_p;
            configOpened = true;
          }
          updatedConf_p->action[portIdx].portData.out1LAdr = newLAdr;
        }

        if(portData->out2LAdr == oldLAdr)
        {
          if(configOpened == false)
          {
            *updatedConf_p = *conf_p;
            configOpened = true;
          }
          updatedConf_p->action[portIdx].portData.out2LAdr = newLAdr;
        }
      }

      if(configOpened)
      {
        SaveConfig(updatedConf_p);
        DeleteConfig(conf_p->longAdr);
      }
      
    }
  }
  delete updatedConf_p;
  return true;
}

