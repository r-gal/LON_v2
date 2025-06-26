#ifndef SENSORS_DATABASE_PROCESS_H
#define SENSORS_DATABASE_PROCESS_H

#define SENSOR_DATA_ARRRAY_SIZE 32

#include "commandHandler.hpp"



struct SensorData_st
{
  uint32_t lAdr;
  uint16_t lastIdx;
  uint8_t port;
  uint8_t spare;


};

enum STATS_RANGE_et
{
  STATS_HOUR,
  STATS_DAY,
  STATS_MONTH,


};

enum STATS_MODE_et
{
  SENSORS_STATS_AVG,
  SENSORS_STATS_SUM
};

class LonSensorsDatabaseProcess_c : public process_c
{

  void HandleSensorData(LonSensorData_c* sig_p);
  bool CheckIfStore(uint32_t lADr,uint8_t port, uint16_t timeIdx);
  void HandleSensorHistory(LonGetSensorsHistory_c* recSig_p);

  SensorData_st sensorDataArray[SENSOR_DATA_ARRRAY_SIZE];

  void GetFileBufferSize(int type, uint16_t* bufferSize_p, uint16_t* fileSize_p, bool* use2ndBuffer_p);
  void GetFileName(int type, int day, uint32_t lAdr,uint8_t port, char* nameBuf);
  bool GetSensorsDayStatistics(int day, int type, uint32_t lAdr, int port, int subport, STATS_DATA_st* buffer, STATS_MODE_et mode,bool detailed);
  void GetSensorsStatistics(LonGetSensorsStats_c* sig_p);

  public :


  LonSensorsDatabaseProcess_c(uint16_t stackSize, uint8_t priority, uint8_t queueSize, HANDLERS_et procId);

  void main(void);



};

class Com_getsensorhist : public Command_c
{
  public:
  char* GetComString(void) { return (char*)"getsensorhist"; }
  void PrintHelp(CommandHandler_c* commandHandler ){}
  comResp_et Handle(CommandData_st* comData_);
};


class CommandLonSensorsDatabase_c :public CommandGroup_c
{

  Com_getsensorhist getsensorhist;

};


#endif

