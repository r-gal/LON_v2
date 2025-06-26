#ifndef HTTP_INTERFACE_H
#define HTTP_INTERFACE_H

#include "HTTP_ServerProcess.hpp"

class LonHttpInterface_c: public HttpInterface_c
{




  void HandleCommand(char* line,char* extraData,SocketTcp_c* socket);



 void HandleGetSensorsValues(char* argsStr, SocketTcp_c* socket);
 void HandleGetSensorHistory(char* argsStr,  SocketTcp_c* socket);
 void HandleSetRelVal(char* argsStr,  SocketTcp_c* socket);
 void HandleGetOutputs(char* argsStr,  SocketTcp_c* socket,bool fullData);
 void HandleGetPowerInfo(char* argsStr,  SocketTcp_c* socket);

 void HandleGetTimers(char* argsStr,  SocketTcp_c* socket);
 void HandleSetTimer(char* argsStr,  SocketTcp_c* socket);


 bool ParseInt(char* valStr, uint8_t strLen, uint32_t* valInt);
 int FetchNextArgString(char** startStr,uint32_t *argVal);
 bool FetchNextArg(char** startStr,uint32_t *argVal);


};




#endif


