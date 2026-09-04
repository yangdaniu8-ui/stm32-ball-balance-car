#include "ps2_control.h"
#include "stm32f10x.h"
#include "sys.h"
#include "moto.h"
#include "PWM.h"
#include "delay.h"

/**
* @par Copyright (C): 2010-2019, Shenzhen Yahboom Tech
* @file         app_ps2Control.c
* @author       liusen
* @version      V1.0
* @date         2017.07.17
* @brief        ps2业务控制流程源文件
* @details      
* @par History  
*                 
* version:		liusen_20170717
*/

u16 Handkey;
u8 Comd[2]={0x01,0x42};	//开始命令。请求数据
u8 scan[9]={0x01,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00};//{0x01,0x42,0x00,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A};	// 类型读取

u8 Data[9]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //数据存储数组
u16 MASK[]={
    PSB_SELECT,
    PSB_L3,
    PSB_R3 ,
    PSB_START,
    PSB_PAD_UP,
    PSB_PAD_RIGHT,
    PSB_PAD_DOWN,
    PSB_PAD_LEFT,
    PSB_L2,
    PSB_R2,
    PSB_L1,
    PSB_R1 ,
    PSB_GREEN,
    PSB_RED,
    PSB_BLUE,
    PSB_PINK
};	//按键值与按键明

int CarSpeedControl = 4500;

int g_CarState = enSTOP;

void delay_1us(u32 n)
{
	u8 j;
	while(n--)
	for(j=0;j<10;j++);
}

void PS2_Init(void)
{

	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);  	/*开启GPIOAB的外设时钟*/
	
  	GPIO_InitStructure.GPIO_Pin = PS2_MISO_PIN | PS2_SCK_PIN;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;      /*设置引脚模式为通用推挽输出*/
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  	GPIO_Init(PS2_SCK_PORT, &GPIO_InitStructure);	

	GPIO_InitStructure.GPIO_Pin = PS2_CS_PIN ;	
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;      /*设置引脚模式为通用推挽输出*/
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  	GPIO_Init(PS2_CS_PORT, &GPIO_InitStructure);	


	GPIO_InitStructure.GPIO_Pin = PS2_MOSI_PIN;	//DI
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;     
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(PS2_MOSI_PORT, &GPIO_InitStructure);
	
	DO_H;
	CLC_H;
	CS_H;
}

//读取手柄数据
u8 PS2_ReadData(u8 command)
{
	u8 i,j=1;
	u8 res=0; 
    for(i=0; i<=7; i++)          
    {
		if(command&0x01)
			DO_H;
		else
			DO_L;
		command = command >> 1;
		delay_1us(10);
		CLC_L;
		delay_1us(10);
		if(DI) 
			res = res + j;
		j = j << 1; 
		CLC_H;
		delay_1us(10);	 
    }
    DO_H;
	delay_1us(50);
    return res;	
}

//对读出来的 PS2 的数据进行处理
//按下为 0， 未按下为 1
unsigned char PS2_DataKey()
{
	u8 index = 0, i = 0;

	PS2_ClearData();
	CS_L;
	for(i=0;i<9;i++)	//更新扫描按键
	{
		Data[i] = PS2_ReadData(scan[i]);	
	} 
	CS_H;
	

	Handkey=(Data[4]<<8)|Data[3];     //这是16个按键  按下为0， 未按下为1
	for(index=0;index<16;index++)
	{	    
		if((Handkey&(1<<(MASK[index]-1)))==0)
			return index+1;
	}
	return 0;          //没有任何按键按下
}

//得到一个摇杆的模拟量	 范围0~256
u8 PS2_AnologData(u8 button)
{
	return Data[button];
}

//清除数据缓冲区
void PS2_ClearData()
{
	u8 a;
	for(a=0;a<9;a++)
		Data[a]=0x00;
}



