# 구현 상세

[README로 돌아가기](../README.md)

현재 소스와 빌드 결과를 기준으로 한 구조 설명입니다. 아래 주파수·시간은 레지스터 설정에서 계산한 값이며 실측값이 아닙니다.

## 핀과 주변장치

| 용도 | 핀 | 설정 / 현재 역할 |
| --- | --- | --- |
| 가변저항 입력 | PA6 | Analog / ADC1_IN6 |
| WS2812B DIN | PA7 | AF02 / TIM3_CH2 |
| 모터 제어 신호 | PA0, PA1 | 정회전 시 PA0 GPIO Low, PA1 AF02 / TIM5_CH2 |
| SPI chip select | PA8 | GPIO / active-low |
| SPI SCK / MISO / MOSI | PB3 / PB4 / PB5 | AF05 / SPI1 |
| UART TX / RX | PA2 / PA3 | AF07 / USART2, 현재 메인은 로그 송신 사용 |
| 일반 LED | PA5 | GPIO Low로 초기화, 이후 표시 동작 없음 |

정확한 개발보드 모델, 모터 드라이버, 전원 및 레벨 변환 회로는 저장소에 명시되어 있지 않습니다. **모터 핀은 외부 구동회로에 전달하는 제어 신호**이며, PA0은 정회전 시 Low로 설정하는 GPIO입니다. 실제 배선은 대상 보드와 외부 회로를 확인해야 합니다.

## 시작 경로와 실행 흐름

```text
crt0.s: 벡터 테이블 → .data 복사 → .bss 초기화 → Main()
Main()
  ├─ Sys_Init(): FPU 접근 허용 → Clock_Init() → USART2 → stdout → PA5 LED
  ├─ Timer_Init(): TIM2 → TIM3 → TIM4 → TIM5
  ├─ ADC1_Init() → SPI1_Init() → SPI1_Config_GPIO() → Motor_Init()
  └─ 반복 루프
      ADC 읽기 → 출력값 계산 → 변화 시 세 출력·로그 갱신 → busy-wait
```

`Clock_Init()`은 HSI 16MHz와 PLL을 사용합니다. SYSCLK/HCLK 96MHz, PCLK1 48MHz, PCLK2 96MHz 설정이며 APB1 타이머 클럭은 96MHz입니다. ADC 클럭은 PCLK2 / 6 = 16MHz이고 CH6 sampling time은 480 cycles입니다.

| 실행 주체 | 역할 | 대기 / 공유 상태 |
| --- | --- | --- |
| main | ADC 읽기, 출력값 계산, LED 전송 준비, SPI 출력, 모터 갱신, 로그 | ADC EOC, SPI TXE/BSY, UART TXE 및 이전 LED 전송의 `is_busy` 확인 |
| TIM3 ISR | 전송 버퍼를 읽어 `CCR2` 갱신, 끝나면 update interrupt 차단 | `ccr_buffer`, `tx_idx`, `is_busy` |
| TIM4 ISR | `Sys_Tick` 증가 | main의 `Motor_Stop()`이 tick 변화 대기 |
| TIM5 | 하드웨어가 설정된 CCR 값으로 PWM 출력 | main이 듀티 설정, 전용 ISR 사용 안 함 |

메인은 [main.c](../main.c), 초기화는 [timer.c](../timer.c), ISR은 [exception.c](../exception.c)에 있습니다. TIM2는 초기화만 하고 시작하지 않습니다. `printf()`는 [runtime.c](../runtime.c)의 `_write()`를 통해 USART2 polling 송신으로 연결됩니다.

## WS2812B 전송

1. `WS2812_Set_Color()`가 RGB를 **GRB / MSB first / LED당 24비트**로 변환합니다.
2. `led_pwm_table[4][24]`에 compare 값을 저장합니다: `0` 비트는 82, `1` 비트는 38.
3. `WS2812_Update()`가 이전 전송을 기다린 뒤 reset용 40요소와 LED 데이터 96요소를 `ccr_buffer[136]`에 복사합니다.
4. TIM3 update interrupt를 허용하고 반환합니다. ISR이 요소를 순차적으로 `CCR2`에 씁니다.
5. 버퍼 소진 후 ISR이 reset용 CCR 값을 쓰고 `is_busy`를 해제합니다. 타이머는 계속 동작해 이후 Low 출력을 유지합니다.

색상 테이블은 192바이트, 전송 버퍼는 272바이트입니다. main은 색상 테이블을 준비하고, ISR은 별도 전송 버퍼를 읽습니다. `CCR2` preload가 켜져 있어 ISR에서 쓴 값은 다음 update event에 반영됩니다. `is_busy` 해제는 LED 내부 latch 완료를 확인한 상태가 아닙니다.

| 항목 | 현재 설정에서 계산한 값 |
| --- | --- |
| 카운터 | 96MHz, downcounter, `PSC=0`, `ARR=120` |
| 비트 주기 | `(120 + 1) / 96MHz ≈ 1.260µs`, 약 793.4kHz |
| `0` 비트 High | `(120 - 82) / 96MHz ≈ 0.396µs` |
| `1` 비트 High | `(120 - 38) / 96MHz ≈ 0.854µs` |
| 버퍼 앞 reset 구간 | 40주기에 해당하는 약 50.4µs Low |

