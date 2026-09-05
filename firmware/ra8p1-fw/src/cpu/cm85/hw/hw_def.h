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


#endif
