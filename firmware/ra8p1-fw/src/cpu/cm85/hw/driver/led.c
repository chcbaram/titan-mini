#include "led.h"


#ifdef _USE_HW_LED


typedef struct
{
  bsp_io_port_pin_t pin;
  bsp_io_level_t    on_state;
  bsp_io_level_t    off_state;
} led_tbl_t;

static const led_tbl_t led_tbl[LED_MAX_CH] =
{
  {BSP_IO_PORT_01_PIN_09, BSP_IO_LEVEL_LOW, BSP_IO_LEVEL_HIGH},   // LED3 RED
  {BSP_IO_PORT_01_PIN_08, BSP_IO_LEVEL_LOW, BSP_IO_LEVEL_HIGH},   // LED3 GREEN
  {BSP_IO_PORT_01_PIN_10, BSP_IO_LEVEL_LOW, BSP_IO_LEVEL_HIGH},   // LED3 BLUE
};


bool ledInit(void)
{
  //-- 핀 방향/초기 레벨은 ra_gen/pin_data.c 에 있고
  //   R_BSP_WarmStart(POST_C) 의 R_IOPORT_Open() 이 이미 적용했다.
  //
  for (int i = 0; i < LED_MAX_CH; i++)
  {
    ledOff(i);
  }

  return true;
}

void ledOn(uint8_t ch)
{
  if (ch >= LED_MAX_CH) return;

  g_ioport.p_api->pinWrite(g_ioport.p_ctrl, led_tbl[ch].pin, led_tbl[ch].on_state);
}

void ledOff(uint8_t ch)
{
  if (ch >= LED_MAX_CH) return;

  g_ioport.p_api->pinWrite(g_ioport.p_ctrl, led_tbl[ch].pin, led_tbl[ch].off_state);
}

void ledToggle(uint8_t ch)
{
  if (ch >= LED_MAX_CH) return;

  bsp_io_level_t pin_level;

  g_ioport.p_api->pinRead(g_ioport.p_ctrl, led_tbl[ch].pin, &pin_level);

  if (pin_level == led_tbl[ch].on_state)
  {
    ledOff(ch);
  }
  else
  {
    ledOn(ch);
  }
}

#endif
