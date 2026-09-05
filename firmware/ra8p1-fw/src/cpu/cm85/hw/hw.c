#include "hw.h"


bool hwInit(void)
{
  //-- 순서가 중요하다.
  //   cliInit / logInit 은 버퍼만 잡으므로 먼저 부른다. 그래야 이후 드라이버가
  //   cliAdd() 로 자기 명령을 등록할 수 있고, logPrintf() 가 열리기 전 출력을
  //   부트 버퍼에 모아 둘 수 있다.
  //
  cliInit();
  logInit();
  threadInit();
  eventInit();

  ledInit();
  uartInit();

  uartOpen(HW_UART_CH_CLI, 115200);

  logOpen(HW_LOG_CH, 115200);
  logPrintf("\r\n[ Firmware Begin... ]\r\n");
  logPrintf("Booting..Name  \t\t: %s\r\n", _DEF_BOARD_NAME);
  logPrintf("Booting..Ver   \t\t: %s\r\n", _DEF_FIRMWATRE_VERSION);
  logPrintf("Booting..Clock \t\t: %d MHz\r\n", (int)(SystemCoreClock / 1000000));

  //-- CPU1 기동은 로그가 열린 뒤에 한다. 그래야 시도와 결과를 남길 수 있다.
  //   (예전에는 hal_entry() 에서 부팅 아주 초기에 했다)
  //
  ipcInit();

  //-- 로그는 logPrintf() 호출 하나를 항목 하나로 센다(줄바꿈 기준이 아니다).
  //   한 줄을 나눠 찍으면 log 덤프에서 줄 번호가 중간에 끼어 보인다.
  //
  if (ipcIsBooted())
  {
    logPrintf("Booting..CPU1  \t\t: %s (%d ms)\r\n",
              ipcGetStateStr(), (int)ipcGetBootTime());
    logPrintf("Booting..CPU1 Name\t: %s\r\n", ipcGetName());
    logPrintf("Booting..CPU1 Ver\t: %s\r\n", ipcGetVersion());
    logPrintf("Booting..CPU1 Clock\t: %d MHz\r\n", (int)(ipcGetClock() / 1000000));
  }
  else
  {
    logPrintf("Booting..CPU1  \t\t: %s\r\n", ipcGetStateStr());
  }

  logPrintf("\r\n");

  cliOpen(HW_UART_CH_CLI, 115200);

  return true;
}
