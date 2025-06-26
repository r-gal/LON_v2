#ifndef CHANNEL_COMMON_H
#define CHANNEL_COMMON_H

#include "LonDevice.hpp"

#define TIMEOUT_CNT_START 40

enum state_et
{
  NEW,
  INITIATING,
  RUNNING,
  ADR_CONQ_SENT_INITIAL,
  ADR_CONQ_SENT,
  CONFIG_SENT_INITIAL,
  CONFIG_SENT,

};

#define ADR_CONQ_INTERVAL 10

class LonChannel_c  : public SignalLayer_c
{


  state_et state;

    bool timeoutActive;
  uint32_t timeoutCnt;

  protected :

  const uint8_t chNo;

  uint8_t adrConqTimer;

  LonDevice_c* devList[NO_OF_DEV_IN_CHANNEL];

  LonTrafficProcess_c* trafProc_p;

  void StartTimeout(void);
  void StopTimeout(void);

  public:

  void CheckTimeout(void);
  uint8_t GetTimeoutCnt(void) 
  {
    if(timeoutActive)
    {
      return  timeoutCnt;
    }
    else
    {
      return 0xFF;
    }

  }


  LonChannel_c(uint8_t chNo_,LonTrafficProcess_c* trafProc_p_);

  LonDevice_c* SearchDevice(uint32_t lAdr);
  LonDevice_c* GetDevice(uint8_t sAdr);
  void RunForAll(Function_type function, void* userPtr);

  #if LON_USE_COMMAND_LINK == 1
  void GetDevList(LonGetDevList_c* recSig_p);
  #endif

  LonDevice_c* CreateNewDevice(uint32_t lAdr,LonTrafficProcess_c* trafProc_p);
  void DeleteDevice(LonDevice_c* dev_p);

  state_et GetState(void) { return state; }

  virtual void HandleReceivedFrame(LonIOdata_c* recSig_p) = 0;

  virtual void SendDetach(void) = 0;

  void SearchNewDevices(bool force);
  virtual void SendAdrConquest(void) = 0;
  void HandleDeviceFound(uint32_t lAdr);

  void ConfigDevice(LonDevice_c* dev_p);

  void ConfirmDeviceConfig(uint32_t lAdr,bool result);

  virtual void SendCheckDevice(uint32_t lAdr) = 0;

  virtual void SendDevConfig(LonDevice_c* dev_p) = 0;

  virtual void SendFrame(LonDevice_c* dev_p, uint8_t bytesNo, uint8_t* buffer) = 0;

  protected: 

  void ReceiveDetachAck(void);
  void CheckDevFromConfigAck(uint32_t lAdr,bool result);
  void HandleDataTrAck(LonDevice_c* dev_p, uint8_t port, uint8_t value);
  void HandleIoReadAck(LonDevice_c* dev_p, uint8_t frameCode, uint8_t* ioData);

  private:


 
  public:

  uint32_t noOfScannedDevices;
  uint32_t noOfSearchedDevices;
  uint32_t noOfLostDevices;
  uint32_t noOfFoundDevices;


};

#endif