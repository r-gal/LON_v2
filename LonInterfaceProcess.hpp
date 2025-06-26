#ifndef DEVICES_PROCESS_H
#define DEVICES_PROCESS_H

#include "LonDataDef.hpp"
#include "LonInterfacePhy.hpp"
#include "SignalList.hpp"

#define DEV_PHY_PROCESS_TASK_PRIORITY	( configMAX_PRIORITIES-3 )
#define	DEV_PHY_PROCESS_STACK_SIZE	( configMINIMAL_STACK_SIZE * 2 )
#define DEV_PHY_PROCESS_QUEUE_SIZE       64
#define DEV_PHY_EMPTY_QUEUE_SIZE         64
#define DEV_PHY_EMPTY_QUEUE_TRESHOLD     15


#ifdef __cplusplus
extern "C" {
#endif
void DEVPHY_TIMER_INTERRUPT(void);
#ifdef __cplusplus
}
#endif


class LonDeviceInterface_c : public LonInterfacePhy_c, public SignalLayer_c
{

  enum ioState_et
  {
    IO_IDLE,    /* at end, data is set down when clk is high, next = START0 */
    IO_STARTING,
    IO_BUSY,    /* nothing, waiting for IDLE */
    IO_START0,  /* at end, clk is set down, next = START1 */
    IO_START1,  /* at end data is released and next = RC0 or */
                /* data is set and next = TR0 */
    IO_TR0,     /* clock is up, next = TR1 */ 
    IO_TR1,     /* clock is down, next data is set and next = TR0 */
                /* or data is released and next = TRACK0 */ 
    IO_RC0,     /* clock is up, next = RC1 */ 
    IO_RC1,     /* clock is down, next = RC0 or */
                 /* data is down and next = RCACK0 */
    IO_RCACK0,  /* clock is up, next = RCACK1 */ 
    IO_RCACK1,  /* clock is down, next = RCACK0 or */
                 /* data is down and next = END0 */ 
    IO_TRACK0,  /* clock is up, next = TRACK1 */ 
    IO_TRACK1,  /* clock is down, next = TRACK0 or */
                 /* data is down and next = END0 */ 
    IO_END0,    /* data is up, next = END1*/
    IO_END1     /* clock is up, next = BUSY */    
  };
  
  ioState_et ioState;

  bool clk;
  bool dat;
  int8_t bytesToTransmit;
  int8_t checkSum;
  int8_t byteCnt;
  int8_t bitCnt;
  int8_t ackSize;

  LonIOdata_c* actFrame;
  LonIOdata_c* emptyFrame;

  bool TXblocked;
  
  bool checkClk(void);
  bool checkDat(void);
  bool getBit(void);
  void setBit(bool bitVal);
  void setClk(bool data);
  void setDat(bool data);

  public:


  static bool longInstructionAllowed;

  LonDeviceInterface_c(uint8_t portNo_);


  void runFrame(LonIOdata_c* frame_)
  {
    frame_->SetChNo(portNo);
    actFrame = frame_;
    ioState = IO_STARTING;
    frame_->triesCount--;
  
  }
  LonIOdata_c* getActFrame(void)
  {
    return actFrame;
  }

  void proceed(void);



};


class LonInterfaceProcess_c : public process_c
{
  LonDeviceInterface_c* dev_phy_p[NO_OF_BASIC_CHANNELS];

  public :
  LonInterfaceProcess_c(uint16_t stackSize, uint8_t priority, uint8_t queueSize, HANDLERS_et procId);

    void main(void);

};



#endif