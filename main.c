#include "stm32f10x.h"
#include "core_cm3.h"
#include "misc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_exti.h"
#include "lcd.h"
#include "touch.h"
#include <stdbool.h>
#include <string.h>

#define BUF_SIZE 30

bool on_off;
bool phoneFlag = false;
volatile uint32_t value[] = {0, 0, 0};
char TodayWeather[];
char NowWaterLevel[];
int color[12] = {WHITE,CYAN,BLUE,RED,MAGENTA,LGRAY,GREEN,YELLOW,BROWN,BRRED,GRAY};

void GPIO_Configure(void);
void EXTI_Configure(void);
void USART1_Init(void);
void USART2_Init(void);
void RCC_Configure(void);
void ADC_Configure(void);
void DMA_Configure(void);
void NVIC_Configure(void);

void EXTI15_10_IRQHandler(void);

void Motor_Init(void);
void openDoor(void);
void closeDoor(void);

void sendPhone(char*);
void Delay(void);

void sendDataUSART1(uint16_t data);
void sendDataUSART2(uint16_t data);

void RCC_Configure(void)
{
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
   RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
}


void GPIO_Configure(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* button pin setting */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;// INPUT PULL-DOWN
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    /* LED pin setting*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //OUTPUT PUSH-PULL
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    //TX-9번(UART1)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // USART1 TX - alternate function
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //RX-10번(UART1)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // USART1 RX - PULL-DOWN
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //TX-2번(UART2)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // USART2 TX - alternate function
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //RX-3번(UART2)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // USART2 RX - PULL-DOWN
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void EXTI_Configure(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    /* Button */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource11);
    EXTI_InitStructure.EXTI_Line = EXTI_Line11;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}

void NVIC_Configure(void) {

    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // User S1 Button
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);


    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void ADC_Configure(void){

  ADC_InitTypeDef ADC_Config;
  ADC_DeInit(ADC1);
  ADC_Config.ADC_Mode = ADC_Mode_Independent;
  ADC_Config.ADC_ScanConvMode = ENABLE;
  ADC_Config.ADC_ContinuousConvMode = ENABLE;
  ADC_Config.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_Config.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_Config.ADC_NbrOfChannel = 3;
  ADC_Init(ADC1, &ADC_Config);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_11,1, ADC_SampleTime_239Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_12,2, ADC_SampleTime_239Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_13,3, ADC_SampleTime_239Cycles5);
  ADC_Cmd(ADC1, ENABLE);
  ADC_DMACmd(ADC1,ENABLE);

  ADC_ResetCalibration(ADC1);
  while (ADC_GetResetCalibrationStatus(ADC1));
  ADC_StartCalibration(ADC1);
  while (ADC_GetCalibrationStatus(ADC1));
  ADC_SoftwareStartConvCmd(ADC1,ENABLE);
}

void DMA_Configure(void){
 DMA_InitTypeDef DMA_InitStructure;
 DMA_DeInit(DMA1_Channel1);
 DMA_InitStructure.DMA_BufferSize = 3;
 DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
 DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
 DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)value;
 DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
 DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
 DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
 DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(ADC1->DR);
 DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
 DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
 DMA_InitStructure.DMA_Priority = DMA_Priority_High;
 DMA_Init(DMA1_Channel1, &DMA_InitStructure);
 DMA_Cmd(DMA1_Channel1,ENABLE);
}

void USART1_Init(void)
{
   USART_InitTypeDef USART1_InitStructure;


   // Initialize the USART1 using the structure 'USART_InitTypeDef' and the function 'USART_Init'
   USART1_InitStructure.USART_BaudRate = 9600;
   USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
   USART1_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
   USART1_InitStructure.USART_Parity = USART_Parity_No;
   USART1_InitStructure.USART_StopBits = USART_StopBits_1;
   USART1_InitStructure.USART_WordLength = USART_WordLength_8b;
   USART_Init(USART1, &USART1_InitStructure);

   // Enable the USART1 peripheral
   USART_Cmd(USART1, ENABLE);
   // Enable the USART1 RX interrupts using the function 'USART_ITConfig' and the argument value 'Receive Data register not empty interrupt'
   USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // USART RX만 Interrupt 설정
}

