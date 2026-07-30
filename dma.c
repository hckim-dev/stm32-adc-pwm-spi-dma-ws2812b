#include "device_driver.h"

void DMA2_Stream0_M2M_Start(void *src_addr, void *dst_addr, int num)
{
	// DMA2 Clock On
	Macro_Set_Bit(RCC->AHB1ENR, 22);

	// CHSEL [0]: 채널 선택 0
	// MSIZE [16b]: Mem측 1 Unit Size (16b)
	// PSIZE [16b]: Peri측 1 Unit Size (16b)
	// MINC [1]: Mem 주소 증가 모드
	// PINC [1]: Peri 주소 증가 모드
	// DIR [2]: M2M
	// EN [0]: DMA Disable
	DMA2_Stream0->CR = (0x0 << 25) | (0x2 << 13) | (0x2 << 11) | (0x1 << 10) | (0x1 << 9) | (0x2 << 6) | (0x0 << 0);
	// DMDIS [Disable]: Direct Mode Disable (M2M인 경우 FIFO를 거쳐야 함)
	// FTH [4/4]: FIFO Threshold Selection (경계값만큼 쌓이면 방출)
	DMA2_Stream0->FCR = (0x1 << 2) | (0x3 << 0);
	// M2M인 경우 Source가 Peri측이 됨
	DMA2_Stream0->PAR = (unsigned int)src_addr;
	DMA2_Stream0->M0AR = (unsigned int)dst_addr;
	DMA2_Stream0->NDTR = num;

	DMA2->LIFCR = 0x3F << 0;			// Pending Clear
	Macro_Set_Bit(DMA2_Stream0->CR, 4); // TCIE
	NVIC_ClearPendingIRQ(DMA2_Stream0_IRQn);
	NVIC_EnableIRQ(DMA2_Stream0_IRQn);

	Macro_Set_Bit(DMA2_Stream0->CR, 0); // DMA Enable
}

void DMA1_Stream6_USART2_TX_Start(void *src_addr, int num)
{
	// DMA1 Clock On (AHB1 버스의 Bit 21 활성화)
	Macro_Set_Bit(RCC->AHB1ENR, 21);

	// CHSEL [4]   : 채널 선택 4 (USART2_TX 전용 채널 지정)
	// MSIZE [8b]  : Mem측 1 Unit Size (8bit / 1 Byte)
	// PSIZE [8b]  : Peri측 1 Unit Size (8bit / 1 Byte)
	// MINC [1]    : Mem 주소 증가 모드 (문자열 배열 순회)
	// PINC [0]    : Peri 주소 고정 모드 (&USART2->DR 레지스터 주소 고정)
	// DIR [1]     : Memory-to-Peripheral (M2P: RAM -> USART2 TX)
	// EN [0]      : DMA Disable (설정 중 안전을 위해 비활성화)
	DMA1_Stream6->CR = (0x4 << 25) | (0x0 << 13) | (0x0 << 11) | (0x1 << 10) | (0x0 << 9) | (0x1 << 6) | (0x0 << 0);

	// DMDIS [Enable] : Direct Mode Enable (0 = FIFO 사용 안 함, M2P 직송 모드)
	DMA1_Stream6->FCR = 0;

	// PAR  : Destination(목적지) 주변장치 주소 (&USART2->DR Data Register)
	// M0AR : Source(출발지) 메모리 주소 (전송할 문자열 버퍼 주소)
	// NDTR : 전송할 데이터 개수 (문자열 길이/바이트 수)
	DMA1_Stream6->PAR = (unsigned int)&USART2->DR;
	DMA1_Stream6->M0AR = (unsigned int)src_addr;
	DMA1_Stream6->NDTR = num;

	// Stream 6번 찌꺼기 상태/인터럽트 플래그 초기화 (HIFCR의 bit 16~21 범위 clear)
	DMA1->HIFCR = 0x3F << 16; // (참고: Stream 6번용 offset은 bit 16부터 시작합니다)

	// TCIE [1] : Transfer Complete Interrupt Enable (전송 완료 인터럽트 허용)
	Macro_Set_Bit(DMA1_Stream6->CR, 4);

	// NVIC 설정 (17번: DMA1_Stream6 전용 IRQ)
	NVIC_ClearPendingIRQ(DMA1_Stream6_IRQn);
	NVIC_EnableIRQ(DMA1_Stream6_IRQn);

	// DMA Enable (USART2 TX DMA 전송 개시!)
	Macro_Set_Bit(DMA1_Stream6->CR, 0);
}

extern volatile unsigned char DMA1_STREAM5_DONE;

void DMA1_Stream5_Init(void)
{
	// DMA1 클럭 활성화 (AHB1ENR: Bit 21)
	Macro_Set_Bit(RCC->AHB1ENR, 21U);

	// DMA Stream 5 제어 레지스터(CR) 설정
	// - CHSEL: Channel 3 선택 (3U << 25U)
	// - MSIZE: 16-bit (1U << 13U)
	// - PSIZE: 16-bit (1U << 11U)
	// - MINC: 메모리 주소 증가 (1U << 10U)
	// - DIR: 메모리 -> 외장장치 (1U << 6U)
	// - TCIE: 전송 완료 인터럽트 허용 (Bit 4 = 1U << 4U)
	// - ENABLE: Disable (0U << 0U)
	DMA1_Stream5->CR = (3U << 25U) | (1U << 13U) | (1U << 11U) | (1U << 10U) | (1U << 6U) | (1U << 4U) | (0U << 0U);

	// DMDIS [Enable] : Direct Mode Enable (0 = FIFO 사용 안 함, M2P 직송 모드)
	DMA1_Stream5->FCR = 0;

	// M2P
	DMA1_Stream5->PAR = (unsigned int)&(TIM2->CCR1);

	// NVIC 인터럽트 허용
	NVIC_ClearPendingIRQ(DMA1_Stream5_IRQn);
	NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}

void DMA1_Stream5_Start(unsigned int memory_addr, unsigned short length)
{
	// // 이전 전송이 끝날 때까지 대기 (혹은 상위단에서 체크 후 호출)
	// while (dma_is_busy)
	// 	;

	// DMA1_Stream5->CR &= ~DMA_SxCR_EN;
	// while (DMA1_Stream5->CR & DMA_SxCR_EN)
	// 	;

	DMA1_STREAM5_DONE = 0;

	// 메모리(RAM) 버퍼 주소 지정 및 전송 데이터 개수 설정
	DMA1_Stream5->M0AR = memory_addr;
	DMA1_Stream5->NDTR = length;

	// Stream 5 관련 잔여 플래그 청소 (CTCIF5: Bit 11)
	// DMA1->HIFCR = (1U << 11U);
	DMA1->HIFCR = (0x3FU << 6U);

	// TIM2 Channel 1 DMA 요청 비트 활성화 (CC1DE: TIM2->DIER Bit 9)
	Macro_Set_Bit(TIM2->DIER, 9U);

	// DMA 전송 시작
	Macro_Set_Bit(DMA1_Stream5->CR, 0);
}
