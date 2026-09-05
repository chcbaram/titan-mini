#ifndef HW_DEF_H_
#define HW_DEF_H_


#include "bsp.h"


#define _DEF_FIRMWATRE_VERSION    "V260905R1"
#define _DEF_BOARD_NAME           "TITAN-MINI-CM85"


#define _USE_HW_RTOS

#define _USE_HW_LED
#define      HW_LED_MAX_CH          3

#define _USE_HW_UART
#define      HW_UART_MAX_CH         1
#define      HW_UART_CH_CLI         _DEF_UART1
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
