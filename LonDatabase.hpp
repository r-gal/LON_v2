#ifndef LONDATABASE_H
#define LONDATABASE_H



#define SECTOR_A_ADR CONFIG_FLASH_A
#define SECTOR_B_ADR CONFIG_FLASH_B
#define SECTOR_A_SIZE CONFIG_FLASH_A_SIZE
#define SECTOR_B_SIZE CONFIG_FLASH_B_SIZE

#define SECTOR_A CONFIG_FLASH_A_SECTOR
#define SECTOR_B CONFIG_FLASH_B_SECTOR

#define FILESIZE 512
#define FILESINSECTOR (SECTOR_A_SIZE / FILESIZE)

#if LON_USE_TIMERS == 1

#define SECTOR_TIMERS_ADR CONFIG_FLASH_TIM
#define SECTOR_TIMERS_SIZE CONFIG_FLASH_TIM_SIZE
#define SECTOR_TIMERS    CONFIG_FLASH_TIM_SECTOR

#define TIMERSIZE 128
#define TIMERSINPAGE 12 //32

#endif






struct LonSensorAlm_st
{
  uint16_t level;
  uint8_t hist;
  uint8_t conf;
};


struct LonPortData_st
{
  uint32_t type;    /*B 0-3*/
  char name[12];    /*B 4-15*/
  uint32_t out1LAdr;  /*B 16-19*/
  uint32_t out1Port;  /*B 20-23*/
  uint32_t out2LAdr;  /*B 24-27*/
  uint32_t out2Port;  /*B 28-31*/
  union{              /*B 32-35*/
  uint32_t cntMax;    
  LonSensorAlm_st hSensor; 
  uint32_t altitude; /* meters over sea level - only for pressure sensor */
  };
  LonSensorAlm_st tSensor; /*B 36-39 */ 
  uint32_t spare[2]; /*B 40-43 */

};

union LonActionUnion
{
  uint32_t rawAction[12];
  LonPortData_st portData;
};

struct LonConfigPage_st /* total 512 bytes */
{
  union
  {
  struct{
  uint32_t longAdr;             /*0*/
  uint32_t enabled;             /*1*/
  uint32_t bistable;            /*2*/
  uint32_t inverted ;           /*3*/
  uint32_t pressCounterMax;     /*4*/
  uint32_t waitCounterMax;      /*5*/
  uint32_t addedToList;         /*6*/
  uint32_t relMode;             /*7*/
  uint32_t spare[24];           /*6-31*/

  LonActionUnion action[8];
  };
  uint32_t rawData[128];
  };


};

#if LON_USE_TIMERS == 1
struct LonTimerTime_st
{
  uint8_t hour;
  uint8_t minutes;  
};

struct LonTimerConfig_st
{ 
  union
  {
    struct
    {
      uint32_t lAdr[4];       /*B 0-15*/
      uint8_t port[4];        /*B 16-19*/
      char name[12];        /*B 20-31*/      
      uint8_t ena;          /*B 32*/
      uint8_t seq1;        /*B 33*/
      uint8_t seq2;        /*B 34*/
      uint8_t type;        /*B 35*/
      LonTimerTime_st time[6]; /*B 36-47*/
      uint16_t timeOn1, timeOff1, timeOn2,timeOff2 , timeOn3,timeOff3;         /*B 48-59*/
      
    };
    uint32_t raw[TIMERSIZE/4];
  };

};

struct LonTimerPage_st
{
  union{
  LonTimerConfig_st timer[TIMERSINPAGE];
  uint32_t raw[(TIMERSIZE * TIMERSINPAGE) / 4];
  };
};
#endif





class LonDatabase_c
{

  

  public:

  LonConfigPage_st* ReadConfig(uint32_t lAdr);
  bool SaveConfig(LonConfigPage_st* config);
  bool DeleteConfig(uint32_t lAdr);
  bool FormatConfig(void);  
  bool UpdateConfig(LonConfigPage_st* config);
  bool ReplaceDevice(uint32_t oldLAdr,uint32_t newLAdr);
  bool RunDefragmentation(void);
#if LON_USE_TIMERS ==1
  LonTimerConfig_st* ReadTimerConfig(uint8_t idx);
  bool SaveTimerConfig(uint8_t idx, LonTimerConfig_st* config);
  bool FormatTimers(void);
  static uint16_t GetNoOfTimers(void) { return TIMERSINPAGE; }
#endif
  static uint16_t GetNoOfDevices(void) { return FILESINSECTOR; }
  static uint16_t GetConfigSize(void) { return FILESIZE; }

  LonConfigPage_st* GetConfig(uint16_t idx);

  bool WriteFile(uint32_t idx, LonConfigPage_st* config, uint8_t masc);

  private:

  
  int32_t SearchFile(uint32_t lAdr);

  bool CheckIfWritable(LonConfigPage_st* src, LonConfigPage_st* dest);
  
  bool SelectAsObsolete(uint32_t idx);
  


};

#endif