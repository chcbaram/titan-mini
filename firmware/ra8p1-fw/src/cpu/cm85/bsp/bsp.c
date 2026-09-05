#include "bsp.h"
#include "hw_def.h"


volatile uint32_t systick_ms = 0;




bool bspInit(void)
{
  //-- Cortex-M85 의 I/D 캐시.
  //   SDRAM / DMA / 코어간 공유 버퍼를 쓰기 시작하면 유지보수 규칙이 필요해지므로
  //   그때까지는 스위치로 막아둔다. (docs/04-dualcore.md)
  //
  #ifdef _USE_HW_CACHE
  SCB_EnableICache();
  SCB_EnableDCache();
  #endif

  //-- FSP 진입점. 핀 설정은 이 앞의 R_BSP_WarmStart(POST_C) 에서 이미 끝나 있다.
  //
  hal_entry();

  SysTick_Config(SystemCoreClock / 1000);   // 1ms

  return true;
}

void SysTick_Handler(void)
{
  systick_ms++;
}

void delay(uint32_t ms)
{
  uint32_t pre_time = systick_ms;

  while (systick_ms - pre_time < ms);
}

uint32_t millis(void)
{
  return systick_ms;
}

uint32_t micros(void)
{
  uint32_t      m0 = millis();
  __IO uint32_t u0 = SysTick->VAL;
  uint32_t      m1 = millis();
  __IO uint32_t u1 = SysTick->VAL;

  const uint32_t tms = SysTick->LOAD + 1;

  if (m1 != m0)
  {
    return (m1 * 1000 + ((tms - u1) * 1000) / tms);
  }
  else
  {
    return (m0 * 1000 + ((tms - u0) * 1000) / tms);
  }
}

void Error_Handler(void)
{
  if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
  {
    __BKPT(0);
  }

  __disable_irq();
  while (1)
  {
  }
}
