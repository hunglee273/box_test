/**
  * @file    board_config.h
  * @brief   Cau hinh phan cung tap trung: chan, ngoai vi, gioi han an toan.
  *          Moi module trong App/ chi lay thong so tu file nay.
  */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==== Bat/tat module khi build (1 = co, 0 = khong) ======================= */
#define TEST_EN_SERVO             1
#define TEST_EN_OPTICAL           1
#define TEST_EN_CYLINDER          1
#define TEST_EN_STEPPER           1
#define TEST_EN_INA226            1

/* ==== Console: USART3 = ST-Link VCP (PD8 TX, PD9 RX, AF7) ================ */
/* Chan da duoc MX_GPIO_Init cau hinh. USART3 KHONG bat trong CubeMX,
   console tu khoi tao bang thanh ghi. Neu sau nay bat USART3 trong CubeMX
   thi phai bo USART3_IRQHandler trong stm32h7xx_it.c hoac trong console.c. */
#define CONSOLE_UART              USART3
#define CONSOLE_UART_IRQn         USART3_IRQn
#define CONSOLE_UART_IRQHandler   USART3_IRQHandler
#define CONSOLE_UART_CLK_ENABLE() __HAL_RCC_USART3_CLK_ENABLE()
#define CONSOLE_UART_CLK_HZ       100000000U  /* kernel clock = PCLK1 */
#define CONSOLE_BAUD              115200U
#define CONSOLE_IRQ_PRIO          5U
#define CONSOLE_ECHO              0  /* 1 neu terminal khong tu hien ky tu go (minicom, PuTTY) */

/* ==== Timer (clock timer = 200 MHz, Prescaler 199 -> 1 tick = 1 us) ===== */
/* TIM4: servo CH1 + xi lanh trai CH3, Period 3002 -> 333 Hz (dung chung)
   TIM2: xi lanh phai CH1,             Period 3002 -> 333 Hz
   TIM1: 3 dong co buoc CH1/CH3/CH4, chay Output Compare Toggle (module
         stepper tu cau hinh lai luc khoi dong), moi kenh tan so rieng.     */
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;
extern I2C_HandleTypeDef hi2c1;

/* ==== Servo: TIM4_CH1 (PD12), chu ky 3003 us (333 Hz) ==================== */
/* Servo analog thuong chi chiu 50 Hz; 333 Hz chi dung cho servo digital. */
#define SERVO_TIM                 (&htim4)
#define SERVO_TIM_CH              TIM_CHANNEL_1

#define SERVO_PW_MIN              1710  /* dai an toan co khi: 1710..1900 us */
#define SERVO_PW_MAX              1900
#define SERVO_PW_INIT             1900  /* vi tri khi khoi dong / stop */
#define SERVO_PW_RELEASE          1900
#define SERVO_PW_PRESS            1710

#define SERVO_T_PRESS_ON          500   /* ms - lenh bat nguon  */
#define SERVO_T_PRESS_OFF         2500  /* ms - lenh tat nguon  */
#define SERVO_T_PRESS_MAX         10000 /* ms - gioi han lenh press */
#define SERVO_T_RELEASE           400   /* ms - cho servo ve vi tri nha */
#define SERVO_T_SETTLE            500   /* ms - giu xung sau home roi tat */
#define SERVO_T_CYCLE_GAP         3000  /* ms - nghi giua 2 lan nhan khi cycle */

/* ==== Cam bien quang (Photo Microsensor), ngat EXTI =================== */
/* Chan, pull, suon ngat cau hinh trong CubeMX. Moi chan phai nam tren mot
   EXTI line rieng (so chan khac nhau). Ten trong console = ten o cot 1.  */
