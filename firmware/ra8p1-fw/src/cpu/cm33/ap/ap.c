#include "ap.h"


void apInit(void)
{
  /*
   * 공유 블록은 NOLOAD 라 부팅 시 0 으로 밀리지 않는다. 그래야 나중에 부팅한
   * 코어가 상대가 써 둔 값을 지우지 않는다. 대신 자기 필드는 자기가 초기화한다.
   *
   * magic 을 마지막에 쓰는 것이 중요하다. CPU0 이 magic 만 보고 유효하다고
   * 판단하므로, 카운터가 쓰레기인 상태에서 magic 이 먼저 서면 CPU0 이 그걸 읽는다.
   */
  g_shared.cpu1_alive = 0;
  g_shared.cpu1_tick  = 0;
  g_shared.version    = SHARED_VERSION;

  __DMB();
  g_shared.magic = SHARED_MAGIC;
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
