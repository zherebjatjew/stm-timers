/*********************************************************
 * Handle TIM2 interrupt
 *********************************************************/

#include "stm32l1xx.h"   // low power mcu
#include "discover_board.h"
#include "stm32l1xx_tim.h"
 
void GPIO_Configuration(void);
void NVIC_Configuration(void);
void RCC_Configuration(void);
void TIM2_Configuration(void);
void SysTick_Configuration(void);
 
int main(void)
{
    RCC_Configuration();
 
    NVIC_Configuration();
 
    GPIO_Configuration();
 
    GPIO_HIGH(GPIOB,GPIO_Pin_6); // PB.06 BLUE
    GPIO_HIGH(GPIOB,GPIO_Pin_7); // PB.07 GREEN
 
    SysTick_Configuration();
 
    TIM2_Configuration();
 
    while(1);   // Infinite loop, main() must not exit
}
 
void RCC_Configuration(void)
{
  /* Enable the GPIOs Clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
 
  /* Enable APB1 clocks */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_PWR,ENABLE);
 
  /* Enable SYSCFG */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
 
  /* Allow access to the RTC */
  PWR_RTCAccessCmd(ENABLE);
 
  /* Reset Backup Domain */
  RCC_RTCResetCmd(ENABLE);
  RCC_RTCResetCmd(DISABLE);
 
  /*!< LSE Enable */
  RCC_LSEConfig(RCC_LSE_ON);
 
  /*!< Wait till LSE is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {}
 
  /*!< LCD Clock Source Selection */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
}
 
void NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
 
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
 
  /* Set SysTick Preemption Priority, it's a system handler rather than a regular interrupt */
  NVIC_SetPriority(SysTick_IRQn, 0x04);
}
 
void SysTick_Configuration()
{
    RCC_ClocksTypeDef RCC_Clocks;
 
  RCC_GetClocksFreq(&RCC_Clocks);
 
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 1);  // 1 Hz
}
 
 
void TIM2_Configuration(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
 
  TIM_DeInit(TIM2);
 
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
 
  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Prescaler = 16000 - 1; // 16 MHz / 16000 = 1 KHz
  TIM_TimeBaseStructure.TIM_Period = 1000 - 1;  // 1 KHz / 1000 = 1 Hz;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
 
  /* Enable TIM2 Update Interrupt */
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
 
  /* Enable TIM2 */
  TIM_Cmd(TIM2,ENABLE);
}
 
void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
 
  // PB.06 BLUE LED, PB.07 GREEN LED
 
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}
 
// These would normally exist in stm32l1xx_it.c, or equivalent
 
void SysTick_Handler(void)
{
    static int SYSTICK_Tick = 0;
 
  SYSTICK_Tick++;
 
  if (SYSTICK_Tick >= 60) // 1 Minute
    SYSTICK_Tick = 0;
 
  if (SYSTICK_Tick % 2) // Toggle PB.07 GREEN LED
  {
    GPIO_LOW(GPIOB,GPIO_Pin_7);
  }
  else
  {
    GPIO_HIGH(GPIOB,GPIO_Pin_7);
  }
}
 
void TIM2_IRQHandler(void)
{
    static int TIM2_Tick = 0;
 
  if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
  {
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
 
        TIM2_Tick++;
 
        if (TIM2_Tick >= 60) // 1 Second
         TIM2_Tick = 0;
 
        if (TIM2_Tick % 2) // Toggle PB.06 BLUE LED
        {
            GPIO_LOW(GPIOB,GPIO_Pin_6);
        }
        else
        {
            GPIO_HIGH(GPIOB,GPIO_Pin_6);
        }
    }
}