#define OPTICAL_SENSOR_LIST(X)                                   \
  X(OP_LM_SW1,    OP_LM_SW1_GPIO_Port,    OP_LM_SW1_Pin)         \
  X(OP_LM_SW2,    OP_LM_SW2_GPIO_Port,    OP_LM_SW2_Pin)         \
  X(CL_LM_SW1,    CL_LM_SW1_GPIO_Port,    CL_LM_SW1_Pin)         \
  X(CL_LM_SW2,    CL_LM_SW2_GPIO_Port,    CL_LM_SW2_Pin)         \
  X(CT_LM_SW1,    CT_LM_SW1_GPIO_Port,    CT_LM_SW1_Pin)         \
  X(LM_SW1_H,     LM_SW1_H_GPIO_Port,     LM_SW1_H_Pin)          \
  X(LM_SW2_H,     LM_SW2_H_GPIO_Port,     LM_SW2_H_Pin)          \
  X(LM_SW1_V,     LM_SW1_V_GPIO_Port,     LM_SW1_V_Pin)          \
  X(LM_SW2_V,     LM_SW2_V_GPIO_Port,     LM_SW2_V_Pin)          \
  X(LM_SW_END_H1, LM_SW_END_H1_GPIO_Port, LM_SW_END_H1_Pin)      \
  X(LM_SW_END_H2, LM_SW_END_H2_GPIO_Port, LM_SW_END_H2_Pin)

/* Ngo ra NPN cua cam bien dan (ON) -> chan bi keo xuong LOW.
   ON la "bi che" hay "thong" tuy che do Light-ON/Dark-ON cua cam bien. */
#define OPTICAL_ON_LEVEL          GPIO_PIN_RESET
#define OPTICAL_DEBOUNCE_MS       5     /* tin hieu phai im lang bay lau moi chot */
#define OPTICAL_RESCAN_MS         50    /* doc lai dinh ky, bat canh ma EXTI bo sot */
#define OPTICAL_STORM_WINDOW_MS   100
#define OPTICAL_STORM_LIMIT       500   /* so ngat/cua so: vuot qua -> khoa ngat chan do */
#define OPTICAL_TEST_TIMEOUT_MS   30000 /* optical test: thoi gian cho moi buoc */

/* ==== Xi lanh dien: Cytron SmartDriveDuo-30 (MDDS30) =================== */
/* Che do PWM + Signed-Magnitude, Independent Both (DIP: 1 0 1 1 0 1 x x).
   L = motor LEFT: PWM -> AN1, DIR -> IN1.  R = motor RIGHT: PWM -> AN2, DIR -> IN2.
   MDDS30 loc PWM thanh dien ap, full toc do = 5 V -> PWM 3.3 V chi dat ~66%.
   Luc MDDS30 bat nguon/reset, AN phai = 0 V (duty 0) neu khong bao Input Error. */
/*        ten, timer,  kenh PWM,      chan DIR                             */
#define CYL_LIST(X)                                                          \
  X(L, &htim4, TIM_CHANNEL_3, DIR_L_XL_GPIO_Port, DIR_L_XL_Pin)  /* PD14 */  \
  X(R, &htim2, TIM_CHANNEL_1, DIR_R_XL_GPIO_Port, DIR_R_XL_Pin)  /* PA5  */

#define CYL_DIR_EXTEND_LEVEL      GPIO_PIN_SET  /* muc IN khi day ra; sai chieu thi doi */
#define CYL_DUTY_DEFAULT          50    /* % */
#define CYL_DUTY_MAX              100   /* % - gioi han an toan */
#define CYL_T_DEFAULT_MS          2000  /* thoi gian chay mac dinh */
#define CYL_T_MAX_MS              30000 /* thoi gian chay toi da moi lenh */
#define CYL_RAMP_MS               200   /* tang duty tu 0 len muc dat trong bay lau */
#define CYL_DEADTIME_MS           100   /* dung han truoc khi doi chieu */

/* ==== Dong co buoc: MISUMI E-42ESTM01 + driver EDR42A (closed loop) ====== */
/* Dau kieu PNP (cathode chung): PUL-, DIR-, ENA- noi GND; PUL+, DIR+, ENA+
   nhan tin hieu tu MCU. Driver can muc 5-24 V nen dat mach dem 5 V
   (vd 74HCT245/74AHCT125 cap 5 V) giua chan MCU va PUL+/DIR+/ENA+.
   ALM+ -> chan ALM cua MCU (pull-up), ALM- -> GND.
   DIP EDR42A: SW3, SW4, SW6 = ON (tra bang tren driver de biet PPR). */
