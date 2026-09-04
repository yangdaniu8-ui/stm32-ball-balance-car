#include "iravoid.h"
#include "stm32f10x.h"
#include "sys.h"
#include "moto.h"
#include "delay.h"

/**
* Function       IRAvoid_GPIO_Init
  
* @brief         红外避障传感器GPIO初始化接口
* @param[in]     void
* @param[out]    void
* @retval        void
* @par History   无
*/
void IRAvoid_GPIO_Init(void)
{
	/*定义一个GPIO_InitTypeDef类型的结构体*/
	GPIO_InitTypeDef GPIO_InitStructure;

#ifdef USE_IRAVOID_L1
	/*开启外设时钟*/
	RCC_APB2PeriphClockCmd(IRAvoid_L1_RCC, ENABLE); 
	/*选择要控制的引脚*/															   
  	GPIO_InitStructure.GPIO_Pin = IRAvoid_L1_PIN;	
	/*设置引脚模式为通用推挽输出*/
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   
	/*设置引脚速率为50MHz */   
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	/*调用库函数，初始化Servo_J4_PORT*/
  	GPIO_Init(IRAvoid_L1_PORT, &GPIO_InitStructure);		 
#endif

#ifdef USE_IRAVOID_R1
	/*开启外设时钟*/
	RCC_APB2PeriphClockCmd(IRAvoid_R1_RCC, ENABLE); 
	/*选择要控制的引脚*/															   
  	GPIO_InitStructure.GPIO_Pin = IRAvoid_R1_PIN;	
	/*设置引脚模式为通用推挽输出*/
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   
	/*设置引脚速率为50MHz */   
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	/*调用库函数，初始化Servo_J4_PORT*/
  	GPIO_Init(IRAvoid_R1_PORT, &GPIO_InitStructure);	
#endif

}
/**
* Function       bsp_GetIRavoid  
* @brief         避障传感器获取状态
* @param[in]     int *p_iL1, int *p_iR1 左右变量指针
* @param[out]    void
* @retval        void
* @par History   无
*/

void bsp_GetIRavoid(int *p_iL1, int *p_iR1)
{
	*p_iL1 = GPIO_ReadInputDataBit(IRAvoid_L1_PORT, IRAvoid_L1_PIN);
	*p_iR1 = GPIO_ReadInputDataBit(IRAvoid_R1_PORT, IRAvoid_R1_PIN);	
}

/**
* Function       bsp_GetIRavoid_Data   
* @brief         避障传感器获取状态上报调用
* @param[in]     void
* @param[out]    void
* @retval        void
* @par History   无
*/

int bsp_GetIRavoid_Data(void)
{
	int L1,R1,ReturnValue;
	L1 = GPIO_ReadInputDataBit(IRAvoid_L1_PORT, IRAvoid_L1_PIN);
	R1 = GPIO_ReadInputDataBit(IRAvoid_R1_PORT, IRAvoid_R1_PIN);
	ReturnValue = (L1 == 0 ? 10 : 0);
	ReturnValue += (R1 == 0 ? 1 : 0);
	return ReturnValue;
}


/**
* Function       app_IRAvoid
* @brief         红外避障模式运动
* @param[in]     void
* @param[out]    void
* @retval        void
* @par History   无
*/
void app_IRAvoid(void)
{
	int IRL1 = 1, IRR1 = 1;

	bsp_GetIRavoid(&IRL1, &IRR1);

	/*右侧检测到*/
  	if( IRL1 == HIGH && IRR1 == LOW)
    {
      	SpinLeft(7000);
    }
	/*左侧检测到*/
    else if ( IRL1 == LOW && IRR1 == HIGH)
    {
      	SpinRight(7000);
    }
	/*左右均未检测到*/  
    else if (IRL1 == HIGH && IRR1 == HIGH)
    {   
		Forward(6000);
	}
	/*左右均检测到*/
    else if(IRL1 == LOW && IRR1 == LOW) 
    {  
		SpinRight(7000);
		delay_ms(500);
	}	
}

