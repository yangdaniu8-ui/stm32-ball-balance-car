#include "balance.h"
#define RX_RING_SIZE 128u
typedef struct { char ch; uint32_t timestamp_ms; } RxByte;
static RxByte rx_ring[RX_RING_SIZE];
static volatile uint16_t rx_head, rx_tail;
static volatile uint32_t rx_errors;
static TaskHandle_t rx_task;
volatile uint32_t g_UART_RxCount;
int g_ServoCenterPWM = APP_SERVO_CENTER_US;
static void uart_init(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef uart;
    NVIC_InitTypeDef nvic;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO |
                           RCC_APB2Periph_USART1, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_6;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &gpio);
    USART_StructInit(&uart);
    uart.USART_BaudRate = 115200;
    uart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &uart);
    USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
    nvic.NVIC_IRQChannel = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 6;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    USART_Cmd(USART1, ENABLE);
}
static void servo_init(void)
{
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_8;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);
    TIM1->CR1 = 0;
    TIM1->PSC = 72 - 1;
    TIM1->ARR = 20000 - 1;
    TIM1->CCR1 = APP_SERVO_CENTER_US;
    TIM1->CCMR1 = (6u << 4) | TIM_CCMR1_OC1PE;
    TIM1->CCER = TIM_CCER_CC1E;
    TIM1->BDTR = TIM_BDTR_MOE;
    TIM1->EGR = TIM_EGR_UG;
    TIM1->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
}
void Balance_Init(void)
{
    servo_init();
    uart_init();
    Balance_SetServo(0.0f);
}
void Balance_StartRx(TaskHandle_t receiver)
{
    volatile uint32_t discard;
    rx_task = receiver;
    discard = USART1->SR;
    discard = USART1->DR;
    (void)discard;
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}
void USART1_IRQHandler(void)
{
    uint32_t status = USART1->SR;
    uint16_t next;
    char ch;
    BaseType_t wake = pdFALSE;
    if (status & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE)) {
        ch = (char)USART1->DR;
        if (status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE)) rx_errors++;
        else if (status & USART_SR_RXNE) {
            g_UART_RxCount++;
            next = (uint16_t)((rx_head + 1u) % RX_RING_SIZE);
            if (next == rx_tail) rx_errors++;
            else {
                rx_ring[rx_head].ch = ch;
                rx_ring[rx_head].timestamp_ms = (uint32_t)xTaskGetTickCountFromISR();
                __DMB();
                rx_head = next;
            }
        }
        if (rx_task != NULL) vTaskNotifyGiveFromISR(rx_task, &wake);
    }
    portYIELD_FROM_ISR(wake);
}
int Balance_ReadRx(char *ch, uint32_t *timestamp_ms)
{
    uint16_t tail = rx_tail;
    if (tail == rx_head) return 0;
    __DMB();
    *ch = rx_ring[tail].ch;
    *timestamp_ms = rx_ring[tail].timestamp_ms;
    __DMB();
    rx_tail = (uint16_t)((tail + 1u) % RX_RING_SIZE);
    return 1;
}
uint32_t Balance_RxErrors(void) { return rx_errors; }
void Balance_SetRawPWM(u16 pwm)
{
    if (pwm < APP_SERVO_MIN_US) pwm = APP_SERVO_MIN_US;
    if (pwm > APP_SERVO_MAX_US) pwm = APP_SERVO_MAX_US;
    TIM1->CCR1 = pwm;
}
void Balance_SetServo(float angle)
{
    float pwm;
    if (angle > APP_SERVO_LIMIT_DEG) angle = APP_SERVO_LIMIT_DEG;
    if (angle < -APP_SERVO_LIMIT_DEG) angle = -APP_SERVO_LIMIT_DEG;
    pwm = g_ServoCenterPWM + angle * (APP_SERVO_MAX_US - APP_SERVO_MIN_US) / 180.0f;
    Balance_SetRawPWM((u16)pwm);
}
