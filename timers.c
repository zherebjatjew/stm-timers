#include "timers.h"
#include <stm32f4xx.h>
#include <stm32f4xx_tim.h>
#include <stm32f4xx_rcc.h>
//#include <misc.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////
typedef struct Schedule Schedule;
struct Schedule {
	DeferredFunction foo;
	int remained;
	int period;
	long nextTime;
	void* data;
	Schedule* next;
};

Schedule* vector = NULL;


//////////////////////////////////////////////////////////////////
/*
https://my.st.com/public/STe2ecommunities/mcu/Lists/STM32Discovery/Flat.aspx?RootFolder=https%3a%2f%2fmy%2est%2ecom%2fpublic%2fSTe2ecommunities%2fmcu%2fLists%2fSTM32Discovery%2fSTM32W%20Timer%20interrupt%20after%20x%20milliseconds&FolderCTID=0x01200200770978C69A1141439FE559EB459D75800084C20D8867EAD444A5987D47BE638E0F&currentviews=995

Since the release of SimpleMAC 2.0.0 RC1, I've decided to update this post.
MB851 rev.A seems not to work correctly with this new firmware. They just don't reply to UART commands (no rx but they do tx).
Anyway, since this 2.0.0 included standard libraries templates for handling the hardware + the usual HAL thing for the wireless related modules, standard timebase TIM initialization goes like this:

  TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
  NVIC_InitTypeDef Init;
  TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInitStruct.TIM_Prescaler = 5;
  TIM_TimeBaseInitStruct.TIM_Period = 375;
  TIM_TimeBaseInit(TIM1,&TIM_TimeBaseInitStruct);
  TIM_PrescalerConfig(TIM1, 5, TIM_PSCReloadMode_Immediate);
  TIM_ITConfig(TIM1_IT, TIM_IT_Update, ENABLE);
  TIM_UpdateRequestConfig(TIM1, TIM_UpdateSource_Global);
  TIM_UpdateDisableConfig(TIM1, DISABLE);
  TIM_Cmd(TIM1, ENABLE);
  Init.NVIC_IRQChannel = TIM1_IRQn;
  Init.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&Init);

This example sets TIM1 to standard priority, prescaler set @ 32 (2^5) and ARR (Period) @ 375. That means the TIM1 generates an update event every 1ms.

Best,
Simone
*/

volatile long gTickCount = 0;

long getTickCount() {
	return gTickCount;
}

void RCC_Configuration(void)
{
	/* Enable the GPIOs Clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	/* Enable APB1 clocks */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_PWR,ENABLE);
	/* Enable SYSCFG */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	/* Allow access to the RTC */
//	PWR_RTCAccessCmd(ENABLE);
	/* Reset Backup Domain */
//	RCC_RTCResetCmd(ENABLE);
//	RCC_RTCResetCmd(DISABLE);
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
	TIM_TimeBaseStructure.TIM_Prescaler = 1600 - 1; // 16 MHz / 1600 = 10 KHz
	TIM_TimeBaseStructure.TIM_Period = 10 - 1;  // 10 KHz / 10 = 1 KHz;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	/* Enable TIM2 Update Interrupt */
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	/* Enable TIM2 */
	TIM_Cmd(TIM2,ENABLE);
}

void timers_init() {
	// Set up timer
    RCC_Configuration();
    NVIC_Configuration();
//    SysTick_Configuration();
    TIM2_Configuration();
}

void timers_close() {
	// Stop timer

	// Free vector
	Schedule* v = vector;
	vector = NULL;
	while (v != NULL) {
		Schedule* next = v->next;
		free(v);
		v = next;
	}
	vector = NULL;
}

handle timers_schedule(DeferredFunction foo, int period_ms, void* data, int repeat) {
	if (foo == NULL || period_ms == 0 || repeat < TIMER_RESOLUTION_MS) {
		return NULL;
	}
	Schedule* created = malloc(sizeof(Schedule));
	created->next = vector;
	created->foo = foo;
	created->remained = repeat;
	created->period = period_ms;
	created->nextTime = getTickCount() + period_ms;
	created->data = data;
	vector = created;
	return (handle)vector;
}

char timers_cancel(handle schedule) {
	Schedule* v = vector;
	Schedule* parent = NULL;
	while (v != NULL) {
		if (v == (Schedule*)schedule) {
			if (parent == NULL) {
				vector = v->next;
			} else {
				parent->next = v->next;
			}
			free(v);
			return 1;
		} else {
			parent = v;
			v = v->next;
		}
	}
	return 0;
}

void onTimer() {
	Schedule* v = vector;
	Schedule* parent = NULL;
	while (v != NULL) {
		if (v->nextTime <= getTickCount()) {
			v->remained--;
			if (v->remained >= 0) {
				v->foo(v->data, &v->remained);
			}
			if (v->remained <= 0) {
				Schedule* cur = v->next;
				if (parent == NULL) {
					vector = v->next;
				} else {
					parent->next = v->next;
				}
				free(v);
				v = cur;
				continue;
			}
			v->nextTime += v->period;
		}
		parent = v;
		v = v->next;
	}
}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

		gTickCount++;
		onTimer();
	}
}
