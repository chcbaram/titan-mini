#ifndef HW_DEF_H_
#define HW_DEF_H_


#include "bsp.h"


#define _DEF_FIRMWATRE_VERSION    "V260905R1"
#define _DEF_BOARD_NAME           "TITAN-MINI-CM85"


//-- 하드웨어 핀맵 (Titan Mini HW:V1.0, 회로도 기준. docs/03-board-mapping.md)
//
//   LED      : P109(R) P108(G) P110(B)  RGB LED3 XL-1615RGBC-RF
//              공통 애노드가 +3V3 이라 전부 active low
//              RA 기본 디버그 핀 자리지만 이 보드는 디버그를 P208~P211 로 뺐다
//   디버그   : P211(SWCLK) P210(SWDIO) P209(SWO) P208(TDI)  J6 FTSH-105
//   UART     : P801(TXD2) / P802(RXD2)  SCI2, H1 커넥터
//              HSLink 디버거의 가상 시리얼이 여기 붙어 있어 케이블 하나로
//              적재와 콘솔이 동시에 된다. 회로도에는 H1 이 DNP 로 표기돼 있다
//   부트모드 : P201(MD) <- SW2 USER/BOOT 버튼
//   I2C1     : P512(SCL) / P511(SDA)   IMU LSM6DS3TR-C @0x6A + 터치
//   OSPI0    : SCLK=P808 CS1=P104 SIO0~3=P100/P803/P103/P101  (W25Q64 8MB, 쿼드)
//   SDHI     : CLK=PD05 CMD=PD04 DAT0~2=PD03/PD02/PD01 CD=PD07
//   CAN      : CTX0=P312 / CRX0=P311
//


#define _USE_HW_RTOS

#define _USE_HW_LED
#define      HW_LED_MAX_CH          3

#define _USE_HW_UART
#define      HW_UART_MAX_CH         1
#define      HW_UART_CH_CLI         _DEF_UART1    // SCI2  P801/P802
#define      HW_UART_CH_LOG         _DEF_UART1

#define _USE_HW_CLI
#define      HW_CLI_CMD_LIST_MAX    32
#define      HW_CLI_CMD_NAME_MAX    16
#define      HW_CLI_LINE_HIS_MAX    8
#define      HW_CLI_LINE_BUF_MAX    64

#define _USE_HW_LOG
#define      HW_LOG_CH              HW_UART_CH_LOG
#define      HW_LOG_BOOT_BUF_MAX    1024
#define      HW_LOG_LIST_BUF_MAX    1024

#define _USE_HW_EVENT
#define      HW_EVENT_Q_MAX         16
#define      HW_EVENT_NODE_MAX      8

#define _USE_HW_THREAD
#define      HW_THREAD_MAX_CNT      8


//-- 스레드 우선순위와 스택
//   configMAX_PRIORITIES 는 8 이다. 아이들이 0, 타이머 서비스가 7 을 쓴다.
//
#define _HW_DEF_THREAD_MAIN_PRI       2
#define _HW_DEF_THREAD_MAIN_STACK     (4 * 1024)
#define _HW_DEF_THREAD_CLI_PRI        3
#define _HW_DEF_THREAD_CLI_STACK      (4 * 1024)


//-- 드라이버별 CLI 명령
//
#define _USE_CLI_HW_UART             1
#define _USE_CLI_HW_LOG              1
#define _USE_CLI_HW_THREAD           1
#define _USE_CLI_HW_EVENT            1
#define _USE_CLI_HW_MODULE           1
#define _USE_CLI_HW_DISPLAY          1


#endif
