#include "device_driver.h"
#include "ws2812b.h"
#include <stdio.h>

void _Invalid_ISR(void)
{
	unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
	printf("\nInvalid_Exception: %d!\n", r);
	printf("Invalid_ISR: %d!\n", r - 16);
	for (;;)
		;
}

// 인터럽트에서 1.25us마다 순차적으로 출력할 1D 전송 버퍼 (RET 40개 + 96비트 = 136개)
extern unsigned short ccr_buffer[TOTAL_BITS];

extern volatile unsigned short tx_idx;
extern volatile unsigned char is_busy;

void TIM2_IRQHandler(void)
{
	if (Macro_Check_Bit_Set(TIM2->SR, 0U))
	{
		Macro_Clear_Bit(TIM2->SR, 0U); // UIF 플래그 클리어

		if (tx_idx < TOTAL_BITS)
		{
			// [PA0 / TIM2_CH1] 레지스터인 CCR1 에 다음 파형 대입
			TIM2->CCR1 = ccr_buffer[tx_idx++];
		}
		else
		{
			// 136개 비트 전송 완료 시 인터럽트 비활성화
			Macro_Clear_Bit(TIM2->DIER, 0U); // UIE = 0
			TIM2->CCR1 = WS2812_DUTY_RET;	 // Output Low 유지
			is_busy = 0;
		}
	}
}

void TIM3_IRQHandler(void)
{
	if (Macro_Check_Bit_Set(TIM3->SR, 0U))
	{
		Macro_Clear_Bit(TIM3->SR, 0U);

		if (tx_idx < TOTAL_BITS)
		{
			TIM3->CCR2 = ccr_buffer[tx_idx++]; // TIM3->CCR2 로 전송!
		}
		else
		{
			Macro_Clear_Bit(TIM3->DIER, 0U); // UIE = 0 (인터럽트 종료)
			TIM3->CCR2 = WS2812_DUTY_RET;
			is_busy = 0;
		}
	}
}

extern volatile unsigned long Sys_Tick;

// TIM4 ISR
void TIM4_IRQHandler(void)
{
	Macro_Clear_Bit(TIM4->SR, 0U);
	NVIC_ClearPendingIRQ(TIM4_IRQn);

	Sys_Tick++; // 1ms 시스템 틱 증가
}

volatile int DMA2_STREAM0_DONE = 0;

void DMA2_Stream0_IRQHandler(void)
{
	DMA2->LIFCR = 0x3F << 0;
	NVIC_ClearPendingIRQ(DMA2_Stream0_IRQn);
	DMA2_STREAM0_DONE = 1;
}

volatile int DMA1_STREAM6_DONE = 0;

void DMA1_Stream6_IRQHandler(void)
{
	DMA1->HIFCR = 0x3F << 16;
	NVIC_ClearPendingIRQ(DMA1_Stream6_IRQn);
	DMA1_STREAM6_DONE = 1;
}

volatile unsigned char DMA1_STREAM5_DONE = 1;

void DMA1_Stream5_IRQHandler(void)
{
	DMA1->HIFCR = (0x3FU << 6U);
	Macro_Clear_Bit(TIM2->DIER, 9U);
	DMA1_STREAM5_DONE = 1;
}
