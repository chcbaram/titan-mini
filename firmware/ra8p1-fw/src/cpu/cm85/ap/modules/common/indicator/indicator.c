#include "module.h"
#include "indicator.h"


//-- 채널 배정. 용도가 늘면 여기만 고친다.
//
#define INDICATOR_LED_STATUS    _DEF_LED1     // RED
#define INDICATOR_LED_LINK      _DEF_LED2     // GREEN
#define INDICATOR_LED_ACT       _DEF_LED3     // BLUE


static bool indicatorInit(void);
static bool indicatorEvent(event_t *p_evt);

#if CLI_USE(HW_INDICATOR)
static void cliIndicator(cli_args_t *args);
#endif


MODULE_DEF(indicator)
{
  .name     = "indicator",
  .priority = MODULE_PRI_HIGH,     // 다른 모듈이 상태를 바꾸기 전에 떠 있어야 한다
  .init     = indicatorInit,
  .event_cb = indicatorEvent,        // moduleInit() 이 자동으로 구독시킨다
};


static const char *indicator_state_name[INDICATOR_STATE_MAX] =
{
  "BOOT", "RUN", "BUSY", "ERROR",
};

static IndicatorState_t indicator_state = INDICATOR_STATE_BOOT;
static bool           is_link       = false;




bool indicatorInit(void)
{
  ledOff(INDICATOR_LED_STATUS);
  ledOff(INDICATOR_LED_LINK);
  ledOff(INDICATOR_LED_ACT);

  indicator_state = INDICATOR_STATE_RUN;

#if CLI_USE(HW_INDICATOR)
  cliAdd("indicator", cliIndicator);
#endif

  return true;
}

void indicatorSetState(IndicatorState_t state)
{
  if (state >= INDICATOR_STATE_MAX) return;

  indicator_state = state;
}

IndicatorState_t indicatorGetState(void)
{
  return indicator_state;
}

void indicatorHeartbeat(void)
{
  //-- 오류일 때는 켜 둔다. 깜빡이면 정상 동작과 구분이 안 된다.
  //
  if (indicator_state == INDICATOR_STATE_ERROR)
  {
    ledOn(INDICATOR_LED_STATUS);
    return;
  }

  ledToggle(INDICATOR_LED_STATUS);
}

/*
 * 이벤트 구독자.
 *
 * 여기에 case 를 늘리는 것만으로 새 표시를 붙일 수 있다. 이벤트를 던지는 쪽은
 * 고치지 않는다.
 */
bool indicatorEvent(event_t *p_evt)
{
  switch (p_evt->code)
  {
    case EVENT_ETH_LINK:
      is_link = (p_evt->data != 0);
      if (is_link) ledOn(INDICATOR_LED_LINK);
      else         ledOff(INDICATOR_LED_LINK);
      break;

    case EVENT_ERROR:
      indicatorSetState(INDICATOR_STATE_ERROR);
      break;

    case EVENT_BOOT_DONE:
      indicatorSetState(INDICATOR_STATE_RUN);
      break;

    default:
      break;
  }

  return true;
}


#if CLI_USE(HW_INDICATOR)
void cliIndicator(cli_args_t *args)
{
  bool ret = false;

  if (args->argc == 1 && args->isStr(0, "info"))
  {
    cliPrintf("state : %s\n", indicator_state_name[indicator_state]);
    cliPrintf("link  : %s\n", is_link ? "up" : "down");
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "state"))
  {
    for (int i = 0; i < INDICATOR_STATE_MAX; i++)
    {
      if (args->isStr(1, indicator_state_name[i]))
      {
        indicatorSetState((IndicatorState_t)i);
        cliPrintf("state -> %s\n", indicator_state_name[i]);
        ret = true;
        break;
      }
    }
  }

  if (ret == false)
  {
    cliPrintf("indicator info\n");
    cliPrintf("indicator state [");
    for (int i = 0; i < INDICATOR_STATE_MAX; i++)
    {
      cliPrintf("%s%s", indicator_state_name[i], (i < INDICATOR_STATE_MAX - 1) ? "|" : "");
    }
    cliPrintf("]\n");
    cliPrintf("\n링크 표시는 event pub %d 1 로 시험한다\n", EVENT_ETH_LINK);
  }
}
#endif
