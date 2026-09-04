#include "ultrasonic.h"
#include "stm32f10x.h"
#include "delay.h"
#include "servo.h"
#include "sys.h"
#include "moto.h"
#include "PWM.h"
#include "usart.h"	

/*��¼��ʱ���������*/
unsigned int overcount = 0;

/**
* Function       Ultrasonic_GPIO_Init
* @brief         ������������GPIO��ʼ���ӿ�
* @param[in]     void
* @param[out]    void
* @retval        void
* @par History   ��
*/
void Ultrasonic_GPIO_Init(void)
{

#ifdef USE_ULTRASONIC

	/*����һ��GPIO_InitTypeDef���͵Ľṹ��*/
	GPIO_InitTypeDef GPIO_InitStructure;

	/*��������ʱ��*/
	RCC_APB2PeriphClockCmd(TRIG_RCC | RCC_APB2Periph_AFIO, ENABLE); 
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);  //����jtag
	/*TRIG�����ź�*/														   
  	GPIO_InitStructure.GPIO_Pin = TRIG_PIN;	
	/*��������ģʽΪͨ���������*/
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   
	/*������������Ϊ50MHz */   
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	/*���ÿ⺯������ʼ��*/
  	GPIO_Init(TRIG_PORT, &GPIO_InitStructure);		 


	/*��������ʱ��*/
	RCC_APB2PeriphClockCmd(ECHO_RCC, ENABLE); 
	/*ECOH�����ź�*/															   
  	GPIO_InitStructure.GPIO_Pin = ECHO_PIN;	
	/*��������ģʽΪͨ���������*/
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;   
	/*������������Ϊ50MHz */   
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	/*���ÿ⺯������ʼ��PORT*/
  	GPIO_Init(ECHO_PORT, &GPIO_InitStructure);
		
#endif

}

/**
* Function       Ultrasonic_Timer2_Init

* @brief         ��ʼ����ʱ��TIM2
* @param[in]     void
* @param[out]    void
* @return        ���븡��ֵ
* @par History   ��
*/

void Ultrasonic_Timer2_Init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructer;
	NVIC_InitTypeDef NVIC_InitStructer;


	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	
	/*��ʱ��TIM2��ʼ��*/
	TIM_DeInit(TIM2);
	TIM_TimeBaseInitStructer.TIM_Period = 999;//��ʱ����Ϊ1000
	TIM_TimeBaseInitStructer.TIM_Prescaler = 71; //��Ƶϵ��72
	TIM_TimeBaseInitStructer.TIM_ClockDivision = TIM_CKD_DIV1;//����Ƶ
	TIM_TimeBaseInitStructer.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructer);
	
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);//���������ж�

	/*��ʱ���жϳ�ʼ��*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitStructer.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructer.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructer.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructer.NVIC_IRQChannelCmd = ENABLE;
	
	NVIC_Init(&NVIC_InitStructer);
	TIM_Cmd(TIM2, DISABLE);//�رն�ʱ��ʹ��

}

/* TIM2_IRQHandler 已由 balance.c (软件串口) 提供 */
__attribute__((unused)) void TIM2_IRQHandler_OLD(void) //�жϣ��������źźܳ��ǣ�����ֵ������ظ����������ж��������������
{
	if(TIM_GetITStatus(TIM2,TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);//����жϱ�־
		overcount++;	
	}
}
/**
* Function       getUltrasonicDistance
   
* @brief         ��ȡ��������
* @param[in]     void
* @param[out]    void
* @return        ���븡��ֵ
* @par History   ��
*/

float GetUltrasonicDistance(void)
{
	float length = 0, sum = 0;
	u16 tim;
	unsigned int  i = 0;

	/*��5�����ݼ���һ��ƽ��ֵ*/
	while(i != 5)
	{
		GPIO_SetBits(TRIG_PORT, TRIG_PIN);  //�����źţ���Ϊ�����ź�
		delay_us(20);  						//�ߵ�ƽ�źų���10us
		GPIO_ResetBits(TRIG_PORT, TRIG_PIN);

		/*�ȴ������ź�*/
		while(GPIO_ReadInputDataBit(ECHO_PORT, ECHO_PIN) == RESET);
		TIM_Cmd(TIM2,ENABLE);//�����źŵ�����������ʱ������
		
		i+=1; //ÿ�յ�һ�λ����ź�+1���յ�5�ξͼ����ֵ
		while(GPIO_ReadInputDataBit(ECHO_PORT, ECHO_PIN) == SET);//�����ź���ʧ
		TIM_Cmd(TIM2, DISABLE);//�رն�ʱ��
		
		tim = TIM_GetCounter(TIM2);//��ȡ��TIM2���Ĵ����еļ���ֵ��һ�߼�������ź�ʱ��
		
		length = (tim + overcount * 1000) / 58.0;//ͨ�������źż������
		
		sum = length + sum;
		TIM2->CNT = 0;  //��TIM2�����Ĵ����ļ���ֵ����
		overcount = 0;  //�ж������������
		delay_ms(1);
	}
	length = sum / 5;
	printf("CSB:%f",length );  	
	return length;		//������Ϊ��������ֵ
	
}



/**
* Function       ultrasonic_servo_mode
* @brief         ����������
* @param[in]     void
* @param[out]    void
* @retval        void
* @par History   ��
*/
void Ultrasonic_servo_mode(void)
{
	int Len = 0;
	int LeftDistance = 0, RightDistance = 0;

	Len = (u16)GetUltrasonicDistance();
	 

    if(Len <= 30)//�������ϰ���ʱ
    {

		Stop();//ͣ���������
		
		Angle_J1 = 180;		// ���
		delay_ms(500); //�ȴ������λ
		Len = GetUltrasonicDistance();			
		LeftDistance = Len;	  
	 
		Angle_J1 = 0;		// �ұ�
		delay_ms(500); //�ȴ������λ
		Len = GetUltrasonicDistance();					
		RightDistance = Len;


		Angle_J1 = 90;		//��λ
		delay_ms(500); //�ȴ������λ

		if((LeftDistance < 22 ) &&( RightDistance < 22 ))//��������������ϰ��￿�ñȽϽ�
		{
			SpinRight(7000);//��ת��ͷ
			delay_ms(600); //�ȴ������λ
		}
		else if(LeftDistance >= RightDistance)//��߱��ұ߿տ�
		{      
			SpinLeft(7000);//��ת
			delay_ms(400); //�ȴ������λ
		}
		else//�ұ߱���߿տ�
		{
			SpinRight(7000); //��ת
			delay_ms(400); //�ȴ������λ
		}
    }
		
    else if(Len > 30)//�������ϰ���ʱ
    {
		Forward(6000); 	 //���ϰ��ֱ��     
    }
}


