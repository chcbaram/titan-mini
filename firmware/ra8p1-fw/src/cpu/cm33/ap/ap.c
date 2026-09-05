#include "ap.h"


void apInit(void)
{
  //-- 공유 블록 초기화는 hwInit() 의 ipcInit() 이 이미 했다.
  //   CPU0 은 그 시점부터 CPU1 을 살아 있는 것으로 본다.
}

void apMain(void)
{
  //-- 지금은 살아 있다는 표시만 남긴다.
  //   LED 는 CPU0 의 indicator 모듈이 소유하므로 건드리지 않는다.
  //
  while (1)
  {
    ipcUpdate();

    delay(100);
  }
}
