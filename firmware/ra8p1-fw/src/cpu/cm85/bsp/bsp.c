#include "bsp.h"
#include "hw_def.h"

#ifdef _USE_HW_RTOS
#include "rtos.h"
#endif


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

  //-- SysTick 은 여기서 건드리지 않는다.
  //   FSP 의 rm_freertos_port/port.c 가 SysTick_Handler 를 직접 정의하고,
  //   vTaskStartScheduler() 안에서 SysTick 을 설정한다. 여기서 미리 인터럽트를
  //   켜면 스케줄러가 없는 상태로 FreeRTOS 틱 처리가 돌아 죽는다.
  //   (docs/22-freertos.md)
  //
  #ifndef _USE_HW_RTOS
  SysTick_Config(SystemCoreClock / 1000);   // 1ms
  #endif

  return true;
}

#ifndef _USE_HW_RTOS
void SysTick_Handler(void)
{
  systick_ms++;
}
#endif

void delay(uint32_t ms)
{
#ifdef _USE_HW_RTOS
  if (rtosIsRunning())
  {
    vTaskDelay(pdMS_TO_TICKS(ms));
    return;
  }

  //-- 스케줄러 전에는 틱이 없다. FSP 의 사이클 루프로 기다린다.
  //
  R_BSP_SoftwareDelay(ms, BSP_DELAY_UNITS_MILLISECONDS);
#else
  uint32_t pre_time = systick_ms;

  while (systick_ms - pre_time < ms);
#endif
}

void delayUs(uint32_t us)
{
  R_BSP_SoftwareDelay(us, BSP_DELAY_UNITS_MICROSECONDS);
}

uint32_t millis(void)
{
#ifdef _USE_HW_RTOS
  //-- configTICK_RATE_HZ 가 1000 이라 틱이 곧 ms 다.
  //   main.c 가 스케줄러를 먼저 띄우고 그 안에서 hwInit/apInit/apMain 을 부르므로,
  //   드라이버가 보는 millis() 는 항상 유효하다.
  //
  return (uint32_t)xTaskGetTickCount();
#else
  return systick_ms;
#endif
}

uint32_t micros(void)
{
  //-- 틱 사이를 SysTick 카운터로 보간한다.
  //   틱리스 아이들을 켜면 SysTick 의 LOAD 가 긴 슬립용으로 바뀌어 이 계산이
  //   깨진다. FreeRTOSConfig.h 에서 configUSE_TICKLESS_IDLE 을 0 으로 둔 이유다.
  //
  uint32_t      m0 = millis();
  __IO uint32_t u0 = SysTick->VAL;
  uint32_t      m1 = millis();
  __IO uint32_t u1 = SysTick->VAL;

  const uint32_t tms = SysTick->LOAD + 1;

  if (tms <= 1) return m0 * 1000;      // SysTick 이 아직 안 돌고 있다

  if (m1 != m0)
  {
    return (m1 * 1000 + ((tms - u1) * 1000) / tms);
  }
  else
  {
    return (m0 * 1000 + ((tms - u0) * 1000) / tms);
  }
}

#ifdef _USE_HW_RTOS
/*
 * FreeRTOS 런타임 통계용 카운터.
 *
 * DWT 사이클 카운터를 8비트 내려서 쓴다. 1 GHz 에서 약 3.9 MHz 가 되어
 * 32비트가 한 바퀴 도는 데 약 1100 초다. 태스크별 누적값의 차이를 1 초 창으로
 * 보기 때문에 이 정도면 충분하다.
 *
 * micros() 를 쓰지 않는 이유는, 이 함수가 태스크 전환 문맥에서 불리는데
 * micros() 는 내부에서 xTaskGetTickCount() 를 부르기 때문이다.
 */
void bspRunTimeStatsInit(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

  DWT->CYCCNT = 0;
  DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t bspGetRunTimeCounter(void)
{
  return DWT->CYCCNT >> 8;
}
#endif

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
