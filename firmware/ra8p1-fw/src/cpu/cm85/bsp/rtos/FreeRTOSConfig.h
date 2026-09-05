#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*
 * FreeRTOS 설정.
 *
 * RASC 는 커널 소스(ra/aws/FreeRTOS)와 포트(ra/fsp/src/rm_freertos_port)만 가져오고
 * 이 파일은 생성하지 않는다. 그래서 손으로 관리한다.
 *
 * 관련 문서: docs/22-freertos.md
 */

#ifndef __IASMARM__
 #include <stdint.h>
 #include "bsp_api.h"
extern uint32_t SystemCoreClock;
#endif


/*-----------------------------------------------------------
 * 스케줄러
 *----------------------------------------------------------*/
#define configUSE_PREEMPTION                      1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION   0
#define configUSE_TIME_SLICING                    1
#define configIDLE_SHOULD_YIELD                   1

#define configCPU_CLOCK_HZ                        (SystemCoreClock)
#define configTICK_RATE_HZ                        (1000)
#define configMAX_PRIORITIES                      (8)
#define configMINIMAL_STACK_SIZE                  (256)
#define configMAX_TASK_NAME_LEN                   (16)

//-- 둘 중 하나만 정의해야 한다. 새 이름을 쓴다.
#define configTICK_TYPE_WIDTH_IN_BITS             TICK_TYPE_WIDTH_32_BITS

/*
 * 틱리스 아이들은 켜지 않는다.
 *
 * micros() 가 SysTick->VAL 을 읽어 틱 사이를 보간하는데, 틱리스 아이들이 켜지면
 * SysTick 의 LOAD 값이 긴 슬립용으로 재설정돼서 그 계산이 깨진다. (bsp.c 참고)
 */
#define configUSE_TICKLESS_IDLE                   0

/*-----------------------------------------------------------
 * 메모리
 *----------------------------------------------------------*/
#define configSUPPORT_STATIC_ALLOCATION           1
#define configSUPPORT_DYNAMIC_ALLOCATION          1
#define configTOTAL_HEAP_SIZE                     (64 * 1024)
#define configAPPLICATION_ALLOCATED_HEAP          0

/*-----------------------------------------------------------
 * 훅
 *----------------------------------------------------------*/
#define configUSE_IDLE_HOOK                       1   // LED 하트비트 (rtos.c)
#define configUSE_TICK_HOOK                       0
#define configUSE_MALLOC_FAILED_HOOK              1
#define configCHECK_FOR_STACK_OVERFLOW            2
#define configUSE_DAEMON_TASK_STARTUP_HOOK        0

/*-----------------------------------------------------------
 * 기능
 *----------------------------------------------------------*/
#define configUSE_MUTEXES                         1
#define configUSE_RECURSIVE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES             1
#define configUSE_TASK_NOTIFICATIONS              1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES     1
#define configQUEUE_REGISTRY_SIZE                 8
#define configUSE_QUEUE_SETS                      0
#define configUSE_APPLICATION_TASK_TAG            0
#define configENABLE_BACKWARD_COMPATIBILITY       1
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS   0
#define configSTACK_DEPTH_TYPE                    uint32_t
#define configMESSAGE_BUFFER_LENGTH_TYPE          size_t

#define configUSE_TIMERS                          1
#define configTIMER_TASK_PRIORITY                 (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                  16
#define configTIMER_TASK_STACK_DEPTH              (512)

#define configUSE_TRACE_FACILITY                  1
#define configUSE_STATS_FORMATTING_FUNCTIONS      0

/*
 * 런타임 통계. thread cpu 명령이 태스크별 점유율을 내는 데 쓴다.
 *
 * 카운터 원천은 Cortex-M85 의 DWT 사이클 카운터다. 틱과 무관하게 단조 증가하고,
 * 태스크 전환 문맥에서 불려도 안전하다. (bsp.c 의 bspGetRunTimeCounter)
 */
#define configGENERATE_RUN_TIME_STATS             1
#ifndef __IASMARM__
void     bspRunTimeStatsInit(void);
uint32_t bspGetRunTimeCounter(void);
 #define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()  bspRunTimeStatsInit()
 #define portGET_RUN_TIME_COUNTER_VALUE()          bspGetRunTimeCounter()
#endif

/*-----------------------------------------------------------
 * assert
 *----------------------------------------------------------*/
#ifndef __IASMARM__
void vAssertCalled(const char *p_file, int line);
 #define configASSERT(x)    if ((x) == 0) { vAssertCalled(__FILE__, __LINE__); }
#endif

/*-----------------------------------------------------------
 * 인터럽트 우선순위
 *
 * RA8P1 의 __NVIC_PRIO_BITS 는 4 다(0~15, 숫자가 작을수록 높다).
 * FreeRTOS API 를 부르는 ISR 은 MAX_SYSCALL 보다 낮은(= 숫자가 큰) 우선순위여야 한다.
 * ra_gen 의 UART 인터럽트는 priority12 로 잡혀 있으므로 조건을 만족한다.
 *----------------------------------------------------------*/
#define configPRIO_BITS                             __NVIC_PRIO_BITS

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY     15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 2

#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/*-----------------------------------------------------------
 * 예외 핸들러 매핑
 *
 * SysTick 은 매핑하지 않는다. FSP 의 rm_freertos_port/port.c 가 SysTick_Handler 를
 * 직접 정의하기 때문이다. 여기서 또 매핑하면 중복 정의로 링크가 깨진다.
 * 같은 이유로 bsp.c 도 SysTick_Handler 를 정의하지 않는다.
 *----------------------------------------------------------*/
#define vPortSVCHandler                           SVC_Handler
#define xPortPendSVHandler                        PendSV_Handler

/*-----------------------------------------------------------
 * 쓰지 않는 API 는 빼서 코드 크기를 줄인다
 *----------------------------------------------------------*/
#define INCLUDE_vTaskPrioritySet                  1
#define INCLUDE_uxTaskPriorityGet                 1
#define INCLUDE_vTaskDelete                       1
#define INCLUDE_vTaskSuspend                      1
#define INCLUDE_vTaskDelayUntil                   1
#define INCLUDE_vTaskDelay                        1
#define INCLUDE_xTaskGetSchedulerState            1
#define INCLUDE_xTaskGetCurrentTaskHandle         1
#define INCLUDE_uxTaskGetStackHighWaterMark       1
#define INCLUDE_xTaskGetIdleTaskHandle            1
#define INCLUDE_eTaskGetState                     1
#define INCLUDE_xTimerPendFunctionCall            1
#define INCLUDE_xTaskAbortDelay                   1
#define INCLUDE_xQueueGetMutexHolder              1
#define INCLUDE_xSemaphoreGetMutexHolder          1

#endif /* FREERTOS_CONFIG_H */
