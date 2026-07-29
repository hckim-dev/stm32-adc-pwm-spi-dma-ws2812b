#include "device_driver.h"
#include "adc.h"

void ADC1_Init(void)
{
	// GPIOA 클럭 활성화 (AHB1ENR: Bit 0)
	Macro_Set_Bit(RCC->AHB1ENR, 0U);

	// PA6 핀 아날로그 모드 설정 (MODER: 6번 핀 -> offset 12, mode 0x3)
	Macro_Write_Block(GPIOA->MODER, 0x3, 0x3, 12U);

	// ADC1 클럭 활성화 (APB2ENR: Bit 8)
	Macro_Set_Bit(RCC->APB2ENR, 8U);

	// CH6 샘플링 타임 설정: 480 Cycles (SMPR2: 6번 채널 -> offset 18, value 0x7)
	Macro_Write_Block(ADC1->SMPR2, 0x7, 0x7, 18U);

	// 변환 시퀀스 길이: 1개 (SQR1: offset 20, value 0x0)
	Macro_Write_Block(ADC1->SQR1, 0xF, 0x0, 20U);

	// 첫 번째 순서(SQ1)로 CH6 지정 (SQR3: offset 0, value 6)
	Macro_Write_Block(ADC1->SQR3, 0x1F, 6U, 0U);

	// ADC Prescaler 설정: PCLK2 / 6 = 16MHz (ADC->CCR: offset 16, value 0x2)
	Macro_Write_Block(ADC->CCR, 0x3, 0x2, 16U);

	// ADC1 전원 켜기 (CR2: ADON Bit 0)
	Macro_Set_Bit(ADC1->CR2, 0U);
}

unsigned short ADC1_Read(void)
{
	// 1) 변환 시작 (CR2: SWSTART Bit 30)
	Macro_Set_Bit(ADC1->CR2, 30U);

	// 2) 변환 완료 대기 (SR: EOC Bit 1이 1이 될 때까지)
	while (!Macro_Check_Bit_Set(ADC1->SR, 1U))
		;

	// 3) EOC 플래그 클리어
	Macro_Clear_Bit(ADC1->SR, 1U);

	// 4) 12비트 변환 결과 읽기 (0 ~ 4095)
	return (unsigned short)(ADC1->DR & 0xFFFU);
}
