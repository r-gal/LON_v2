#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#if LON_USE_SENSORS == 1

#include "SignalList.hpp"
#include "LonDataDef.hpp"

#include "LonDevice.hpp"
#include "LonSensors.hpp"
#include "LonTrafficProcess.hpp"

#include "LonDatabase.hpp"

LonTrafficProcess_c* LonSensors_c::trafProc_p;

void LonSensors_c::SendRequest(LonDevice_c* dev_p, uint8_t port)
{
  uint8_t buffer[4] = {FRAME_REQUEST<<4 | 6, dev_p->GetSadr(),  0,port};

  trafProc_p->SendFrame(dev_p,4, buffer);


  //printf("sent sensor request %X/%d \n",dev_p->GetLAdr(),port);

}

void LonSensors_c::ReadSensor(LonDevice_c* dev_p,void* userPtr)
{
  switch(dev_p->GetState())
  {
    case LonDevice_c::RUNNING:
    case LonDevice_c::CONFIGURED:
    case LonDevice_c::CONFIGURING:
    {
      devType_et type = dev_p->GetDevType();
      if((type == HIG) || (type == PRS))
      {
        for(uint8_t i =0; i<3;i++)
        {
          if( dev_p->IsEnabled(i) )
          {
            SendRequest(dev_p,i);
            break;
          }
        }
      }
      break;
    }
    default:
    break;
  }
}

LonSensors_c::LonSensors_c(LonTrafficProcess_c* trafProc_p_) 
{
  trafProc_p = trafProc_p_;


}

void LonSensors_c::ScanSensors(LonTime_c* recSig_p)
{
  uint32_t lAdr;
  trafProc_p->RunForAll(ReadSensor,nullptr);

}

void LonSensors_c::HandleSensorData(LonDevice_c* dev_p, uint8_t port, uint8_t* sensorData)
{

  //printf("received sensor data %X/%d \n",dev_p->GetLAdr(),port);
  devType_et type = dev_p->GetDevType();
  if(type == HIG)
  {
    handleDataHig(dev_p, port, sensorData);
  }
  else if(type == PRS)
  {
    handleDataPress(dev_p, port, sensorData);
  }

  for(uint8_t i = (port+1); i<3;i++)
  {
    if( dev_p->IsEnabled(i) )
    {
      SendRequest(dev_p,i);
      break;
    }
  }
 
}

void LonSensors_c::handleDataHig(LonDevice_c* dev_p, uint8_t portNo, uint8_t* sensorData)
{
 //printf("rec HIgro data, port=%d\n",portNo);

  if((portNo == 0) | (portNo == 1))
  {
    ((LonPortDataHigro_c*)dev_p->GetPortData(portNo))->SetMeasurement(sensorData);

    CheckSensorAlarm(dev_p,portNo);

    float hum = ((LonPortDataHigro_c*)dev_p->GetPortData(portNo))->GetSensorValue(0);
    float temp = ((LonPortDataHigro_c*)dev_p->GetPortData(portNo))->GetSensorValue(1);

    #if LON_USE_SENSORS_DATABASE == 1
    LonSensorData_c* sig_p = new LonSensorData_c;
    sig_p->lAdr = dev_p->GetLAdr();
    sig_p->port = portNo;
    sig_p->data[0] = hum;
    sig_p->data[1] = temp;
   // printf("portNo=%d h=%d t=%d \n",portNo,sig_p->data1,sig_p->data2);
    sig_p->type = 0;
    sig_p->Send();
    #endif

    #if LON_USE_WEATHER_UNIT == 1
    trafProc_p->weatherUnit.HandleSensorData(dev_p->GetLAdr(),portNo,hum,temp); /* temperature and humidity sensors data */
    #endif
  }
}

void LonSensors_c::handleDataPress(LonDevice_c* dev_p, uint8_t portNo, uint8_t* sensorData)
{
  if(portNo == 0 )
  {  
    
    ((LonPortDataPress_c*)dev_p->GetPortData(portNo))->SetMeasurement(sensorData);

    float press = ((LonPortDataPress_c*)dev_p->GetPortData(portNo))->GetSensorValue(0);

    #if LON_USE_SENSORS_DATABASE == 1
    LonSensorData_c* sig_p = new LonSensorData_c;
    sig_p->lAdr = dev_p->GetLAdr();
    sig_p->port = portNo;
    sig_p->data[0] = press;
    sig_p->type = 1;
    sig_p->Send();
    #endif

    #if LON_USE_WEATHER_UNIT == 1
    trafProc_p->weatherUnit.HandleSensorData(dev_p->GetLAdr(),0,press,0); /* pressure sensors data */
    #endif
  }
}


void LonSensors_c::CheckSensorAlarm(LonDevice_c* dev_p, uint8_t portNo)
{
  LonDatabase_c database;
  LonConfigPage_st* config = database.ReadConfig(dev_p->GetLAdr());

  if(config != NULL)
  {
    LonPortData_st* portData = &(config->action[portNo].portData);

    uint16_t hVal,tVal;
    uint8_t newState = 0;
    bool stateValid = false;

    if(portData->hSensor.conf != 0)
    {
      hVal =  ((LonPortDataHigro_c*)dev_p->GetPortData(portNo))->GetSensorValue(0);
      int16_t onVal = (portData->hSensor.level) ;
      int16_t offVal = (portData->hSensor.level - portData->hSensor.hist) ;

      if(hVal >=onVal)
      {
        if(portData->hSensor.conf == 1)
        {          
          newState = 1;
        }
        stateValid = true;
      }
      if(hVal <= offVal)
      {
        if(portData->hSensor.conf == 2)
        {
          newState = 1;
        }
        stateValid = true;
      }      
    }
    if(portData->tSensor.conf != 0)
    {
      tVal = ((LonPortDataHigro_c*)dev_p->GetPortData(portNo))->GetSensorValue(1);
      int16_t onVal = (portData->tSensor.level) ;
      int16_t offVal = (portData->tSensor.level - portData->tSensor.hist) ;

      if(tVal >=onVal)
      {
        if(portData->tSensor.conf == 1)
        {
          newState = 1;
        }
        stateValid = true;
      }
      if(tVal <= offVal)
      {
        if(portData->tSensor.conf == 2)
        {
          newState = 1;
        }
        stateValid = true;
      }
    }

    if(stateValid)
    {
      LonDevice_c* outDev_p;
      if(portData->out1LAdr > 0)
      {
        outDev_p = trafProc_p->GetDevByLadr(portData->out1LAdr);
        if(outDev_p != NULL)
        {
          if(outDev_p->GetOutputVal(portData->out1Port) != newState)
          {
            outDev_p->SetOutput(portData->out1Port,newState);
          }
        }
      }
      if(portData->out2LAdr > 0)
      {
        outDev_p = trafProc_p->GetDevByLadr(portData->out2LAdr);
        if(outDev_p != NULL)
        {
          if(outDev_p->GetOutputVal(portData->out2Port) != newState)
          {
            outDev_p->SetOutput(portData->out2Port,newState);
          }
        }
      }
    }
  }
}


#endif






