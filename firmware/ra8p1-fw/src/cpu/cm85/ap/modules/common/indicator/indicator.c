#include "module.h"
#include "indicator.h"


/*
 * 표시 정책
 *
 *   상태          색      패턴        의미
 *   BOOT          파랑    켜짐        초기화 중
 *   RUN  링크X    빨강    500ms 점멸  정상. 네트워크 없음
 *   RUN  링크O    초록    500ms 점멸  정상. 네트워크 연결됨
 *   BUSY          파랑    500ms 점멸  오래 걸리는 작업 중
 *   ERROR         빨강    켜짐        오류. 깜빡이면 정상과 구분이 안 된다
 *
 * 직렬 저항이 색마다 달라(R 2KΩ / G 12KΩ / B 10KΩ) 빨강이 가장 밝다.
 * 밝기를 맞추려면 PWM 이 필요한데 지금은 그럴 이유가 없다.
 */
typedef enum
{
  COLOR_OFF = 0,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE,
} Color_t;


static bool indicatorInit(void);
static bool indicatorEvent(event_t *p_evt);
static void indicatorRender(Color_t color, bool is_on);

#if CLI_USE(HW_INDICATOR)
static void cliIndicator(cli_args_t *args);
#endif


MODULE_DEF(indicator)
{
  .name     = "indicator",
  .priority = MODULE_PRI_HIGH,     // 다른 모듈이 상태를 바꾸기 전에 떠 있어야 한다
  .init     = indicatorInit,
  .event_cb = indicatorEvent,      // moduleInit() 이 자동으로 구독시킨다
};


static const char *indicator_state_name[INDICATOR_STATE_MAX] =
{
  "BOOT", "RUN", "BUSY", "ERROR",
};

static const uint8_t color_led_ch[] =
{
  [COLOR_RED]   = _DEF_LED1,
  [COLOR_GREEN] = _DEF_LED2,
  [COLOR_BLUE]  = _DEF_LED3,
};

static IndicatorState_t indicator_state = INDICATOR_STATE_BOOT;
static bool             is_link         = false;
static bool             is_on           = false;




bool indicatorInit(void)
{
  indicator_state = INDICATOR_STATE_BOOT;
  indicatorRender(COLOR_BLUE, true);

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

/*
 * 지금 무슨 색을 보여야 하는가.
 *
 * 위에서부터 우선한다. 오류가 링크 상태보다 급하다.
 */
static Color_t indicatorPickColor(void)
{
  switch (indicator_state)
  {
    case INDICATOR_STATE_ERROR: return COLOR_RED;
    case INDICATOR_STATE_BOOT:  return COLOR_BLUE;
    case INDICATOR_STATE_BUSY:  return COLOR_BLUE;
    case INDICATOR_STATE_RUN:   return is_link ? COLOR_GREEN : COLOR_RED;
    default:                    return COLOR_OFF;
  }
}

/*
 * RGB 가 한 패키지라 두 채널을 동시에 켜면 색이 섞인다.
 * 요청한 색 말고는 반드시 끈다.
 */
void indicatorRender(Color_t color, bool on)
{
  for (int c = COLOR_RED; c <= COLOR_BLUE; c++)
  {
    if (c == color && on)
    {
      ledOn(color_led_ch[c]);
    }
    else
    {
      ledOff(color_led_ch[c]);
    }
  }
}

void indicatorHeartbeat(void)
{
  Color_t color = indicatorPickColor();

  //-- 오류와 부팅 중에는 켜 둔다. 깜빡이면 정상 동작과 구분이 안 된다.
  //
  if (indicator_state == INDICATOR_STATE_ERROR ||
      indicator_state == INDICATOR_STATE_BOOT)
  {
    is_on = true;
  }
  else
  {
    is_on = !is_on;
  }

  indicatorRender(color, is_on);
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
    const char *color_name[] = {"OFF", "RED", "GREEN", "BLUE"};

    cliPrintf("state : %s\n", indicator_state_name[indicator_state]);
    cliPrintf("link  : %s\n", is_link ? "up" : "down");
    cliPrintf("color : %s\n", color_name[indicatorPickColor()]);
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
