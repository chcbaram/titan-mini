#include "ap.h"


void apInit(void)
{
  //-- CPU0 이 이 값을 보고 CPU1 이 떴는지 판단한다.
  //
  g_shared.version = SHARED_VERSION;
  g_shared.magic   = SHARED_MAGIC;
}

void apMain(void)
{
  //-- 지금은 살아 있다는 표시만 남긴다.
  //   LED 는 CPU0 의 indicator 모듈이 소유하므로 건드리지 않는다.
  //
  while (1)
  {
    g_shared.cpu1_alive++;
    g_shared.cpu1_tick = millis();

    delay(100);
  }
}
