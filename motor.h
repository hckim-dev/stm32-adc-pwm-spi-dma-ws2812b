#ifndef __MOTOR_H__
#define __MOTOR_H__

#define STOP_DELAY_MS (1U)    // 모터 정지 지연 시간 (1ms)
#define MOTOR_BASE_DUTY (50U) // 기본 듀티비 (50%)
#define MOTOR_DUTY_STEP (5U)  // 속도 단계 (5%)

// 모터 동작 상태
typedef enum
{
    MOTOR_STATE_STOP = 0,
    MOTOR_STATE_FORWARD,
    MOTOR_STATE_REVERSE
} MotorState;

// 모터 제어 구조체
typedef struct
{
    MotorState state;    // 현재 동작 상태
    MotorState last_dir; // 마지막 동작 방향
    int duty;            // PWM 듀티비 (%)
} MotorController;

void Motor_Init(void);
void Motor_Stop(void);
void Motor_Forward(int duty);
void Motor_Reverse(int duty);
void Motor_Update(const MotorController *motor);

#endif // __MOTOR_H__