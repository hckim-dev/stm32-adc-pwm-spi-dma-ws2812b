#ifndef __DMA_H
#define __DMA_H

// 함수 프로토타입 선언
void DMA1_Stream5_Init(void);
void DMA1_Stream5_Start(unsigned int memory_addr, unsigned short length);

#endif /* __DMA_H */