소스 주석의 1.25µs / 800kHz와 계산값은 다릅니다. LED 리비전별 reset 요구사항과 실제 파형·ISR 지연은 별도 확인이 필요합니다. 인터럽트 우선순위는 별도로 설정하지 않습니다. 타이머 해석 기준: [ST RM0383](https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf).

## SPI와 모터 출력

| 항목 | 구현 |
| --- | --- |
| SPI 형식 | Master, mode 0, MSB first, 16-bit frame, GPIO CS |
| SPI 클럭 | `SPI1_Init(32)`: PCLK2 / 32 = 3MHz |
| SC16IS752 쓰기 | 상위 8비트는 write 명령·레지스터 주소·채널, 하위 8비트는 데이터 |
| GPIO 방향 / 상태 | `IODIR=0x0A`에 `0xFF`, `IOSTATE=0x0B`에 바그래프 패턴 쓰기 |
| 바그래프 계산 | `(adc × 8 + 2047) / 4095`로 반올림, 하위 비트부터 active-low 패턴 생성 |
| 모터 계산 | `adc × 100 / 4095`로 듀티 명령 생성, TIM5 `PSC=47`, `ARR=99` → 20kHz |
| 모터 갱신 | `Motor_Forward()` → `Motor_Stop()` → CCR 설정 → PA1 PWM 연결 |

SC16IS752는 `IODir`의 **1이 출력, 0이 입력**입니다. `spi.h` 주석은 반대로 적혀 있지만 현재 `0xFF` 설정은 전체 출력에 해당합니다. 기준: [NXP SC16IS752/SC16IS762 데이터시트](https://www.nxp.com/docs/en/data-sheet/SC16IS752_SC16IS762.pdf).

## 메인에서 사용하지 않는 코드

| 코드 | 현재 상태 |
| --- | --- |
| [dma.c](../dma.c) | M2M, USART2 TX, TIM2_CH1 대상 DMA 함수 존재. 메인 호출 없음 |
| [i2c.c](../i2c.c) | PB6/PB7 SC16IS752 I2C 쓰기 함수. 초기화·호출 없음 |
| [key.c](../key.c) | PC13 polling / EXTI 설정 함수. 메인 호출과 전용 C ISR 없음 |
| [motor.c](../motor.c) | 역회전 및 `MotorController` 기반 API. 메인은 `Motor_Forward()`만 사용 |
| [systick.c](../systick.c), [uart.c](../uart.c) | SysTick polling, USART1, USART2 RX interrupt 기능은 미사용 |

## 알려진 제한

### 현재 제어 경로

| 구분 | 현재 동작 / 확인할 사항 |
| --- | --- |
| 제어 주기 | 50,000회 빈 반복문에 ADC·UART·모터 대기가 더해짐. 고정 10ms 주기 보장 안 함 |
| 입력 안정화 | ADC 필터·hysteresis 없음. RGB만 달라져도 세 출력 모두 갱신 |
| 모터 정지와 듀티 | downcounter PWM mode 1 / active-high에서 명령 0은 `CCR=0`이므로 설정상 약 1% High 구간 존재. 속도 피드백 없음 |
| 모터 전환 대기 | 매 갱신에 PA0/PA1을 GPIO Low로 바꾸고 tick 1회 증가 대기. 정확한 최소 1ms나 물리적 정지 완료를 보장하지 않음 |
| 초기 모터 핀 | TIM5 초기화가 PA0/PA1을 AF로 연결한 뒤 시작함. 이후 `Motor_Init()`은 ODR만 지우므로 초기 구동 신호의 실측 필요 |
| 오류 복구 | ADC·SPI·UART polling에 timeout 없음. SPI 수신 데이터·OVR 처리와 readback 확인 없음 |

### 기반 코드와 미사용 드라이버

| 구분 | 현재 동작 / 확인할 사항 |
| --- | --- |
| CMSIS 클럭 변수 | `SystemInit()`·`SystemCoreClockUpdate()` 미호출. 실제 설정은 96MHz이나 `SystemCoreClock` 변수는 초기값 16MHz 유지 |
| 기본 예외 경로 | `crt0.s` alias로 기본 핸들러가 자기 자신으로 분기함을 ELF에서 확인. C의 `_Invalid_ISR()` 로그 경로와 연결되지 않음 |
| DMA 통합 | M2M 실제 전송 폭은 주석과 달리 32비트. USART2 TX DMA 함수에 USART DMA 요청 허용 설정 없음. TIM2 DMA도 현재 실행 경로에 미통합 |
| API 정리 | `device_driver.h`의 이전 SPI 함수명 및 구현 없는 DMA 초기화 선언이 남음. main은 별도 헤더의 현재 API 사용 |

이 문서의 검증 범위는 소스 대조, 레지스터 계산, IRQ 벡터·ELF 확인 및 빌드입니다. 실제 보드 다운로드, LED 동작, 모터 구동, 전기적 파형·응답시간은 문서 작성 과정에서 검증하지 않았습니다.
