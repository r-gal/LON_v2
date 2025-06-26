#ifndef LONDATADEF_H
#define LONDATADEF_H

#include "GeneralConfig.h"

#define UP 1
#define DOWN 0



#define MAX_MESSAGE_SIZE 16
#define MAX_ACK_SIZE 5

#define NO_OF_CHANNELS (NO_OF_BASIC_CHANNELS + NO_OF_VIRTUAL_CHANNELS)


/*

  FRAME_ADRCONQUEST | 0  | C/3  | CS | [A0] [A1] [A2] [A3] [ACK/CS]
  FRAME_DEVCONFIG   | 0  | C/13 | A0..A3 | Sadr|Conf0|Conf1|Conf2|Conf3|Conf4|CS| [ACK/CS]
  FRAME_CHECKDEVICE | 0  | C/7 | A0..A3 | CS | [ACK/CS]
  FRAME_DATA_SW     |Sadr| C/N+3 | Event0..EventN | CS | [ACK/CS]
  FRAME_DATA_TR     | 0  | C/6  |Sadr| ChNr | Data | CS | [ACK/CS]
  FRAME_READ_IO     | 0  | C/5 | Sadr | ChNr | CS | [data] [ACK/CS]

  FRAME_DETACH_ALL  | 0  | C/3 | CS  | [A0]

  FRAME_REQUEST     | 0  | C/6   | Sadr | REQ | data | CS | [ ACK/CS]
  FRAME_DATA        | Sadr | C/N+4 | REQ | data0...dataN | CS | [ACK/CS]
  FRAME_READ_STATUS | 0  | C/5  |Sadr | Mode | CS | [aV] | [minV] | [maxV] | [noCol] | [ACK/CS]
*/

  /*conf 0 - enabled*/
  /*conf 1 - monostable*/
  /*conf 2 - inverted*/
  /*conf 3 - pressCounterMax*/
  /*conf 4 - waitCounterMax*/


class LonDevice_c;
typedef void (*Function_type)(LonDevice_c*,void*);
  
enum event_et
{
	EVENT_NO_EVENT = 0,
	EVENT_CHANGE_UP = 1,
	EVENT_CHANGE_DOWN = 2,
	EVENT_PRESSED = 3,
	EVENT_RELEASED = 4,
	EVENT_SERIES_CLICK = 5,
	EVENT_HOLD = 6
};

enum IOact_et
{
  IOACK_NOK = 0,
  IOACK_OK = 1,
  IOACK_NORESP
};

enum frameCode_et
{  
    FRAME_ADRCONQUEST = 0,
    FRAME_DEVCONFIG = 1,
    FRAME_CHECKDEVICE = 2,
    FRAME_DATA_SW = 3,
    FRAME_DATA_TR = 4,
    FRAME_READ_IO = 5,
    FRAME_READ_STATUS = 6,
    FRAME_DETACH_ALL = 7,
    FRAME_REQUEST = 8,
    FRAME_DATA = 9,
    FRAME_READ_IO_EXT = 10,
    FRAME_CONTAINER = 11,
    FRAME_NO_OF_TYPES = 11
};
enum frameAck_et
{
   ACK_NORMAL = 0,
   ACK_ERROR = 1,
   ACK_UNKNOWN = 2,
   ACK_INVALID_ARG = 3,
  ACK_INVALID_CHECKSUM = 0xF
};

static const uint8_t ackSize[] =
{
  5,  /*FRAME_ADRCONQUEST = 0,*/
  1,  /*FRAME_DEVCONFIG = 1,*/
  1,  /*FRAME_CHECKDEVICE = 2,*/
  1,  /*FRAME_DATA_SW = 3,*/
  1,  /*FRAME_DATA_TR = 4,*/
  2,  /*FRAME_READ_IO = 5,*/
  1,  /*FRAME_READ_STATUS = 6,*/
  1,  /*FRAME_DETACH_ALL = 7,*/
  1,  /*FRAME_REQUEST = 8,*/
  1,  /*FRAME_DATA = 9,*/
  5,   /*FRAME_READ_IO_EXT = 10,*/
  1   /*FRAME_CONTAINER = 11,*/
};

enum devType_et
{
  SW3 = 0,
  SW6 = 1,
  RL2 = 2,
  RL4 = 3,
  RL8 = 4,
  MX1 = 5, /* unused */
  HIG = 6,
  MX2 = 7,
  PRS = 8,
  PWM = 9,
  VIR = 10,
  UNN = 11,
  DEV_TYPES_NO
};

static const uint8_t devPrefix[] = { 0x10,0x20,0x50,0x60,0x70, 0xB0, 0xA0, 0xC0, 0xD0, 0xE0, 0x90, 0xFF };
static const char* devTypeString[] = {"SW3","SW6","RL2","RL4","RL8","MX1","HIG","MX2","PRS","PWM","VIR","UNN"};

static const uint8_t portMasc[][8] = {{ 0x01,0x03,0x04,0xFF,0xFF,0xFF,0xFF,0xFF,}, /*SW3*/  
                                      { 0x00,0x01,0x02,0x03,0x05,0x07,0xFF,0xFF,}, /*SW6*/ 
                                      { 0x13,0x14,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,}, /*RL2*/ 
                                      { 0x10,0x11,0x13,0x17,0xFF,0xFF,0xFF,0xFF,}, /*RL4*/ 
                                      { 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,}, /*RL8*/ 
                                      { 0x00,0x07,0x11,0x12,0x13,0xFF,0xFF,0xFF,}, /*MX1*/ 
                                      { 0x20,0x21,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,}, /*HIG*/ 
                                      { 0x00,0x01,0x02,0x13,0x14,0x15,0x16,0x17,}, /*MX2*/
                                      { 0x30,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,}, /*PRS*/
                                      { 0x01,0x02,0x10,0x4B,0x4C,0x4D,0xFF,0xFF,}, /*PWM*/
                                      { 0x50,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,}, /*VIR*/
                                      { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,}};/*UNN*/ 







struct STATS_DATA_st
{
float min;
float max;
union 
{
float sum;
float average;
};

};

#endif