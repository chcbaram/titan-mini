#include "module.h"


static bool cliModuleInit(void);
static void cliThread(void *arg);

static uint8_t  cli_ch   = HW_UART_CH_CLI;
static uint32_t cli_baud = 115200;


MODULE_DEF(cli)
{
  .name     = "cli",
  .priority = MODULE_PRI_NORMAL,
  .init     = cliModuleInit
};




bool cliModuleInit(void)
{
  cliOpen(cli_ch, cli_baud);

  return (threadCreate("cli", cliThread, NULL,
                       _HW_DEF_THREAD_CLI_PRI, _HW_DEF_THREAD_CLI_STACK) >= 0);
}

void cliThread(void *arg)
{
  (void)arg;

  logPrintf("[  ] Thread Started : cli\r\n");

  while (1)
  {
    cliMain();

    //-- CDC / 텔넷 같은 다른 채널이 붙으면 여기서 우선순위를 정해 전환한다.
    //   지금은 UART 하나뿐이다. (docs/22-freertos.md)

    delay(2);
  }
}
