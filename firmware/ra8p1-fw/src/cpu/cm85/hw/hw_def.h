#ifndef HW_DEF_H_
#define HW_DEF_H_


#include "bsp.h"


#define _DEF_FIRMWATRE_VERSION    "V260905R1"
#define _DEF_BOARD_NAME           "TITAN-MINI-CM85"


//-- LED
//   Titan Mini 의 RGB LED3 (XL-1615RGBC-RF)
//     LED1 : P109 RED    R80 2KΩ
//     LED2 : P108 GREEN  R81 12KΩ
//     LED3 : P110 BLUE   R82 10KΩ
//   공통 애노드가 +3V3 에 물려 있어 전부 active low 다. (회로도 13페이지)
//
//   주의: RA 계열 기본 디버그 핀이 P108~P110 이지만 이 보드는 디버그를
//         P208~P211 로 뺐다. pin_data.c 를 그렇게 잡아야 한다.
//
#define _USE_HW_LED
#define      HW_LED_MAX_CH          3


//-- UART
//   SCI2 : P801 TXD2 / P802 RXD2 — 회로도 13페이지의 H1 커넥터
//   회로도에는 H1 이 DNP 로 표기돼 있지만 실물에는 배선돼 있고, 여기에 HSLink
//   디버거의 가상 시리얼이 붙어 있다. 케이블 하나로 적재와 콘솔이 동시에 된다.
//
//   40핀 헤더의 SCI1(P707/P706) 도 UART 로 쓸 수 있다. 그쪽으로 옮기려면
//   configuration.xml 의 채널과 핀을 바꾸고 재생성한다. (docs/21-uart-cli.md)
//
#define _USE_HW_UART
#define      HW_UART_MAX_CH         1
#define      HW_UART_CH_CLI         _DEF_UART1
#define      HW_UART_CH_LOG         _DEF_UART1

//-- CLI
//
#define _USE_HW_CLI
#define      HW_CLI_CMD_LIST_MAX    32
#define      HW_CLI_CMD_NAME_MAX    16
#define      HW_CLI_LINE_HIS_MAX    8
#define      HW_CLI_LINE_BUF_MAX    64

//-- LOG
//
#define _USE_HW_LOG
#define      HW_LOG_CH              HW_UART_CH_LOG
#define      HW_LOG_BOOT_BUF_MAX    1024
#define      HW_LOG_LIST_BUF_MAX    1024


//-- 드라이버별 CLI 명령
//
#define _USE_CLI_HW_UART             1
#define _USE_CLI_HW_LOG              1


#endif
