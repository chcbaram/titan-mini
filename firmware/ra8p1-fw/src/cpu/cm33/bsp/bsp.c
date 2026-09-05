#include "bsp.h"
#include "hw_def.h"


volatile uint32_t systick_ms = 0;




bool bspInit(void)
{
  //-- CPU1 은 클럭을 설정하지 않는다. CPU0 이 잡아 둔 것을 그대로 쓴다.
  //   FSP 의 system.c 가 BSP_SECONDARY_CORE_BUILD 로 그 분기를 처리한다.
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
  R_BSP_SoftwareDelay(ms, BSP_DELAY_UNITS_MILLISECONDS);
}

void delayUs(uint32_t us)
{
  R_BSP_SoftwareDelay(us, BSP_DELAY_UNITS_MICROSECONDS);
}

uint32_t millis(void)
{
  return systick_ms;
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
