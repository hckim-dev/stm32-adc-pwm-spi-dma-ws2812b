#include "device_driver.h"
#include "motor.h"
#include "timer.h"

extern volatile unsigned long Sys_Tick;

// 모터 제어 GPIO(PA0, PA1) 초기화
void Motor_Init(void)
{
    Macro_Set_Bit(RCC->AHB1ENR, 0U); // GPIOA 클럭 활성화

    // PA0, PA1 초기 출력값 0V(Low) 설정
    Macro_Clear_Bit(GPIOA->ODR, 0U);
    Macro_Clear_Bit(GPIOA->ODR, 1U);
}

// 모터 정지 (PA0, PA1 모두 Output 모드 0V)
void Motor_Stop(void)
{
    unsigned long start_tick = Sys_Tick;

    Macro_Write_Block(GPIOA->MODER, 0xFU, 0x5U, 0U);
    while ((Sys_Tick - start_tick) < STOP_DELAY_MS)
        ;
}

// 모터 정회전 설정
void Motor_Forward(int duty)
{
    Motor_Stop();

    TIM5_Set_Duty(0, duty);
    Macro_Write_Block(GPIOA->MODER, 0xFU, 0x9U, 0U); // PA0: Output(0V), PA1: AF(PWM)
}

// 모터 역회전 설정
void Motor_Reverse(int duty)
{
    Motor_Stop();

    TIM5_Set_Duty(duty, 0);
    Macro_Write_Block(GPIOA->MODER, 0xFU, 0x6U, 0U); // PA0: AF(PWM), PA1: Output(0V)
}

// 모터 컨트롤러 상태 업데이트
void Motor_Update(const MotorController *motor)
{
    switch (motor->state)
    {
    case MOTOR_STATE_STOP:
        Motor_Stop();
        break;

    case MOTOR_STATE_FORWARD:
        Motor_Forward(motor->duty);
        break;

    case MOTOR_STATE_REVERSE:
        Motor_Reverse(motor->duty);
        break;

    default:
        Motor_Stop();
        break;
    }
}