/*        ten, kenh, PUL,              DIR,                ENA,                ALM,                dao chieu */
#define STEPPER_LIST(X)                                                                                                       \
  X(H1, 4, GPIOE, GPIO_PIN_14, DIR1_GPIO_Port, DIR1_Pin, ENA1_GPIO_Port, ENA1_Pin, ALM1_GPIO_Port, ALM1_Pin, 0)  \
  X(V,  3, GPIOE, GPIO_PIN_13, DIR2_GPIO_Port, DIR2_Pin, ENA2_GPIO_Port, ENA2_Pin, ALM2_GPIO_Port, ALM2_Pin, 0)  \
  X(H2, 1, GPIOE, GPIO_PIN_9,  DIR3_GPIO_Port, DIR3_Pin, ENA3_GPIO_Port, ENA3_Pin, ALM3_GPIO_Port, ALM3_Pin, 0)

#define STEPPER_TIM               TIM1
#define STEPPER_TIM_CC_IRQn       TIM1_CC_IRQn
#define STEPPER_TIM_CC_IRQHandler TIM1_CC_IRQHandler  /* KHONG bat ngat nay trong CubeMX */
#define STEPPER_TIM_CLK_HZ        200000000U
#define STEPPER_IRQ_PRIO          1U

/* 0 = PNP / cathode chung: push-pull, opto dan khi chan HIGH (dang dung)
   1 = NPN / anode chung len 5 V: open-drain, opto dan khi chan LOW */
#define STEPPER_COMMON_ANODE      0
/* Driver closed-loop thuong: opto ENA dan = TAT driver (tha truc).
   Khong noi day ENA = driver luon bat. Neu nguoc thi dat 0. */
#define STEPPER_ENA_OPTO_DISABLES 1
#define STEPPER_ALM_ACTIVE_LEVEL  GPIO_PIN_RESET  /* muc chan ALM khi driver bao loi */
#define STEPPER_ALM_STOP          0     /* 1 = dung + chan lenh khi ALM; bat sau khi da xac nhan muc ALM */
#define STEPPER_ENA_DELAY_MS      200   /* cho driver san sang sau khi bat ENA */

#define STEPPER_PPR               1600  /* xung/vong theo DIP EDR42A - CHUA XAC NHAN, do bang "step H1 move 1600" */
#define STEPPER_SPS_MIN           10    /* buoc/s - gioi han boi timer 16 bit */
#define STEPPER_SPS_MAX           50000 /* buoc/s */
#define STEPPER_SPS_DEFAULT       1600  /* = 1 vong/s voi PPR 1600 */
#define STEPPER_SPS_START         200   /* toc do bat dau / ket thuc ramp */
#define STEPPER_ACC_DEFAULT       3200  /* buoc/s^2 */
#define STEPPER_MOVE_MAX          1000000 /* so buoc toi da moi lenh move */

/* ==== INA226 do dien ap / dong (I2C1: PB6 SCL, PB7 SDA, 100 kHz) ======= */
/* Gia tri mac dinh theo module nguon Holybro PM02D (PX4: dia chi 0x41,
   shunt 0.0005 ohm) - CHUA XAC NHAN voi mach thuc te, kiem tra bang
   'ina scan' va thong so shunt tren mach. Dieu kien: shunt * Imax <= 80 mV. */
#define INA226_I2C                (&hi2c1)
#define INA226_ADDR_7BIT          0x41
#define INA226_SHUNT_OHM          0.0005f
#define INA226_MAX_CURRENT_A      120.0f
#define INA226_AVERAGE            2     /* 0..7 = 1,4,16,64,128,256,512,1024 mau */
#define INA226_STREAM_MIN_MS      20

#ifdef __cplusplus
}
#endif

#endif /* BOARD_CONFIG_H */
