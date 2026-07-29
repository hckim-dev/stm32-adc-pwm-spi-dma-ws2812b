#include "device_driver.h"
#include "timer.h"
#include "ws2812b.h"
#include "adc.h"
#include "spi.h"
#include "motor.h"
#include <stdio.h>

// ADC -> RGB 연속 무지개 색상 변환
static void ADC_To_RGB(unsigned short adc, unsigned char *r, unsigned char *g, unsigned char *b)
{
    unsigned int pos = ((unsigned int)adc * 1530U) / 4095U;

    if (pos < 255U)
    {
        *r = 255U;
        *g = (unsigned char)pos;
        *b = 0U;
    }
    else if (pos < 510U)
    {
        *r = (unsigned char)(255U - (pos - 255U));
        *g = 255U;
        *b = 0U;
    }
    else if (pos < 765U)
    {
        *r = 0U;
        *g = 255U;
        *b = (unsigned char)(pos - 510U);
    }
    else if (pos < 1020U)
    {
        *r = 0U;
        *g = (unsigned char)(255U - (pos - 765U));
        *b = 255U;
    }
    else if (pos < 1275U)
    {
        *r = (unsigned char)(pos - 1020U);
        *g = 0U;
        *b = 255U;
    }
    else
    {
        *r = 255U;
        *g = 0U;
        *b = (unsigned char)(255U - (pos - 1275U));
    }
}

// ADC -> SPI LED 0~8개 바그래프 변환 (Active Low)
static unsigned char ADC_To_SPI_LED(unsigned short adc)
{
    unsigned int num_leds = ((unsigned int)adc * 8U + 2047U) / 4095U;
    if (num_leds > 8U)
        num_leds = 8U;
    if (num_leds == 0U)
        return 0xFFU;
    return (unsigned char)~((1U << num_leds) - 1U);
}

static void Sys_Init(int baud)
{
    SCB->CPACR |= (0x3 << 10 * 2) | (0x3 << 11 * 2);
    Clock_Init();
    Uart2_Init(baud);
    setvbuf(stdout, NULL, _IONBF, 0);
    LED_Init();
}

void Main(void)
{
    unsigned short adc_val;
    unsigned char r = 0, g = 0, b = 0;
    unsigned char spi_data = 0xFF;

    // 이전 값 저장용 (중복 전송 방지)
    unsigned char prev_r = 0, prev_g = 0, prev_b = 0;
    unsigned char prev_spi = 0x00;

    int motor_duty = 0;
    int prev_motor_duty = -1;

    Sys_Init(115200);
    printf("\n=== System Initializing (PA7 / TIM3_CH2 WS2812B) ===\n");

    Timer_Init();           // WS2812B (PA7, TIM3_CH2)
    ADC1_Init();            // 가변저항 ADC (PA6)
    SPI1_Init(32);          // SPI1 (PB3, PB4, PB5, PA8)
    SPI1_Config_GPIO(0xFF); // SC16IS752 Output 설정
    Motor_Init();

    printf("System Ready!\n");

    for (;;)
    {
        // 1. ADC 값 읽기 (0 ~ 4095)
        adc_val = ADC1_Read();

        // 2. 값 매핑 (RGB 색상 & SPI LED 개수)
        ADC_To_RGB(adc_val, &r, &g, &b);
        spi_data = ADC_To_SPI_LED(adc_val);
        motor_duty = ((int)adc_val * 100) / 4095; // 0 ~ 100%

        // 3. 변화가 있을 때만 장치 업데이트
        if (r != prev_r || g != prev_g || b != prev_b || spi_data != prev_spi || motor_duty != prev_motor_duty)
        {
            prev_r = r;
            prev_g = g;
            prev_b = b;
            prev_spi = spi_data;
            prev_motor_duty = motor_duty;

            // WS2812B 색상 업데이트 (PA7 핀으로 출력)
            WS2812_Set_Color(0, r, g, b);
            WS2812_Set_Color(1, r, g, b);
            WS2812_Set_Color(2, r, g, b);
            WS2812_Set_Color(3, r, g, b);
            WS2812_Update();

            // SPI LED 바그래프 업데이트
            SPI1_Write_GPIO(spi_data);

            // 모터 속도 업데이트 (PA1)
            Motor_Forward(motor_duty);

            printf("ADC: %4d | Motor: %3d%% | RGB:(%3d,%3d,%3d) | SPI: 0x%02X\n", adc_val, motor_duty, r, g, b, spi_data);
        }

        // 10ms 루프 딜레이
        for (volatile unsigned int i = 0; i < 50000U; i++)
            ;
    }
}