#include "hw_def.h"
#include "rtos.h"

#ifdef _USE_HW_RTOS

#include "log.h"
#include "indicator.h"


bool rtosInit(void)
{
  return true;
}

bool rtosIsRunning(void)
{
  return (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED);
}

/*
 * 하트비트를 아이들 태스크에서 돌린다.
 *
 * 아이들은 CPU 가 남을 때만 실행되므로 점멸이 곧 "여유가 있다" 는 표시가 된다.
 * 상위 스레드가 CPU 를 물면 깜빡임이 느려지거나 멈춘다. 터미널 없이 눈으로 보는
 * 지표다. 정확한 수치는 CLI 의 thread cpu 로 보지만, CPU 가 꽉 차면 CLI 스레드도
 * 같이 굶어서 명령이 안 먹는다. 그래서 둘 다 필요하다.
 *
 * 무엇을 어떻게 표시할지는 indicator 모듈이 정한다. 여기서는 주기만 잰다.
 * 아이들 훅은 절대 블로킹하면 안 된다.
 */
void vApplicationIdleHook(void)
{
  static TickType_t pre_tick = 0;

  TickType_t cur_tick = xTaskGetTickCount();

  if ((cur_tick - pre_tick) >= pdMS_TO_TICKS(500))
  {
    pre_tick = cur_tick;
    indicatorHeartbeat();
  }
}

/*
 * configSUPPORT_STATIC_ALLOCATION 을 켜면 커널이 idle / timer 태스크의 메모리를
 * 애플리케이션에서 받아 간다. 두 콜백을 주지 않으면 링크가 깨진다.
 * 정적 할당을 켜 두는 이유는 나중에 결정적인 버퍼가 필요할 때를 위해서다.
 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t  **ppxIdleTaskStackBuffer,
                                   uint32_t      *pulIdleTaskStackSize)
{
  static StaticTask_t idle_tcb;
  static StackType_t  idle_stack[configMINIMAL_STACK_SIZE];

  *ppxIdleTaskTCBBuffer   = &idle_tcb;
  *ppxIdleTaskStackBuffer = idle_stack;
  *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t  **ppxTimerTaskStackBuffer,
                                    uint32_t      *pulTimerTaskStackSize)
{
  static StaticTask_t timer_tcb;
  static StackType_t  timer_stack[configTIMER_TASK_STACK_DEPTH];

  *ppxTimerTaskTCBBuffer   = &timer_tcb;
  *ppxTimerTaskStackBuffer = timer_stack;
  *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}


//-- 아래 훅들은 조용히 넘어가면 안 되는 상황이다. 로그를 남기고 멈춘다.
//   디버거가 붙어 있으면 여기서 잡힌다.
//
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  (void)xTask;

  logPrintf("\r\nStackOverflow : %s\r\n", pcTaskName);

  if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
  {
    __BKPT(0);
  }
  while (1);
}

void vApplicationMallocFailedHook(void)
{
  logPrintf("\r\nMallocFailed\r\n");

  if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
  {
    __BKPT(0);
  }
  while (1);
}

void vAssertCalled(const char *p_file, int line)
{
  logPrintf("\r\nassert : %s:%d\r\n", p_file, line);

  if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
  {
    __BKPT(0);
  }
  __disable_irq();
  while (1);
}

#endif
