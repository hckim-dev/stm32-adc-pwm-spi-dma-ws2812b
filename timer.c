#include "device_driver.h"
#include "timer.h"
#include "ws2812b.h"

volatile unsigned long Sys_Tick = 0; // 시스템 틱 카운터

// 타이머 모듈 전체 초기화
void Timer_Init(void)
{
	TIM2_Init();
	TIM3_Init();
	TIM4_Init();
	TIM5_Init();
}

// TIM2 초기화 (롱클릭 타임아웃용, 단발성)
void TIM2_Init(void)
{
	Macro_Set_Bit(RCC->APB1ENR, 0U);	 // TIM2 클럭 활성화
	TIM2->CR1 = (1U << 4U) | (1U << 3U); // Down-counter, One-Pulse Mode
	TIM2->PSC = (unsigned int)(TIMXCLK / (double)TIM_FREQ + 0.5) - 1U;
}

// TIM2 원샷 타이머 시작 (ms)
void TIM2_Start(int delay_ms)
{
	TIM2->ARR = (unsigned int)(TIM_1MS_PLS * delay_ms) - 1U;
	Macro_Set_Bit(TIM2->EGR, 0U);  // 레지스터 동기화
	Macro_Clear_Bit(TIM2->SR, 0U); // 펜딩 플래그 클리어

	NVIC_ClearPendingIRQ(TIM2_IRQn);
	Macro_Set_Bit(TIM2->DIER, 0U); // 인터럽트 허용
	NVIC_EnableIRQ(TIM2_IRQn);

	Macro_Set_Bit(TIM2->CR1, 0U); // 타이머 시작
}

// TIM2 정지 및 인터럽트 차단
void TIM2_Stop(void)
{
	Macro_Clear_Bit(TIM2->CR1, 0U);
	Macro_Clear_Bit(TIM2->DIER, 0U);
	NVIC_DisableIRQ(TIM2_IRQn);
}

// TIM3 초기화 (PA7 / TIM3_CH2 - WS2812B 전용)
void TIM3_Init(void)
{
	// 1) 클럭 활성화 (GPIOA: Bit 0, TIM3: Bit 1)
	Macro_Set_Bit(RCC->AHB1ENR, 0U); // GPIOA
	Macro_Set_Bit(RCC->APB1ENR, 1U); // TIM3 (Bit 1)

	// 2) PA7 핀 설정 (AF 모드 = 0x2, Very High Speed = 0x3)
	Macro_Write_Block(GPIOA->MODER, 0x3U, 0x2U, 7U * 2U);	// PA7 AF 모드
	Macro_Write_Block(GPIOA->OSPEEDR, 0x3U, 0x3U, 7U * 2U); // PA7 Very High Speed

	// PA7 AF02 (TIM3_CH2) 매핑 (AFR[0] 28번 비트 위치에 0x2 대입)
	Macro_Write_Block(GPIOA->AFR[0], 0xFU, 0x2U, 7U * 4U);

	// 3) Down-counter, Repeat Mode
	TIM3->CR1 = (1U << 4U) | (0U << 3U);

	// 4) 레지스터 설정 (96MHz 기준, 1.25us 주기 / 800kHz)
	TIM3->PSC = 0U;
	TIM3->ARR = WS2812_ARR;		  // 120U
	TIM3->CCR2 = WS2812_DUTY_RET; // CH2 전용 CCR2 레지스터 사용 (초기 120U)

	// 5) CCMR1: CH2 PWM Mode 1 (0x6000) & Preload Enable (0x0800) -> 0x6800
	Macro_Write_Block(TIM3->CCMR1, 0xFF00U, 0x6800U, 0U);

	// 6) CCER: CH2 출력 활성화 (CC2E: Bit 4) & Active Low (CC2P: Bit 5)
	Macro_Set_Bit(TIM3->CCER, 4U); // CC2E = 1 (CH2 출력 Enable)
	Macro_Set_Bit(TIM3->CCER, 5U); // CC2P = 1 (CH2 Active Low)

	// 7) 레지스터 강제 반영 및 찌꺼기 플래그 청소
	Macro_Set_Bit(TIM3->EGR, 0U);
	Macro_Clear_Bit(TIM3->SR, 0U);

	// 8) NVIC 설정 (TIM3 인터럽트 허용)
	NVIC_ClearPendingIRQ(TIM3_IRQn);
	NVIC_EnableIRQ(TIM3_IRQn);

	// 9) 타이머 동작 시작 (CEN: Bit 0)
	Macro_Set_Bit(TIM3->CR1, 0U);
}

// TIM4 초기화 (1ms 주기 시스템 틱)
void TIM4_Init(void)
{
	Macro_Set_Bit(RCC->APB1ENR, 2U);	 // TIM4 클럭 활성화
	TIM4->CR1 = (1U << 4U) | (0U << 3U); // Down-counter, Repeat Mode
	TIM4->PSC = (unsigned int)(TIMXCLK / (double)TIM_FREQ + 0.5) - 1U;
	TIM4->ARR = (unsigned int)(TIM_1MS_PLS * 1) - 1U; // 1ms 주기 설정

	Macro_Set_Bit(TIM4->EGR, 0U);
	Macro_Clear_Bit(TIM4->SR, 0U);

	NVIC_ClearPendingIRQ(TIM4_IRQn);
	Macro_Set_Bit(TIM4->DIER, 0U); // 인터럽트 허용
	NVIC_EnableIRQ(TIM4_IRQn);

	Macro_Set_Bit(TIM4->CR1, 0U); // 타이머 시작
}

// TIM5 초기화 (모터 PWM 제어용)
void TIM5_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 0U); // GPIOA 클럭 활성화
	Macro_Set_Bit(RCC->APB1ENR, 3U); // TIM5 클럭 활성화

	// PA0, PA1 핀 설정 (AF 모드 / AF02: TIM5)
	Macro_Write_Block(GPIOA->MODER, 0xFU, 0xAU, 0U);
	Macro_Write_Block(GPIOA->AFR[0], 0xFFU, 0x22U, 0U);

	TIM5->CR1 = (1U << 4U) | (0U << 3U); // Down-counter, Repeat Mode

	// PWM 주파수 설정
	TIM5->PSC = (unsigned int)(TIMXCLK / TIM5_CNT_FREQ) - 1U;
	TIM5->ARR = (unsigned int)(TIM5_CNT_FREQ / TIM5_PWM_FREQ) - 1U;

	// CH1, CH2 PWM Mode 1 및 Preload 활성화
	Macro_Write_Block(TIM5->CCMR1, 0xFFFFU, 0x6868U, 0U);

	// CH1, CH2 출력 활성화
	Macro_Set_Bit(TIM5->CCER, 0U);
	Macro_Set_Bit(TIM5->CCER, 4U);

	// 동기화 및 타이머 시작
	Macro_Set_Bit(TIM5->EGR, 0U);
	Macro_Set_Bit(TIM5->CR1, 0U);
}

// TIM5 CH1, CH2 PWM Duty 비 설정 (0~100%)
void TIM5_Set_Duty(int ch1_duty, int ch2_duty)
{
	TIM5->CCR1 = (unsigned int)(TIM5->ARR * (ch1_duty / 100.0));
	TIM5->CCR2 = (unsigned int)(TIM5->ARR * (ch2_duty / 100.0));
}
