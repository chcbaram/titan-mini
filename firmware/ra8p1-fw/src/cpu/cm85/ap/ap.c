#include "ap.h"
#include "module.h"


void apInit(void)
{
  //-- 각 모듈의 init() 이 여기서 실행된다. 모듈은 그 안에서 threadCreate() 로
  //   자기 스레드를 등록한다.
  //
  moduleInit();
}

void apMain(void)
{
  //-- 등록된 스레드를 모두 만든다. 스케줄러는 main.c 가 이미 돌리고 있다.
  //
  threadBegin();

  logBoot(false);

  eventPub(EVENT_BOOT_DONE, 0);

  //-- LED 점멸은 아이들 훅이 한다(rtos.c). 여기서 하면 main 스레드가 도는 것만
  //   보여줘서 그보다 낮은 스레드가 굶는 상황을 잡지 못한다.
  //
  while (1)
  {
    eventUpdate();
    delay(10);
  }
}
