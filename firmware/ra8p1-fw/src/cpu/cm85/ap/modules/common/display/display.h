#ifndef DISPLAY_H_
#define DISPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ap_def.h"


/*
 * 보드의 표시 장치를 한곳에서 관리한다.
 *
 * 드라이버(led.c)는 채널을 켜고 끄는 일만 하고, "언제 무슨 색으로 무엇을 알릴지" 는
 * 이 모듈이 정한다. 나중에 LCD 상태 표시나 부저가 붙어도 정책은 여기 모은다.
 *
 * 다른 모듈이 이 모듈을 직접 부르는 대신 이벤트를 던지는 것이 기본이다.
 * 예를 들어 이더넷 드라이버는 EVENT_ETH_LINK 만 발행하고, 그걸 LED 로 보여줄지는
 * 이 모듈이 정한다. 그래야 이더넷 드라이버가 LED 를 알 필요가 없다.
 *
 * RGB LED3 채널 배정
 *   LED1 (RED)   상태 하트비트
 *   LED2 (GREEN) 네트워크 링크
 *   LED3 (BLUE)  활동 표시
 */
typedef enum
{
  DISPLAY_STATE_BOOT = 0,   // 부팅 중
  DISPLAY_STATE_RUN,        // 정상 동작
  DISPLAY_STATE_BUSY,       // 오래 걸리는 작업 중
  DISPLAY_STATE_ERROR,      // 오류
  DISPLAY_STATE_MAX,
} DisplayState_t;


void           displaySetState(DisplayState_t state);
DisplayState_t displayGetState(void);

/*
 * 하트비트 한 틱.
 *
 * FreeRTOS 아이들 훅이 부른다. 아이들은 CPU 가 남을 때만 실행되므로 점멸 자체가
 * "여유가 있다" 는 표시가 된다. 호출자가 주기를 재고, 이 함수는 표시만 갱신한다.
 */
void displayHeartbeat(void);


#ifdef __cplusplus
}
#endif

#endif
