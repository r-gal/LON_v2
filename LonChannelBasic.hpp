#ifndef CHANNEL_BASIC_H
#define CHANNEL_BASIC_H







class LonChannelBasic_c : public LonChannel_c
{
  





  

  public:

  LonChannelBasic_c (uint8_t chNo_,LonTrafficProcess_c* trafProc_p_);

  void HandleReceivedFrame(LonIOdata_c* recSig_p);
  void RouteFrame(LonIOdata_c* recSig_p);

  void SendDetach(void);

  void SendAdrConquest(void);

   void SendDevConfig(LonDevice_c* dev_p);


  void CheckDevices(void);


  void SearchNewDevices(void);
  void SendCheckDevice(uint32_t lAdr);

  void SendFrame(LonDevice_c* dev_p, uint8_t bytesNo, uint8_t* buffer);

  

  private: 
  


  

  

  







};


#endif