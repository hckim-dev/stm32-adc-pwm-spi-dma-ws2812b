#ifndef __SPI_H
#define __SPI_H

#include "device_driver.h"

// SC16IS752 레지스터 주소 정의
#ifndef SC16IS752_IODIR
#define SC16IS752_IODIR 0x0AU // IO 방향 레지스터 (0: Output, 1: Input)
#endif

#ifndef SC16IS752_IOSTATE
#define SC16IS752_IOSTATE 0x0BU // IO 출력 상태 레지스터
#endif

void SPI1_Init(unsigned int div);
void SPI1_Write_Reg(unsigned int addr, unsigned int data);
void SPI1_Config_GPIO(unsigned int config);
void SPI1_Write_GPIO(unsigned int data);

#endif /* __SPI_H */