#ifndef LonSensors_cLASS_H
#define LonSensors_cLASS_H

#include "SignalList.hpp"

class LonTrafficProcess_c;




class LonSensors_c : public SignalLayer_c
{
  static LonTrafficProcess_c* trafProc_p;

  static void ReadSensor(LonDevice_c* dev_p,void* userPtr);
  static void SendRequest(LonDevice_c* dev_p, uint8_t port);

  void handleDataPress(LonDevice_c* dev_p, uint8_t port, uint8_t* sensorData);
  void handleDataHig(LonDevice_c* dev_p, uint8_t port, uint8_t* sensorData);

  void CheckSensorAlarm(LonDevice_c* dev_p,uint8_t portNo);
 
  
  public:

  LonSensors_c(LonTrafficProcess_c* trafProc_p_);

  void ScanSensors(LonTime_c* recSig_p);
  void HandleSensorData(LonDevice_c* dev_p, uint8_t port, uint8_t* sensorData);



};





#endif