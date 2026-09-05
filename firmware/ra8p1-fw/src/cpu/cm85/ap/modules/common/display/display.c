#include "module.h"
#include "display.h"


//-- 채널 배정. 용도가 늘면 여기만 고친다.
//
#define DISPLAY_LED_STATUS    _DEF_LED1     // RED
#define DISPLAY_LED_LINK      _DEF_LED2     // GREEN
#define DISPLAY_LED_ACT       _DEF_LED3     // BLUE


static bool displayInit(void);
static bool displayEvent(event_t *p_evt);

#if CLI_USE(HW_DISPLAY)
static void cliDisplay(cli_args_t *args);
#endif


MODULE_DEF(display)
{
  .name     = "display",
  .priority = MODULE_PRI_HIGH,     // 다른 모듈이 상태를 바꾸기 전에 떠 있어야 한다
  .init     = displayInit
};


static const char *display_state_name[DISPLAY_STATE_MAX] =
{
  "BOOT", "RUN", "BUSY", "ERROR",
};

static DisplayState_t display_state = DISPLAY_STATE_BOOT;
static bool           is_link       = false;




bool displayInit(void)
{
  ledOff(DISPLAY_LED_STATUS);
  ledOff(DISPLAY_LED_LINK);
  ledOff(DISPLAY_LED_ACT);

  display_state = DISPLAY_STATE_RUN;

  //-- 이벤트를 구독한다. 발행하는 쪽은 LED 가 있는지도 모른다.
  //
  eventSub(displayEvent);

#if CLI_USE(HW_DISPLAY)
  cliAdd("display", cliDisplay);
#endif

  return true;
}

void displaySetState(DisplayState_t state)
{
  if (state >= DISPLAY_STATE_MAX) return;

  display_state = state;
}

DisplayState_t displayGetState(void)
{
  return display_state;
}

void displayHeartbeat(void)
{
  //-- 오류일 때는 켜 둔다. 깜빡이면 정상 동작과 구분이 안 된다.
  //
  if (display_state == DISPLAY_STATE_ERROR)
  {
    ledOn(DISPLAY_LED_STATUS);
    return;
  }

  ledToggle(DISPLAY_LED_STATUS);
}

/*
 * 이벤트 구독자.
 *
 * 여기에 case 를 늘리는 것만으로 새 표시를 붙일 수 있다. 이벤트를 던지는 쪽은
 * 고치지 않는다.
 */
bool displayEvent(event_t *p_evt)
{
  switch (p_evt->code)
  {
    case EVENT_ETH_LINK:
      is_link = (p_evt->data != 0);
      if (is_link) ledOn(DISPLAY_LED_LINK);
      else         ledOff(DISPLAY_LED_LINK);
      break;

    case EVENT_ERROR:
      displaySetState(DISPLAY_STATE_ERROR);
      break;

    case EVENT_BOOT_DONE:
      displaySetState(DISPLAY_STATE_RUN);
      break;

    default:
      break;
  }

  return true;
}


#if CLI_USE(HW_DISPLAY)
void cliDisplay(cli_args_t *args)
{
  bool ret = false;

  if (args->argc == 1 && args->isStr(0, "info"))
  {
    cliPrintf("state : %s\n", display_state_name[display_state]);
    cliPrintf("link  : %s\n", is_link ? "up" : "down");
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "state"))
  {
    for (int i = 0; i < DISPLAY_STATE_MAX; i++)
    {
      if (args->isStr(1, display_state_name[i]))
      {
        displaySetState((DisplayState_t)i);
        cliPrintf("state -> %s\n", display_state_name[i]);
        ret = true;
        break;
      }
    }
  }

  if (ret == false)
  {
    cliPrintf("display info\n");
    cliPrintf("display state [");
    for (int i = 0; i < DISPLAY_STATE_MAX; i++)
    {
      cliPrintf("%s%s", display_state_name[i], (i < DISPLAY_STATE_MAX - 1) ? "|" : "");
    }
    cliPrintf("]\n");
    cliPrintf("\n링크 표시는 event pub %d 1 로 시험한다\n", EVENT_ETH_LINK);
  }
}
#endif
