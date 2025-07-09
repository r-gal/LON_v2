#ifndef SIGNALLIST_C
#define SIGNALLIST_C

class SignalList_c
{

  public: enum HANDLERS_et
  {
    HANDLE_CTRL,
    HANDLE_TCP,
    HANDLE_HTTP,
    HANDLE_TELNET,
    HANDLE_FTP,

    HANDLE_LON_TRAFFIC,
    HANDLE_LON_INTERFACE,
    HANDLE_LON_SENSORS_DATABASE,
    HANDLE_POWER,
    HANDLE_LON_PHY_0,
    HANDLE_LON_PHY_1,
    HANDLE_LON_PHY_2,
    HANDLE_LON_PHY_3,
    HANDLE_LON_PHY_4,
    HANDLE_LON_PHY_5,
    HANDLE_LON_PHY_6,
    HANDLE_LON_PHY_7,
    HANDLE_UART,
    HANDLE_NO_OF

  };


  public: enum SIGNO_et
  {
    SIGNO_TEST = 0,
    SIGNO_SDIO_CardDetectEvent = 1,

    SIGNO_TCP_LINKEVENT,
    SIGNO_TCP_RXEVENT,
    SIGNO_TCP_DHCP_TIMEOUT,
    SIGNO_TCP_SEND_TIMER,
    SIGNO_TCP_TICK,
    SIGNO_SOCKET_SEND_REQUEST,
    SIGNO_SOCKET_REC_REQUEST,
    SIGNO_SOCKET_TCP_REQUEST,
    SIGNO_SOCKET_ADD,
    SIGNO_IP_CHANGED,

    SIGNO_LOG,

    SIGNO_LON_INTERFACE_TICK,
    SIGNO_LON_IODATAOUT,
    SIGNO_LON_IODATAIN_TRAF,
    SIGNO_LON_IODATAIN_CTRL,

    SIGNO_LON_DEVCHECK_TICK,
    SIGNO_LON_TMOSCAN_TICK,
    SIGNO_LON_TIME,

    SIGNO_LON_SET_OUTPUT,
    SIGNO_LON_GET_OUTPUT,
    SIGNO_LON_DEVICECONFIG,
    SIGNO_LON_GETDEVLIST,
    SIGNO_LON_GETDEVINFO,

    SIGNO_LON_SENSOR_DATA,
    SIGNO_LON_GET_SENSOR_VALUES,
    SIGNO_LON_GET_SENSOR_HISTORY,
    SIGNO_LON_GET_OUTPUTS_LIST,

    SIGNO_PWR_TIM_TICK,
    SIGNO_PWR_ADC_READY,

    SIGNO_LON_GET_POWER_INFO,
    SIGNO_CMD_POWERINFO,

    SIGNO_LON_5_MIN_TICK,
    SIGNO_LON_GET_WEATHER_STATS,
    SIGNO_LON_GET_SENSOR_STATS,
    SIGNO_LON_IRRIG_GETSTAT,

    SIGNO_LON_IRRIG_UPD_CONFIG


  };
};

#include "CommonSignal.hpp"

#include "LonSignals.hpp"

class testSig_c : public Sig_c
{
  public:
  testSig_c(void) : Sig_c(SIGNO_TEST,HANDLE_CTRL) {}

};

#if CONF_USE_ETHERNET == 1

class tcpRxEventSig_c: public Sig_c
{
  public:
  tcpRxEventSig_c(void) : Sig_c(SIGNO_TCP_RXEVENT,HANDLE_TCP) {dataBuffer = nullptr;}

  uint8_t* dataBuffer;
  uint32_t dataSize;

};

class tcpLinkEventSig_c: public Sig_c
{
  public:
  tcpLinkEventSig_c(void) : Sig_c(SIGNO_TCP_LINKEVENT,HANDLE_TCP) {}

  uint8_t linkState;

};

class tcpDhcpTimerSig_c: public Sig_c
{
  public:
  tcpDhcpTimerSig_c(void) : Sig_c(SIGNO_TCP_DHCP_TIMEOUT,HANDLE_TCP) {}

  uint8_t timerIndicator;

};


class tcpSendSig_c : public Sig_c
{
  public:
  tcpSendSig_c(void) : Sig_c(SIGNO_TCP_SEND_TIMER,HANDLE_TCP) {}

};

class tcpTickSig_c : public Sig_c
{
  public:
  tcpTickSig_c(void) : Sig_c(SIGNO_TCP_TICK,HANDLE_TCP) {}

};

#include "TcpDataDef.hpp"
class socketSendReqSig_c : public Sig_c
{
  public:
  socketSendReqSig_c(void) : Sig_c(SIGNO_SOCKET_SEND_REQUEST,HANDLE_TCP) {}
  SocketTcp_c* socket;
  TaskHandle_t task;
  uint32_t bufferSize;
  uint32_t bytesSent;
  uint8_t* buffer_p;

};

class socketReceiveReqSig_c : public Sig_c
{
  public:
  socketReceiveReqSig_c(void) : Sig_c(SIGNO_SOCKET_REC_REQUEST,HANDLE_TCP) {}
  SocketTcp_c* socket;
  TaskHandle_t task;
  uint32_t bufferSize;
  uint32_t bytesReceived;
  uint8_t* buffer_p;

};


class socketTcpRequestSig_c: public Sig_c
{
  public:
  SocketRequest_et code;
  socketTcpRequestSig_c(void) : Sig_c(SIGNO_SOCKET_TCP_REQUEST,HANDLE_TCP) {}
  SocketTcp_c* socket;
  TaskHandle_t task;

  SocketAdress_st* soccAdr;
  uint32_t bufferSize;
  uint8_t* buffer_p;
  uint8_t clientMaxCnt;
};

class ipChanged_c : public Sig_c
{
  public:
  ipChanged_c(void) : Sig_c(SIGNO_IP_CHANGED,HANDLE_TCP) {}

};


class socketAddSig_c : public Sig_c
{
  public:
  socketAddSig_c(void) : Sig_c(SIGNO_SOCKET_ADD,HANDLE_TCP) {}
  class Socket_c* socket;
};

#endif

class pwrTimTick_c : public Sig_c
{
  public:
  pwrTimTick_c(void) : Sig_c(SIGNO_PWR_TIM_TICK,HANDLE_POWER) {}
  SystemTime_st time;

};

class pwrAdcReady_c : public Sig_c
{
  public:
  pwrAdcReady_c(void) : Sig_c(SIGNO_PWR_ADC_READY,HANDLE_POWER) {}

};

class pwrGetInfo_c : public Sig_c
{
  public:
  pwrGetInfo_c(void) : Sig_c(SIGNO_CMD_POWERINFO,HANDLE_POWER) {data_p = nullptr;}

  TaskHandle_t task;
  uint8_t lastIdx;
  uint8_t lastIdxMinutes;

  uint32_t* data_p;
  uint32_t* data2_p;

};

#if CONF_USE_LOGGING == 1

#define LOG_TEXT_LENGTH 128
class logSig_c : public Sig_c
{
  public:
  logSig_c(void) : Sig_c(SIGNO_LOG,HANDLE_CTRL)
   {
     text[0] = 0;
   }

  SystemTime_st time;
  uint16_t size;
  uint8_t level;
  uint8_t spare;
  char  text[LOG_TEXT_LENGTH];
  

};
#endif


#endif