#ifndef __WS2812B_H
#define __WS2812B_H

// -------------------------------------------------------------
// 96MHz 다운카운터 + PWM Mode 1 + Active Low 기준 타이밍
// -------------------------------------------------------------
#define WS2812_ARR 120U
#define WS2812_DUTY_0 82U    // '0' 비트 (High 0.4us / Low 0.85us)
#define WS2812_DUTY_1 38U    // '1' 비트 (High 0.85us / Low 0.4us)
#define WS2812_DUTY_RET 120U // RET 구간 (100% Low)

#define LED_COUNT 4U                               // Full LED 개수
#define RET_COUNT 40U                              // 40 * 1.25us = 50us (RET 구간)
#define TOTAL_BITS (RET_COUNT + (24U * LED_COUNT)) // 총 136개 비트

void WS2812_Set_Color(unsigned char index, unsigned int red, unsigned int green, unsigned int blue);
void WS2812_Update(void);

#endif /* __WS2812B_H */