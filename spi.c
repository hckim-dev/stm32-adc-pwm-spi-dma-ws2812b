#include "device_driver.h"
#include "spi.h"

#define SPI1_CS_HIGH() (Macro_Set_Bit(GPIOA->ODR, 8U))
#define SPI1_CS_LOW() (Macro_Clear_Bit(GPIOA->ODR, 8U))

void SPI1_Init(unsigned int div)
{
	volatile int i;

	int n = (__builtin_ctz(div) & 0x7) - 1;
	if (n < 0)
		n = 0;

	Macro_Set_Bit(RCC->AHB1ENR, 0U);  // GPIOA
	Macro_Set_Bit(RCC->AHB1ENR, 1U);  // GPIOB
	Macro_Set_Bit(RCC->APB2ENR, 12U); // SPI1

	Macro_Clear_Bit(RCC->APB2RSTR, 12U);
	Macro_Set_Bit(RCC->APB2RSTR, 12U);
	for (i = 0; i < 1000; i++)
		;
	Macro_Clear_Bit(RCC->APB2RSTR, 12U);

	// PA8(nCS) => Output Push-Pull
	Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 16U);
	Macro_Clear_Bit(GPIOA->OTYPER, 8U);
	SPI1_CS_HIGH();

	// PB3(SCK), PB4(MISO), PB5(MOSI) => AF05
	Macro_Write_Block(GPIOB->MODER, 0x3F, 0x2A, 6U);
	Macro_Write_Block(GPIOB->AFR[0], 0xFFF, 0x555, 12U);
	Macro_Write_Block(GPIOB->OTYPER, 0x7, 0x0, 3U);
	Macro_Write_Block(GPIOB->OSPEEDR, 0x3F, 0x2A, 6U);

	SPI1->CR2 = (0U << 4U) | (1U << 2U);

	// SSM(Bit 9)=1, SSI(Bit 8)=1 추가로 Master mode 다운 현상 방지
	SPI1->CR1 = (1U << 11U) | (1U << 9U) | (1U << 8U) | ((unsigned int)n << 3U) | (1U << 2U);
	Macro_Set_Bit(SPI1->CR1, 6U); // SPE
}

void SPI1_Write_Reg(unsigned int addr, unsigned int data)
{
	SPI1_CS_HIGH();
	SPI1_CS_LOW();

	SPI1->DR = (0U << 15U) | ((addr & 0x0FU) << 11U) | (data & 0xFFU);
	while (Macro_Check_Bit_Clear(SPI1->SR, 1U))
		; // TXE
	while (Macro_Check_Bit_Set(SPI1->SR, 7U))
		; // BSY

	SPI1_CS_HIGH();
}

void SPI1_Config_GPIO(unsigned int config)
{
	// SC16IS752 칩은 0xFF 가 8개 핀 전체 Output 모드입니다.
	SPI1_Write_Reg(SC16IS752_IODIR, config);
}

void SPI1_Write_GPIO(unsigned int data)
{
	SPI1_Write_Reg(SC16IS752_IOSTATE, data);
}
