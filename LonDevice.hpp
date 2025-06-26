#ifndef LONDEVICE_H
#define LONDEVICE_H

#include "TimeClass.hpp"
#include "SignalList.hpp"

#define SENSOR_ARRAY_SIZE 5

class LonTmoCounter_c;
class LonDevice_c;
#if LON_USE_VIRTUAL_CHANNELS == 1
class LonChannelVirtual_c;
#endif

class LonTrafficProcess_c;

class LonPortData_c
{
  public:
  LonPortData_c(LonDevice_c* dev_p_,uint8_t portNo_)
  {
    portNo = portNo_;
    dev_p = dev_p_;
  }
  uint8_t portNo;
  LonDevice_c* dev_p;

  //virtual void Init(void) = 0;
   
};

class  LonPortDataSw_c : public LonPortData_c
{
  public:

  LonPortDataSw_c(LonDevice_c* dev_p_,uint8_t portNo_) : LonPortData_c(dev_p_,portNo_) {}

  LonTmoCounter_c* tmo_p;

  uint8_t ignoreNextRelease;

  void Init(void) { tmo_p = NULL; }


};

class LonPortDataRel_c : public LonPortData_c
{
  public:

  LonPortDataRel_c(LonDevice_c* dev_p_,uint8_t portNo_) : LonPortData_c(dev_p_,portNo_) {}

  uint8_t value;
  uint8_t storedValue;

  void Init(void) { }


};

class LonPortDataPWM_c : public LonPortData_c
{
  public:
  LonPortDataPWM_c(LonDevice_c* dev_p_,uint8_t portNo_) : LonPortData_c(dev_p_,portNo_) {}

  uint8_t value;

  void Init(void) { }
};

class LonPortDataHigro_c : public LonPortData_c
{
  int16_t hig[SENSOR_ARRAY_SIZE];
  int16_t term[SENSOR_ARRAY_SIZE];
  uint8_t nextIdx;

  public:
  LonPortDataHigro_c(LonDevice_c* dev_p_,uint8_t portNo_) : LonPortData_c(dev_p_,portNo_) {}
 

  void Init(void) 
  {
    nextIdx = 0;
    for(int i =0; i<SENSOR_ARRAY_SIZE;i++) {hig[i] = -1000; term[i] = -1000; }
  }

  void SetMeasurement(const uint8_t* buffer);
  float GetSensorValue(uint8_t subPort);
  uint8_t GetType(void);
  

};

class LonPortDataPress_c : public LonPortData_c
{
  int32_t press[SENSOR_ARRAY_SIZE];
  uint8_t nextIdx;
  public:
  LonPortDataPress_c(LonDevice_c* dev_p_,uint8_t portNo_) : LonPortData_c(dev_p_,portNo_) {}


  void Init(void)
  {
    nextIdx = 0;
    for(int i =0; i<SENSOR_ARRAY_SIZE;i++) {press[i] = -1000;}
  }

  void SetMeasurement(const uint8_t* buffer);
  float GetSensorValue(uint8_t subPort);
  float GetReducedPressure(float absPressure, float altitude, float temperature);

};

#if LON_USE_VIRTUAL_CHANNELS == 1
class LonPortDataVirtualChannel_c : public LonPortData_c
{
  public:
  LonPortDataVirtualChannel_c(LonDevice_c* dev_p_,uint8_t portNo_) : LonPortData_c(dev_p_,portNo_) {}

  LonChannelVirtual_c* channel_p;

  void Init(void) { channel_p = NULL; }
};
#endif

class LonDevice_c  : public SignalLayer_c
{

  const uint32_t lAdr;
  const uint8_t chNo;
  const uint8_t sAdr;

  uint8_t* portDataMemSpace;

  LonPortData_c* portData_p[8];

  uint32_t get_u32(uint8_t* buf, uint8_t pos);
  uint16_t get_u16(uint8_t* buf, uint8_t pos);
  uint8_t get_u8(uint8_t* buf, uint8_t pos);

  public: enum STATE_et
  {
    NEW,
    CONFIGURING,
    CONFIGURED,
    RUNNING,
    CONFIGURING_DUMMY,
    CONFIGURED_DUMMY,   
    RUNNING_DUMMY,
    LOST
  };

  private: STATE_et state;

  public:

  static LonTrafficProcess_c* trafProc_p;

  uint8_t timeToNextScan;

  uint8_t lastSeqNo;

  TimeLight_st lastCheckTime;
  void UpdateCheckTime(void);
  uint8_t noOfLost;

  LonPortData_c* GetPortData(uint8_t port) { return portData_p[port]; }

  LonDevice_c(uint32_t lAdr_,uint8_t chNo_,uint8_t sAdr_);
  ~LonDevice_c();
  uint32_t GetLAdr(void) { return lAdr; }
  uint8_t GetSadr(void) {return  sAdr; }
  uint8_t GetChNo(void) {return chNo; }

  void GetConfig(uint8_t* confBuffer);

  devType_et  GetDevType(void);
  STATE_et GetState(void) { return state; }

  void SetOutput(uint8_t port, uint8_t value);
  void SetOutputConfirm(uint8_t port, uint8_t value);
  void SetOutputFinish(void);

  uint8_t phyPortToPort(uint8_t phyPort);
  uint8_t mascToPhyMasc(uint8_t masc);
  void HandleSwEvent(LonTrafficProcess_c* trafproc_p, uint8_t port,uint8_t event);

  uint8_t GetOutputVal(uint8_t port);
  uint8_t GetStoredRelVal(uint8_t port);

  void ClearTmo(uint8_t port);

  void ConfirmConfig(void);
  void CheckOutputStates(void);

  bool IsEnabled(uint8_t port);
  
  void GetValues(uint8_t port, float* val1, float* val2);

  static devType_et GetDevType(uint32_t lAdr);
  static uint8_t GetPortType(uint32_t lAdr,uint8_t port);
  static const char* GetDevTypeString(uint32_t lAdr);

  #if LON_USE_COMMAND_LINK == 1
  void GetDevInfo(LonGetDevInfo_c* recSig_p);
  #endif

  void SetLost() { state = LOST; noOfLost++; }
  bool IsLost() { return (state == LOST); }

  
  void CheckTMOConsistency(LonTrafficProcess_c* trafproc_p);

};

#endif