#include "servo.h"
#include "delay.h"

/**
* Function       Servo_GPIO_Init    
* @brief         ��Ҫ�õ��Ķ����ʼ���ӿ�
* @param[in]     void
* @param[out]    void
* @retval        void
* @par History   ��
*/

/*ʹ�ö�ʱ��1����*/
int Angle_J1 = 0;

void Servo_GPIO_Init(void)
{		
   	/*����һ��GPIO_InitTypeDef���͵Ľṹ��*/
	GPIO_InitTypeDef GPIO_InitStructure;

#ifdef USE_SERVO_J1

	/*��������ʱ��*/
//	RCC_APB2PeriphClockCmd(Servo_J1_RCC, ENABLE); 
	/*ѡ��Ҫ���Ƶ�����*/															   
		GPIO_InitStructure.GPIO_Pin = Servo_J1_PIN;	
	/*��������ģʽΪͨ���������*/
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   
	/*������������Ϊ50MHz */   
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	/*���ÿ⺯������ʼ��PORT*/
//		GPIO_Init(Servo_J1_PORT, &GPIO_InitStructure);		  		  
#endif
	
}

/**
* Function       Servo_J1  
* @brief         ���1���ƺ���
* @param[in]     v_iAngle �Ƕȣ�0~180��
* @param[out]    void
* @retval        void
* @par History   ��
*/
void Servo_J1(int v_iAngle)/*����һ�����庯��������ģ�ⷽʽ����PWMֵ*/
{
//	int pulsewidth;    						//������������

//	pulsewidth = (v_iAngle * 11) + 500;			//���Ƕ�ת��Ϊ500-2480 ������ֵ

//	GPIO_SetBits(Servo_J1_PORT, Servo_J1_PIN );		//������ӿڵ�ƽ�ø�
//	delay_us(pulsewidth);					//��ʱ����ֵ��΢����

	//GPIO_ResetBits(Servo_J1_PORT, Servo_J1_PIN );	//������ӿڵ�ƽ�õ�
//	delay_ms(20 - pulsewidth/1000);			//��ʱ������ʣ��ʱ��
}

/**
* Function       front_detection  
* @brief         ��̨�����ǰ
* @param[in]     void
* @param[out]    void
* @retval        void
* @par History   ��
*/
void front_detection()
{
	int i = 0;
  	//�˴�ѭ���������٣�Ϊ������С�������ϰ���ķ�Ӧ�ٶ�
  	for(i=0; i <= 15; i++) 						//����PWM��������Ч��ʱ�Ա�֤��ת����Ӧ�Ƕ�
  	{
    	Servo_J1(90);						//ģ�����PWM
  	}
}

/**
* Function       left_detection 
* @brief         ��̨�������
* @param[in]     void
* @param[out]    void
* @retval        void
* @par History   ��
*/
void left_detection()
{
	int i = 0;
	for(i = 0; i <= 15; i++) 						//����PWM��������Ч��ʱ�Ա�֤��ת����Ӧ�Ƕ�
	{
		Servo_J1(175);					//ģ�����PWM
	}
}

/**
* Function       right_detection
* @brief         ��̨�������
* @param[in]     void
* @param[out]    void
* @retval        void
* @par History   ��
*/
void right_detection()
{
	int i = 0;
	for(i = 0; i <= 15; i++) 						//����PWM��������Ч��ʱ�Ա�֤��ת����Ӧ�Ƕ�
	{
		Servo_J1(5);						//ģ�����PWM
	}
}

/**
* Function       TIM1_Int_Init  
* @brief         ��ʱ��1��ʼ���ӿ�
* @param[in]     arr���Զ���װֵ��psc��ʱ��Ԥ��Ƶ��
* @param[out]    void
* @retval        void
* @par History   ����ʱ��ѡ��ΪAPB1��2������APB1Ϊ36M
*/
void TIM1_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE); //ʱ��ʹ��
	
	//��ʱ��TIM1��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler = (psc-1); //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim   //36Mhz
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;    //�ظ������ر�
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE ); //ʹ��ָ����TIM1�ж�,���������ж�

	//�ж����ȼ�NVIC����
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;  //TIM1�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //��ռ���ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //�����ȼ�3��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);  //��ʼ��NVIC�Ĵ���

	TIM_Cmd(TIM1, ENABLE);  //ʹ��TIMx					 
}
/**
* Function       TIM1_Int_Init   
* @brief         ��ʱ��1�жϷ������: ��Ҫ����6·�������
* @param[in]     arr���Զ���װֵ��psc��ʱ��Ԥ��Ƶ��
* @param[out]    void
* @retval        void
* @par History   ����ʱ��ѡ��ΪAPB1��2������APB1Ϊ36M
*/
int num = 0;


__attribute__((unused)) void TIM1_UP_IRQHandler_OLD(void)   //TIM1�ж�
{
	if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)  //���TIM1�����жϷ������
	{
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);  //���TIMx�����жϱ�־ 
		num++;
		
		if(num <= (Angle_J1 * 11 + 500)/10)
		{
			GPIO_SetBits(Servo_J1_PORT, Servo_J1_PIN );		//������ӿڵ�ƽ�ø�
		}
		else
		{
			GPIO_ResetBits(Servo_J1_PORT, Servo_J1_PIN );		//������ӿڵ�ƽ�ø�
		}

		if(num == 2000)
		{
			num = 0;
		}
		
	}
}

