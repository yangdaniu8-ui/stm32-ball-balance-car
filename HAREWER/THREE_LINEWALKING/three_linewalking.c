#include "stm32f10x.h"
#include "three_linewalking.h"
#include "sys.h"
#include "moto.h"
#include "delay.h"

/**
* Function       three_LineWalking_GPIO_Init
* @brief         巡线传感器GPIO初始化接口
* @param[in]     void
* @param[out]    void
* @retval        void
* @par History   无
*/
void three_LineWalking_GPIO_Init(void)
{
	/*定义一个GPIO_InitTypeDef类型的结构体*/
	GPIO_InitTypeDef GPIO_InitStructure;

#ifdef USE_LINE_L
	/*开启外设时钟*/
	RCC_APB2PeriphClockCmd(LineWalk_L_RCC, ENABLE); 
	/*选择要控制的引脚*/															   
  	GPIO_InitStructure.GPIO_Pin = LineWalk_L_PIN;	
	/*设置引脚模式为通用推挽输出*/
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   
	/*设置引脚速率为50MHz */   
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	/*调用库函数，初始化PORT*/
  	GPIO_Init(LineWalk_L_PORT, &GPIO_InitStructure);		 
#endif

#ifdef USE_LINE_M
	/*开启外设时钟*/
	RCC_APB2PeriphClockCmd(LineWalk_M_RCC, ENABLE); 
	/*选择要控制的引脚*/															   
  	GPIO_InitStructure.GPIO_Pin = LineWalk_M_PIN;	
	/*设置引脚模式为通用推挽输出*/
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   
	/*设置引脚速率为50MHz */   
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	/*调用库函数，初始化PORT*/
  	GPIO_Init(LineWalk_M_PORT, &GPIO_InitStructure);	
#endif

#ifdef USE_LINE_R
	/*开启外设时钟*/
	RCC_APB2PeriphClockCmd(LineWalk_R_RCC, ENABLE); 
	/*选择要控制的引脚*/															   
  	GPIO_InitStructure.GPIO_Pin = LineWalk_R_PIN;	
	/*设置引脚模式为通用推挽输出*/
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   
	/*设置引脚速率为50MHz */   
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	/*调用库函数，初始化PORT*/
  	GPIO_Init(LineWalk_R_PORT, &GPIO_InitStructure);	
#endif
	
}


/**
* Function       three_GetLineWalking
* @brief         获取巡线状态
* @param[in]     int *p_iL, int *p_iM,  int *p_iR  三路巡线位指针
* @param[out]    void
* @retval        void
* @par History   无
*/
void three_GetLineWalking(int *p_iL, int *p_iM, int *p_iR)
{
	*p_iL = GPIO_ReadInputDataBit(LineWalk_L_PORT, LineWalk_L_PIN);
	*p_iM = GPIO_ReadInputDataBit(LineWalk_M_PORT, LineWalk_M_PIN);
	*p_iR = GPIO_ReadInputDataBit(LineWalk_R_PORT, LineWalk_R_PIN);
		
}

/**
* Function       three_LineWalking  
* @brief         巡线模式运动
* @param[in]     void
* @param[out]    void
* @retval        void
* @par History   无
*/
void three_LineWalking(void)
{
	int LineL = 0, LineM = 0, LineR = 0;

	three_GetLineWalking(&LineL, &LineM, &LineR);	//获取黑线检测状态	

//  if( LineL == HIGH &&  LineM == HIGH) //直角锐角
//    {  
//		SpinLeft(7000);
//		delay_ms(30);
//	}
//    else if ( LineM == HIGH && LineR == HIGH) //直角锐角
//    {  
//		SpinRight(7000);
//		delay_ms(30);
//	}
//    if (LineL == LOW ) //左最外侧检测微调车左转
//    {   
//		Turnleft(6000);   
//	}
//	else 
		if ( LineR == HIGH) //右最外侧检测微调车右转
    {   
		Turnright(6000);   
	}
//    else if(LineM == LOW ) // 黑色, 加速前进
//    {  
//		Forward(6000);
//	}	
//   else if(LineL == HIGH && LineM == HIGH && LineR == HIGH) // 黑色, 加速前进
//    {  
//		Forward(6000);
//	}	
//		

}



