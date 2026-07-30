# 🚀 STM32F4 Multi-Peripheral Control System

STM32F4 베어메탈(Bare-metal) 레지스터 제어 기반으로 가변저항(ADC) 입력값에 따라 **WS2812B Full Color LED**, **SPI 확장 보드 LED**, **DC 모터 속도**를 동시 연동하는 시스템입니다.

---

## 📌 주요 기능 (Features)

* **가변저항 (ADC Input):** `PA6` (`ADC1_IN6`)을 통해 12비트 ($0 \sim 4095$) 아날로그 전압을 주기적으로 샘플링합니다.


* **WS2812B RGB LED:** `PA7` (`TIM3_CH2`) PWM 및 타이머 인터럽트를 이용해 $1.25\,\mu\text{s}$ 타이밍 파형을 정밀 출력하며, ADC 값에 따라 연속 무지개 색상(HSV/RGB)을 표현합니다.


* **SPI LED Bar-graph:** SPI1 통신(`PB3~5`, `PA8`)으로 `SC16IS752` GPIO 확장 칩을 제어하여 ADC 값 크기에 맞춰 8단계 LED 바그래프를 출력합니다.


* **DC Motor Control:** `TIM5_CH2` PWM 제어를 통해 ADC 비율에 맞춰 모터 회전 속도($0 \sim 100\%$)를 조절합니다.


* **DMA 드라이버:** Memory-to-Memory, USART2 TX, TIM2-DMA 전송 제어 모듈을 포함합니다.



---

## 🗺️ Pinout Map

| 기능 / 장치 | STM32 핀 | 설정 모드 | 설명 |
| --- | --- | --- | --- |
| **WS2812B DIN** | **`PA7`** | AF02 (`TIM3_CH2`) | $800\,\text{kHz}$ 정밀 PWM 파형 출력 |
| **가변저항 (ADC)** | **`PA6`** | Analog (`ADC1_IN6`) | 아날로그 입력을 읽어 제어 신호로 변환 |
| **DC 모터 (PWM)** | **`PA1`** | AF02 (`TIM5_CH2`) | PWM Duty비 기반 모터 속도 제어 |
| **DC 모터 (GND)** | **`PA0`** | GPIO Output (Low) | 모터 회전 방향 기준 전압 ($0\,\text{V}$) |
| **SPI CS** | **`PA8`** | GPIO Output | SC16IS752 Chip Select (Active Low) |
| **SPI1 (SCK/MISO/MOSI)** | **`PB3 ~ PB5`** | AF05 (`SPI1`) | SPI 확장 칩 통신 |

---

## 🏗️ 시스템 구조 (System Flow)

```
[ 가변저항 (PA6 ADC) ]
         │ (0 ~ 4095 전압 읽기)
         ▼
[ 메인 제어 루프 (main.c) ] ── (변화 감지 시에만 갱신)
         │
         ├───> WS2812B (PA7): 타이머 인터럽트 기반 24비트 GRB 파형 생성 (무지개 색상)
         ├───> SPI Board (PB3~5): SC16IS752 제어 (0~8개 LED 바그래프 출력)
         └───> DC Motor (PA1): TIM5 PWM Duty 제어 (0~100% 회전 속도)

```
