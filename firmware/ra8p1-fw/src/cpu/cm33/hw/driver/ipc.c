#include "ipc.h"

#ifdef _USE_HW_IPC
#include "shared.h"


/*
 * 상대 코어(CPU1) 쪽 구현.
 *
 * 여기서는 자기가 살아 있다는 것만 알린다. 기동을 요청하는 쪽은 CPU0 이다.
 *
 * 공유 블록의 실체를 이 파일 안에 둔다. 두 코어가 각자 정의하고 링커가 .shared 를
 * 같은 주소에 놓아서 주소가 일치한다. static 이라 바깥에서는 보이지 않는다.
 */
__attribute__((section(".shared"), used))
static shared_t shared;


bool ipcInit(void)
{
  /*
   * 공유 블록은 NOLOAD 라 부팅 시 0 으로 밀리지 않는다. 그래야 먼저 부팅한 코어가
   * 써 둔 값을 나중에 부팅한 코어가 지우지 않는다. 대신 자기 필드는 자기가 잡는다.
   *
   * magic 을 마지막에 쓰는 것이 중요하다. CPU0 이 magic 을 보고 유효성을 판단하므로,
   * 카운터가 쓰레기인 상태에서 magic 이 먼저 서면 CPU0 이 그걸 읽는다.
   * CPU0 은 기동 전에 magic 을 0 으로 지워 두고 이 값이 서기를 기다린다.
   */
  shared.peer_alive = 0;
  shared.peer_tick  = 0;
  shared.version    = SHARED_VERSION;

  __DMB();
  shared.magic = SHARED_MAGIC;

  return true;
}

void ipcUpdate(void)
{
  shared.peer_alive++;
  shared.peer_tick = millis();
}

#endif
