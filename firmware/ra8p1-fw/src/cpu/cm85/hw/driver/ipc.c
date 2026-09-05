#include "ipc.h"

#ifdef _USE_HW_IPC
#include "cli.h"
#include "shared.h"


/*
 * RA8P1 구현.
 *
 * 상대 코어는 CPU1(Cortex-M33) 이다. 이 파일은 MCU 별 계층이므로 여기서는
 * 벤더 용어를 그대로 쓴다. 중립 이름은 common/hw/include/ipc.h 가 유지한다.
 *
 * 공유 블록의 실체는 두 코어가 각자 정의하고, 링커가 .shared 섹션을 양쪽 모두
 * 같은 주소에 놓아서 주소가 일치한다. static 이라 이 파일 밖에서는 보이지 않는다 -
 * 바깥은 ipc.h 의 함수로만 접근한다. NOLOAD 라 이미지에 담기지 않고 시작 시
 * 0 으로 밀리지도 않는다.
 */
__attribute__((section(".shared"), used))
static shared_t shared;


#if CLI_USE(HW_IPC)
static void cliIpc(cli_args_t *args);
#endif


static IpcState_t ipc_state    = IPC_STATE_DISABLED;
static uint32_t   boot_time_ms = 0;

//-- ipcIsRunning() 이 쓰는 캐시. alive 가 마지막으로 변한 시각을 들고 있는다.
static uint32_t   last_alive    = 0;
static uint32_t   last_alive_ms = 0;

static const char *ipc_state_str[] =
  {
    "DISABLED",
    "TIMEOUT",
    "BAD_VERSION",
    "RUNNING",
  };


bool ipcInit(void)
{
#if _HW_DEF_CPU1_IMAGE
  uint32_t pre_ms;

  //-- 낡은 magic 을 먼저 지운다. 이유는 ipc.h 주석 참고.
  //
  shared.magic = 0;
  __DMB();

  pre_ms = millis();

  //-- FSP 의 유일한 기동 API. CPU1INITVTOR / CPU1WAITCR / CPU1ACTCSR 을 쓴다.
  //   반환값이 없으므로 성공 여부는 아래 핸드셰이크로 직접 확인해야 한다.
  //
  R_BSP_SecondaryCoreStart();

  ipc_state = IPC_STATE_TIMEOUT;

  while ((millis() - pre_ms) < HW_IPC_BOOT_TIMEOUT_MS)
  {
    if (shared.magic == SHARED_MAGIC)
    {
      __DMB();

      //-- 구조체가 바뀐 채 한쪽만 다시 빌드하면 여기서 걸린다.
      if (shared.version == SHARED_VERSION)
        ipc_state = IPC_STATE_RUNNING;
      else
        ipc_state = IPC_STATE_BAD_VERSION;
      break;
    }
    delay(1);
  }

  boot_time_ms  = millis() - pre_ms;
  last_alive    = shared.peer_alive;
  last_alive_ms = millis();
#else
  //-- CPU1 이미지를 빌드하지 않았다. 깨우면 빈 MRAM 으로 뛰어 폴트에 빠지므로
  //   기동 자체를 하지 않는다.
  //
  ipc_state = IPC_STATE_DISABLED;
#endif

#if CLI_USE(HW_IPC)
  cliAdd("ipc", cliIpc);
#endif

  return (ipc_state == IPC_STATE_RUNNING);
}

IpcState_t ipcGetState(void)
{
  return ipc_state;
}

const char *ipcGetStateStr(void)
{
  return ipc_state_str[ipc_state];
}

bool ipcIsBooted(void)
{
  return (ipc_state == IPC_STATE_RUNNING);
}

bool ipcIsRunning(void)
{
  uint32_t alive;

  if (ipc_state != IPC_STATE_RUNNING)
    return false;

  //-- 상대 코어가 도중에 죽으면 magic 은 남아 있어도 카운터가 멈춘다.
  if (shared.magic != SHARED_MAGIC)
    return false;

  alive = shared.peer_alive;
  if (alive != last_alive)
  {
    last_alive    = alive;
    last_alive_ms = millis();
  }

  return ((millis() - last_alive_ms) < HW_IPC_ALIVE_TIMEOUT_MS);
}

uint32_t ipcGetBootTime(void)
{
  return boot_time_ms;
}

uint32_t ipcGetAliveCnt(void)
{
  return shared.peer_alive;
}

uint32_t ipcGetTick(void)
{
  return shared.peer_tick;
}


#if CLI_USE(HW_IPC)
void cliIpc(cli_args_t *args)
{
  bool ret = false;

  if (args->argc == 1 && args->isStr(0, "info"))
  {
    cliPrintf("peer       : CPU1 (Cortex-M33)\n");
    cliPrintf("image      : %s\n",
              _HW_DEF_CPU1_IMAGE ? "있음" : "없음 (BUILD_CM33=OFF)");
    cliPrintf("state      : %s\n", ipcGetStateStr());

    if (ipc_state == IPC_STATE_RUNNING || ipc_state == IPC_STATE_BAD_VERSION)
    {
      uint32_t a0 = shared.peer_alive;
      uint32_t t0 = shared.peer_tick;

      cliPrintf("boot time  : %d ms\n", (int)boot_time_ms);
      cliPrintf("magic      : 0x%08X\n", (unsigned)shared.magic);
      cliPrintf("version    : %d (기대 %d)\n",
                (int)shared.version, (int)SHARED_VERSION);

      delay(500);

      cliPrintf("alive      : %d  (+%d / 500ms)\n",
                (int)shared.peer_alive, (int)(shared.peer_alive - a0));
      cliPrintf("tick       : %d ms  (+%d)\n",
                (int)shared.peer_tick, (int)(shared.peer_tick - t0));
      cliPrintf("running    : %s\n", ipcIsRunning() ? "예" : "아니오");
    }
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("ipc info\n");
  }
}
#endif

#endif
