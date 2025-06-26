#ifndef CHANNEL_VIRTUAL_H
#define CHANNEL_VIRTUAL_H

#include "LonChannel.hpp"
#include "LonDataDef.hpp"

class LonChannelVirtual_c: public LonChannel_c
{

  LonDevice_c* ownerDevice_p;

  public:


  LonChannelVirtual_c (uint8_t chNo_,LonDevice_c* dev_p_,LonTrafficProcess_c* trafProc_p_);
  ~LonChannelVirtual_c(void);

  void HandleReceivedFrame(LonIOdata_c* recSig_p);

  void SendDetach(void);

  void SendAdrConquest(void);

  void SendDevConfig(LonDevice_c* dev_p);


  void SendCheckDevice(uint32_t lAdr);

  void SendFrame(LonDevice_c* dev_p, uint8_t bytesNo, uint8_t* buffer);

   
};

#endif