/**
* Function       app_ps2_deal
* @author        liusen
* @date          2017.08.23    
* @brief         PS2协议处理
* @param[in]     void
* @param[out]    void
* @retval        void
* @par History   无
*/
void app_ps2_deal(void)
{
	u8 PS2_KEY = 0, X1=0,Y1=0,X2=0,Y2=0;

	__set_PRIMASK(1);  //关中断；       
	PS2_KEY = PS2_DataKey();	 //手柄按键捕获处理
	__set_PRIMASK(0);//开中断

	switch(PS2_KEY)
	{
		case PSB_SELECT: 	printf("PSB_SELECT \n");  break;
		case PSB_L3:     	g_CarState = enSTOP;  printf("PSB_L3 \n");  break;  
		case PSB_R3:     	g_CarState = enSTOP;	 printf("PSB_R3 \n");  break;  
		case PSB_START:  	printf("PSB_START \n");  break;  
		case PSB_PAD_UP: 	g_CarState = enRUN;   printf("PSB_PAD_UP \n");  break;  
		case PSB_PAD_RIGHT:	g_CarState = enRIGHT; printf("PSB_PAD_RIGHT \n");  break;
		case PSB_PAD_DOWN:	g_CarState = enBACK;  printf("PSB_PAD_DOWN \n");  break; 
		case PSB_PAD_LEFT:	g_CarState = enLEFT;  printf("PSB_PAD_LEFT \n");  break; 
		case PSB_L2:      	
		{
			CarSpeedControl += 600;
			if (CarSpeedControl > 7200)
			{
				CarSpeedControl = 7200;
			}
			 
			printf("PSB_L2 \n");  	

		}break; 
		case PSB_R2:      	
		{
			CarSpeedControl -= 600;
			if (CarSpeedControl < 4000)
			{
				CarSpeedControl = 4000;
			} 
			printf("PSB_R2 \n");

		}  break; 
		case PSB_L1:      	printf("PSB_L1 \n");  break; 
		case PSB_R1:      	printf("PSB_R1 \n");  break;     
	
		case PSB_CIRCLE:  	g_CarState = enTRIGHT; printf("PSB_CIRCLE \n");  break;  	
		case PSB_SQUARE:  	g_CarState = enTLEFT; printf("PSB_SQUARE \n");  break;  	
		default: g_CarState = enSTOP; break; 
	}
	//获取模拟值
	if(PS2_KEY == PSB_L1 || PS2_KEY == PSB_R1)
	{
		X1 = PS2_AnologData(PSS_LX);
		Y1 = PS2_AnologData(PSS_LY);
		X2 = PS2_AnologData(PSS_RX);
		Y2 = PS2_AnologData(PSS_RY);
		/*左摇杆*/
	    if (Y1 < 5 && X1 > 80 && X1 < 180) //上
	    {
	      g_CarState = enRUN;
		  
	    }
	    else if (Y1 > 230 && X1 > 80 && X1 < 180) //下
	    {
	      g_CarState = enBACK;
		  
	    }
	    else if (X1 < 5 && Y1 > 80 && Y1 < 180) //左
	    {
	      g_CarState = enLEFT;

	    }
	    else if (Y1 > 80 && Y1 < 180 && X1 > 230)//右
	    {
	      g_CarState = enRIGHT;

	    }
	    else if (Y1 <= 80 && X1 <= 80) //左上
	    {
	      g_CarState = enUPLEFT;

	    }
	    else if (Y1 <= 80 && X1 >= 180) //右上
	    {
	      g_CarState = enUPRIGHT;
	
	    }
	    else if (X1 <= 80 && Y1 >= 180) // 左下
	    {
	      g_CarState = enDOWNLEFT;
	
	    }
	    else if (Y1 >= 180 && X1 >= 180) //右下
	    {
	      g_CarState = enDOWNRIGHT;
		  
	    }
	    else//停
	    {
	      g_CarState = enSTOP;
	    }
	}

	
}

void app_CarstateOutput(void)
{
//根据小车状态做相应的动作
	switch (g_CarState)
	{
		case enSTOP: Stop(); break;
		case enRUN: Forward(CarSpeedControl); break;
 		case enLEFT: Turnleft(CarSpeedControl); break;
		case enRIGHT: Turnright(CarSpeedControl); break;
		case enBACK: Backward(CarSpeedControl); break;
		case enTLEFT: SpinLeft(CarSpeedControl); break;
		case enTRIGHT: SpinRight(CarSpeedControl); break;

		case enUPLEFT:break;//左上转
		case enDOWNLEFT:break;//左下转
		case enUPRIGHT:break;//右上转 
		case enDOWNRIGHT:break;//右下转  
		default: Stop(); break;
	}

}
