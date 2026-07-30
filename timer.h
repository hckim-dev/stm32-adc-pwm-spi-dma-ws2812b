#ifndef __TIMER_H__
#define __TIMER_H__

#define TIM_TICK (20U)                  // 틱 단위 (us)
#define TIM_FREQ (1000000.0 / TIM_TICK) // 카운트 주파수 (Hz)
#define TIM_1MS_PLS (TIM_FREQ / 1000.0) // 1ms당 카운트 펄스 수
#define TIM_MAX_PLS (0xFFFFU)           // 16비트 타이머 최대 값

#define TIM5_PWM_FREQ (20000U)               // 목표 PWM 주파수 (20kHz)
#define TIM5_CNT_FREQ (TIM5_PWM_FREQ * 100U) // 카운트 클럭 (목표 주파수 * 100)

// 모든 타이머 모듈 일괄 초기화
void Timer_Init(void);

// TIM2: 스위치 롱클릭 타임아웃용 타이머
void TIM2_Init(void);
void TIM2_Start(int delay_ms);
void TIM2_Stop(void);

// WS2812B
void TIM3_Init(void);

// TIM4: 1ms 시스템 틱 생성 타이머
void TIM4_Init(void);

// TIM5: 모터 PWM 제어 타이머
void TIM5_Init(void);
void TIM5_Set_Duty(int ch1_duty, int ch2_duty);

#endif // __TIMER_H__