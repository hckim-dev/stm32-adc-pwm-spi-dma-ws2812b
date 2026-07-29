#include "device_driver.h"
#include "ws2812b.h"

// 2차원 배열: [LED 4개][24비트 PWM CCR 값]
unsigned short led_pwm_table[LED_COUNT][24];

// 인터럽트에서 1.25us마다 순차적으로 출력할 1D 전송 버퍼 (RET 40개 + 96비트 = 136개)
unsigned short ccr_buffer[TOTAL_BITS];

volatile unsigned short tx_idx = 0;
volatile unsigned char is_busy = 0;

void WS2812_Set_Color(unsigned char index, unsigned int red, unsigned int green, unsigned int blue)
{
    if (index < 0 || index >= LED_COUNT)
        return;

    // GRB 24비트 데이터 생성
    unsigned int grb = (green << 16U) | (red << 8U) | (blue << 0U);

    for (int bit = 23; bit >= 0; bit--)
    {
        if (Macro_Check_Bit_Set(grb, bit))
        {
            led_pwm_table[index][23 - bit] = WS2812_DUTY_1; // '1' 비트 (38)
        }
        else
        {
            led_pwm_table[index][23 - bit] = WS2812_DUTY_0; // '0' 비트 (82)
        }
    }
}

// -------------------------------------------------------------
// 4. 2차원 배열을 인터럽트 전송 버퍼로 병합 후 전송 시작
// -------------------------------------------------------------
void WS2812_Update(void)
{
    while (is_busy)
        ;

    unsigned short idx = 0;

    // 1) RET 구간 채우기 (40개 x 1.25us = 50us Low)
    for (int i = 0; i < RET_COUNT; i++)
    {
        ccr_buffer[idx++] = WS2812_DUTY_RET;
    }

    // 2) 2차원 배열[4][24] 데이터를 1차원 전송 버퍼로 직렬화
    for (int i = 0; i < LED_COUNT; i++)
    {
        for (int b = 0; b < 24; b++)
        {
            ccr_buffer[idx++] = led_pwm_table[i][b];
        }
    }

    tx_idx = 0;
    is_busy = 1;

    Macro_Clear_Bit(TIM3->SR, 0U);   // TIM3 UIF 클리어
    NVIC_ClearPendingIRQ(TIM3_IRQn); // TIM3 NVIC Pending 클리어
    Macro_Set_Bit(TIM3->DIER, 0U);   // TIM3 UIE = 1 (인터럽트 시작)
}