void USART2_Init(void)
{
   USART_InitTypeDef USART2_InitStructure;

   // Initialize the USART2 using the structure 'USART_InitTypeDef' and the function 'USART_Init'
   USART2_InitStructure.USART_BaudRate = 9600;
   USART2_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
   USART2_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
   USART2_InitStructure.USART_Parity = USART_Parity_No;
   USART2_InitStructure.USART_StopBits = USART_StopBits_1;
   USART2_InitStructure.USART_WordLength = USART_WordLength_8b;
   USART_Init(USART2, &USART2_InitStructure);

   // Enable the USART2 peripheral
   USART_Cmd(USART2, ENABLE);
   // Enable the USART2 RX interrupts using the function 'USART_ITConfig' and the argument value 'Receive Data register not empty interrupt'
   USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); // USART RX만 Interrupt 설정
}

void Motor_Init() {
   GPIO_InitTypeDef GPIO_InitStructure;
   TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
   int PrescalerValue;

    PrescalerValue = (uint16_t) (SystemCoreClock / 1000000) - 1;
    TIM_TimeBaseStructure.TIM_Period = 20000-1;
    TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;

    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Disable);
    TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Disable);
    TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Disable);
    TIM_ARRPreloadConfig(TIM4, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
} // Right Side : Port B9, Left Side : Port B7

// OC2 : PB7
// Open -> 0
// Close -> 270

// OC4 : PB9
// Open -> 90 // 2300
// Open -> 45 // 1750
// Close -> 0

void openDoor(){
    TIM_OCInitTypeDef  TIM_OCInitStructure;
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 1750;
    TIM_OC4Init(TIM4, &TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_Pulse = 700;
    TIM_OC2Init(TIM4, &TIM_OCInitStructure);
}

void closeDoor() {
    TIM_OCInitTypeDef  TIM_OCInitStructure;
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 1350;
    TIM_OC4Init(TIM4, &TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_Pulse = 1650;
    TIM_OC2Init(TIM4, &TIM_OCInitStructure);
}

void USART1_IRQHandler() {
   uint16_t word1;
    while(USART_GetITStatus(USART1,USART_IT_RXNE) ==RESET);
       // the most recent received data by the USART1 peripheral
        word1= USART_ReceiveData(USART1);

        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        sendDataUSART2(word1);
        // clear 'Read data register not empty' flag
       USART_ClearITPendingBit(USART1,USART_IT_RXNE);

}

void USART2_IRQHandler() {
   uint16_t word;
    while(USART_GetITStatus(USART2,USART_IT_RXNE)==RESET);
       // the most recent received data by the USART1 peripheral
        word = USART_ReceiveData(USART2);
        //while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        sendDataUSART1(word);
        if(word == 'o' || word == 'O') {
          phoneFlag = true;
          openDoor();
          //int i =0;
          //for (i = 0; i < 10000000; i++) {}
        }
        else if(word == 'c' || word == 'C') {
          phoneFlag = true;
          closeDoor();
          // int i =0;
          // for (i = 0; i < 10000000; i++) {}
        }
        else if(word == 'e' || word == 'E') {
          phoneFlag = false;
        }
        // clear 'Read data register not empty' flag
       USART_ClearITPendingBit(USART2,USART_IT_RXNE);
}

void EXTI15_10_IRQHandler(void) { // when the button is pressed
   if (EXTI_GetITStatus(EXTI_Line11) != RESET){
     // char end[] = ".\r\n";
     // strcat(TodayWeather, end);
     // strcat(NowWaterLevel, end);
     if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_11) == Bit_RESET) {
       // send weather
       if ((uint32_t)value[1] < 3500) {
         sendPhone("It is Rainy today.\r\n");
       }
       else if ((uint32_t)value[2] < 400) {
         sendPhone("It is Cloudy today.\r\n");
       }
       else if ((uint32_t)value[2] >= 400) {
         sendPhone("It is Sunny today.\r\n");
       }
       // send water level
       if ((uint32_t)value[0] > 2200) {
         sendPhone("The Water level is High.\r\n");
       }
       else if ((uint32_t)value[0] > 1900) {
         sendPhone("The Water level is Medium.\r\n");
       }
       else {
         sendPhone("The Water level is Low.\r\n");
       }
     }
   }
   EXTI_ClearITPendingBit(EXTI_Line11);
}

