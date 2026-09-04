#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "delay.h"
#include "moto.h"
#include "servo.h"
#include "linewalking.h"
#include "ultrasonic.h"
#include "usart.h"
#include "adc.h"


/*小车运行状态枚举*/
enum{

  enSTOP = 0,
  enRUN,
  enBACK,
  enLEFT,
  enRIGHT,
  enUPLEFT,
  enUPRIGHT,
  enDOWNLEFT,
  enDOWNRIGHT,  
  enTLEFT,
  enTRIGHT

};

extern int g_CarState; //  1前2后3左4右0停止
extern int CarSpeedControl;


void Protocol(void);



#endif