void sendPhone(char* buf) {
  char *info = buf;
  while (*info) {
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) ;
    USART_SendData(USART2, *info++);
  }
}

void Delay(void) {
   int i =0;
   for (i = 0; i < 2000000; i++) {}
}

void sendDataUSART1(uint16_t data) {
   USART_SendData(USART1, data);
}

void sendDataUSART2(uint16_t data) {
   USART_SendData(USART2, data);
}

int main() {
  SystemInit();
  RCC_Configure();
  GPIO_Configure();
  ADC_Configure();
  DMA_Configure();
  EXTI_Configure();
  NVIC_Configure();
  USART1_Init();
  USART2_Init();
  Motor_Init();
  LCD_Init();
  Touch_Configuration();
  LCD_Clear(WHITE);

  LCD_ShowString(100,20, "MON_TEAM11", BLACK, WHITE);

  while(1){
    GPIO_ResetBits(GPIOD, GPIO_Pin_2);
    GPIO_ResetBits(GPIOD, GPIO_Pin_3);
    GPIO_ResetBits(GPIOD, GPIO_Pin_4);
    GPIO_ResetBits(GPIOD, GPIO_Pin_7);

    // weather selection
    if ((uint32_t)value[1] < 3500) {
      strcpy(TodayWeather, "Rainy");  // Rainy
    }
    else if ((uint32_t)value[2] < 400) {
      strcpy(TodayWeather, "Cloudy"); // Cloudy
    }
    else if ((uint32_t)value[2] >= 400) {
      strcpy(TodayWeather, "Sunny"); // Sunny;
    }

    // water level Selection
    if ((uint32_t)value[0] > 2200) {
      strcpy(NowWaterLevel, "High"); // High
    }
    else if ((uint32_t)value[0] > 1900) {
      strcpy(NowWaterLevel, "Medium"); // Medium
    }
    else {
      strcpy(NowWaterLevel, "Low"); // Low
    }

    LCD_ShowString(100,40, "Water Level", BLACK, WHITE);
    LCD_ShowNum(100,60,(uint32_t)value[0],4,BLACK,WHITE);
    LCD_ShowString(100,80, "Rain", BLACK, WHITE);
    LCD_ShowNum(100,100,(uint32_t)value[1],4,BLACK,WHITE);
    LCD_ShowString(100,120, "Light", BLACK, WHITE);
    LCD_ShowNum(100,140,(uint32_t)value[2],4,BLACK,WHITE);
    LCD_ShowString(100,160, "Weather", BLACK, WHITE);
    if ((uint32_t)value[1] < 3500) {
      LCD_ShowString(100,180, "Rainy", BLACK, WHITE);
    }
    else if ((uint32_t)value[2] < 400) {
      LCD_ShowString(100,180, "Cloudy", BLACK, WHITE);
    }
    else if ((uint32_t)value[2] >= 400) {
      LCD_ShowString(100,180, "Sunny", BLACK, WHITE);
    }

    if(on_off == true){
      GPIO_SetBits(GPIOD, GPIO_Pin_2);
      GPIO_SetBits(GPIOD, GPIO_Pin_3);
      GPIO_SetBits(GPIOD, GPIO_Pin_4);
      GPIO_SetBits(GPIOD, GPIO_Pin_7);
      //Delay();
    }
    if(phoneFlag == false) {
      if((uint32_t)value[0] < 1800 && (uint32_t)value[1] >= 3500){ // 물부족, 비안옴
        openDoor();
      }
      else { // 물 충분 or 물 부족이지만 비오고있음
        closeDoor();
      }
    }
    Delay();
  }
}
