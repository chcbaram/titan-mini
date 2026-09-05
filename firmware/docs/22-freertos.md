# 22. FreeRTOS

> 커널을 올리고 스레드 OSAL 을 붙인다. 이후 FatFs · lwIP · USB 스택 · LVGL 이 전부 RTOS 를 전제한다.
> 관련: [21-uart-cli.md](21-uart-cli.md) · [11-fsp-config.md](11-fsp-config.md) · [04-dualcore.md](04-dualcore.md)
>
> **상태: 완료.** 스케줄러 기동, `main` / `cli` 두 스레드 동작, `thread info` 확인.

---

## 1. 왜 지금인가

FatFs · lwIP · USB 스택 · LVGL 이 전부 RTOS 를 전제한다. 늦게 넣으면 이미 만든 드라이버를 전부 스레드 안전하게 다시 손봐야 한다. 드라이버가 두 개(LED, UART)뿐인 지금이 가장 싸다.

## 2. 커널은 RASC 가 가져온다

FSP 는 **포트 레이어만** 제공한다(`ra/fsp/src/rm_freertos_port/{port.c, portmacro.h}`). 커널은 별도 팩(`Amazon.FreeRTOS-Kernel.11.1.0+fsp.6.6.0`)에서 온다.

`configuration.xml` 에 세 가지를 넣고 재생성했다.

```xml
<option key="#RTOS#" value="FreeRTOS" />

<component class="RTOS"  group="FreeRTOS" subgroup="all"    vendor="AWS" version="11.1.0+fsp.6.6.0"/>
<component class="Heaps" group="FreeRTOS" subgroup="heap_4" vendor="AWS" version="11.1.0+fsp.6.6.0"/>

<config id="config.awsfreertos.thread"> ... </config>
```

결과:

```
ra/aws/FreeRTOS/FreeRTOS/Source/        커널 (tasks.c, queue.c, timers.c, ...)
ra/aws/FreeRTOS/FreeRTOS/Source/portable/MemMang/heap_4.c
ra/fsp/src/rm_freertos_port/            포트 (Cortex-M85)
```

heap 은 **heap_4**(병합 가능한 free 리스트)를 골랐다. 예제는 heap_2 지만 그건 free 블록을 병합하지 않아 장기 실행에서 단편화가 쌓인다.

> ### FreeRTOSConfig.h 는 RASC 가 만들지 않는다
>
> `config.awsfreertos.thread` 에 온갖 설정을 넣어도 **`FreeRTOSConfig.h` 는 생성되지 않는다.**
> 직접 관리해야 한다. `src/cpu/cm85/bsp/rtos/FreeRTOSConfig.h` 에 뒀다 — 기존 프로젝트의
> `src/bsp/rtos/` 관례와 같은 자리다.
>
> 오히려 잘 됐다. 아래 SysTick 문제 때문에 이 파일을 우리가 통제해야 한다.

## 3. SysTick 은 FSP 포트가 소유한다

`rm_freertos_port/port.c` 가 **`SysTick_Handler` 를 직접 정의한다.**

```c
/* port.c:858 */
void SysTick_Handler (void)
{
    ...
}
```

FreeRTOS 의 보통 Cortex-M 포트처럼 `FreeRTOSConfig.h` 의 `#define xPortSysTickHandler SysTick_Handler` 로 매핑하는 방식이 아니다. 그래서

- **`FreeRTOSConfig.h` 에서 `xPortSysTickHandler` 를 매핑하면 안 된다.** 중복 정의로 링크가 깨진다.
- **`bsp.c` 도 `SysTick_Handler` 를 정의할 수 없다.**

`SVC` 와 `PendSV` 만 매핑한다.

```c
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
// SysTick 은 매핑하지 않는다 — port.c 가 직접 정의한다
```

`bspInit()` 에서도 SysTick 을 건드리지 않는다. 스케줄러가 없는 상태로 FreeRTOS 틱 처리가 돌면 죽는다.

```c
#ifndef _USE_HW_RTOS
SysTick_Config(SystemCoreClock / 1000);
#endif
```

### 그래서 millis / delay 가 바뀐다

| 함수 | RTOS 켜기 전 | 지금 |
|---|---|---|
| `millis()` | `systick_ms` (자체 SysTick ISR) | `xTaskGetTickCount()` — `configTICK_RATE_HZ` 가 1000 이라 틱이 곧 ms |
| `delay()` | `systick_ms` 폴링 | 스케줄러 중이면 `vTaskDelay()`, 아니면 `R_BSP_SoftwareDelay()` |
| `delayUs()` | — | `R_BSP_SoftwareDelay(us, MICROSECONDS)` |
| `micros()` | SysTick 보간 | 그대로. 단 **틱리스 아이들을 켜면 깨진다** |

> ### 스케줄러 밖에서는 millis() 가 0 이다
>
> `xTaskGetTickCount()` 는 스케줄러가 돌기 전에 0 을 돌려준다. 그래서 **초기화를 전부
> 스케줄러 안으로 옮겼다**(아래 5장). `bspInit()` 만 밖에서 돌고, 거기서는 시간을 쓰지 않는다.
>
> `delay()` 는 스케줄러 밖에서 `R_BSP_SoftwareDelay()`(사이클 루프)로 떨어지므로 언제든 동작한다.

`micros()` 가 `SysTick->VAL` 로 틱 사이를 보간하므로 **`configUSE_TICKLESS_IDLE` 은 0 으로 둔다.** 틱리스가 켜지면 긴 슬립을 위해 `SysTick->LOAD` 가 재설정돼 보간이 틀어진다.

## 4. 헤더 순서 — bsp.h 가 rtos.h 를 무조건 include 한다

`hw_def.h` 는 맨 위에서 `bsp.h` 를 include 한 **뒤에** `_USE_HW_RTOS` 를 정의한다. 그래서 `bsp.h` 안에서 그 매크로로 FreeRTOS 헤더를 조건부 include 하면 항상 빠진다. `log.c` 가 `SemaphoreHandle_t` 를 못 찾는 식으로 터진다.

```
hw_def.h → bsp.h → (여기서 _USE_HW_RTOS 는 아직 없다)
         → #define _USE_HW_RTOS
```

`bsp.h` 가 `rtos.h` 를 **무조건** include 하고, `rtos.h` 가 FreeRTOS 헤더를 조건 없이 끌어온다. C5A3ZG 도 같은 구조다.

## 5. 스케줄러부터 띄우고 그 안에서 초기화한다

`main.c` 는 `bspInit()` 만 하고 곧바로 `main` 스레드를 만들어 스케줄러를 시작한다.
**`hwInit` / `apInit` / `apMain` 이 전부 스케줄러 아래에서 돈다.**

```c
int main(void)
{
  bspInit();

  if (xTaskCreate(mainThread, "main", ..., NULL) != pdPASS)
  {
    ledInit();
    while (1) { /* 힙 부족. 로그도 없으니 LED 로 알린다 */ }
  }

  vTaskStartScheduler();
  return 0;
}

void mainThread(void *arg)
{
  hwInit();
  apInit();
  apMain();
}
```

이렇게 하면 **드라이버가 보는 `millis()` 와 `delay()` 가 처음부터 정상**이다. 초기화를
스케줄러 밖에서 하면 `xTaskGetTickCount()` 가 0 이라 시간 관련 코드가 전부 특수 케이스를
타야 한다. C5A3ZG 도 같은 구조다.

## 6. 스레드 OSAL

`src/common/hw/include/osal/thread.h` + `src/cpu/cm85/hw/driver/osal/thread.c`.

**등록과 생성을 분리한다.** `threadCreate()` 는 테이블에 담아만 두고 `threadBegin()` 이
한꺼번에 만든다. 모듈들이 `apInit()` 단계에서 자기 스레드를 등록할 수 있게 하려는 것이다.
스케줄러는 `main.c` 가 이미 돌리고 있으므로 `threadBegin()` 은 생성만 한다.

스택 인자는 **바이트**로 받아 내부에서 워드로 나눈다. `xTaskCreate` 는 워드 단위라 헷갈리기 쉽다.

```c
xTaskCreate(..., thread_tbl[i].stack_bytes / sizeof(StackType_t), ...);
```

## 7. 모듈 구조

기능 하나가 파일 하나로 자기를 등록한다. `ap/modules/` 아래에 파일을 두고 `MODULE_DEF()` 를
쓰면 끝이다. 목록을 따로 관리하지 않는다.

```c
// ap/modules/common/cli/cli.c
MODULE_DEF(cli)
{
  .name     = "cli",
  .priority = MODULE_PRI_NORMAL,
  .init     = cliModuleInit
};

static bool cliModuleInit(void)
{
  cliOpen(cli_ch, cli_baud);
  return (threadCreate("cli", cliThread, NULL,
                       _HW_DEF_THREAD_CLI_PRI, _HW_DEF_THREAD_CLI_STACK) >= 0);
}
```

모듈 디스크립터는 네 가지를 선언할 수 있다.

| 필드 | 언제 불리나 |
|---|---|
| `init` | `moduleInit()` 이 **우선순위 순으로** 한 번 |
| `update` | `moduleUpdate()` 가 등록 순으로 주기적으로. 스레드를 만들 만큼 무겁지 않은 일 |
| `arg` | `update` 에 그대로 넘긴다 |
| `event_cb` | 이벤트 구독자. **`init()` 이 성공하면 자동으로 등록된다** |

`event_cb` 를 디스크립터에 두면 모듈이 `eventSub()` 를 직접 부르지 않아도 된다.
초기화에 실패한 모듈은 구독자로 올리지 않는다 — 준비 안 된 상태를 이벤트가 건드리면 안 된다.

```
[  ] moduleInit()
       count : 2
       indicator        OK
       cli              OK
[  ] Thread Started : cli

cli# module info
count : 2
idx  name             pri  update event
0    cli              6    -      -
1    indicator        1    -      yes
```

### 링커 스크립트를 우리가 소유한다

디스크립터는 `.module` 섹션에 들어간다. 그런데 RASC 가 만드는 `script/fsp.ld` 에 이 섹션을
넣으면 **재생성할 때마다 날아간다.** 그래서 우리 링커 스크립트를 따로 두고 생성물 두 개를
`INCLUDE` 한다.

```
/* src/cpu/cm85/bsp/ldscript/titan-mini-cm85.ld */
INCLUDE memory_regions.ld
INCLUDE fsp_gen.ld

SECTIONS
{
  .module : ALIGN(4)
  {
    _smodule = .;
    KEEP (*(.module))
    KEEP (*(.module*))
    _emodule = .;
    . = ALIGN(4);
  } > FLASH
}
```

CMake 의 `-T` 를 이쪽으로 돌리고, `INCLUDE` 가 찾을 수 있게 `ra_sdk` 루트를 `-L` 로 넘긴다.
앞으로 `.version` 이나 코어간 non-cacheable 영역 같은 커스텀 섹션도 전부 여기에 넣는다.

> ### 디스크립터는 const 여야 한다
>
> `MODULE_DEF` 가 `const` 로 선언하는 이유는 FLASH 에 그대로 두기 위해서다.
> RAM 으로 복사되는 섹션에 넣으면 FSP 의 `SystemRuntimeInit()` 복사 테이블
> (`bsp_init_copy_info_t`, `fsp_gen.ld` 가 만든다)에 들어가지 않아 **부팅 시 쓰레기가 남는다.**
> 참조 프로젝트는 `volatile` 로 선언하고 `.data` 안에 섹션을 넣어 해결했지만,
> 우리는 그 출력 섹션을 손댈 수 없다.

### 표시는 indicator 모듈이, 주기는 아이들 훅이

LED 를 어디서 깜빡일지에 세 가지 후보가 있었다.

| 방식 | CPU 여유를 보여주나 | 비용 |
|---|---|---|
| `apMain()` 의 while 루프 | ✗ main 스레드(우선순위 2)가 도는 것만 보여준다 | 없음 |
| 전용 heartbeat 스레드 (우선순위 1) | ✓ | 스택 + TCB |
| **FreeRTOS 아이들 훅** | ✓ 아이들은 CPU 가 남을 때만 실행된다 | **없음** |

아이들 훅으로 갔다. 상위 스레드가 CPU 를 물면 깜빡임이 느려지거나 멈춘다 — 그게 정확히 보고 싶은 신호다.

```c
void vApplicationIdleHook(void)
{
  static TickType_t pre_tick = 0;
  TickType_t cur_tick = xTaskGetTickCount();

  if ((cur_tick - pre_tick) >= pdMS_TO_TICKS(500))
  {
    pre_tick = cur_tick;
    indicatorHeartbeat();      // 무엇을 표시할지는 indicator 모듈이 정한다
  }
}
```

**아이들 훅은 절대 블로킹하면 안 된다.** 여기서는 틱만 비교한다.

CLI 의 `thread cpu` 로 정확한 수치를 볼 수 있지만 **CPU 가 꽉 차면 CLI 스레드도 같이 굶어서 명령 자체가 안 먹는다.** 그래서 둘 다 필요하다 — LED 는 무언가 잘못됐다는 것을, `thread cpu` 는 무엇이 잘못됐는지를 알려준다.

## 8. 이벤트 버스 — 발행하는 쪽이 LED 를 몰라야 한다

"이더넷 링크가 붙으면 LED 를 켠다" 를 이더넷 드라이버가 직접 하면, 드라이버가 LED 를 알게 되고 표시를 바꿀 때마다 드라이버를 고치게 된다. 그래서 이벤트를 사이에 둔다.

```c
// 이더넷 드라이버는 이것만 한다. LED 가 있는지도 모른다.
eventPub(EVENT_ETH_LINK, is_up);
```

```c
// indicator 모듈이 구독해서 표시를 정한다. 디스크립터에 event_cb 만 적으면
// moduleInit() 이 자동으로 등록한다.
MODULE_DEF(indicator)
{
  .name     = "indicator",
  .priority = MODULE_PRI_HIGH,
  .init     = indicatorInit,
  .event_cb = indicatorEvent,
};

static bool indicatorEvent(event_t *p_evt)
{
  switch (p_evt->code)
  {
    case EVENT_ETH_LINK:
      is_link = (p_evt->data != 0);
      if (is_link) ledOn(INDICATOR_LED_LINK);
      else         ledOff(INDICATOR_LED_LINK);
      break;

    case EVENT_ERROR:     indicatorSetState(INDICATOR_STATE_ERROR); break;
    case EVENT_BOOT_DONE: indicatorSetState(INDICATOR_STATE_RUN);   break;
    default: break;
  }
  return true;
}
```

표시를 바꾸려면 이 `switch` 에 `case` 를 늘리면 된다. 이벤트를 던지는 쪽은 고치지 않는다.

**구조**

| 파일 | 역할 |
|---|---|
| `common/evt_code.h` | 이벤트 코드 목록 (`EventCode_t`) |
| `common/hw/include/event.h` | API |
| `cpu/cm85/hw/driver/event.c` | qbuffer 기반 큐 + 구독자 목록 |

발행은 큐에 넣기만 하고, 뿌리는 것은 `eventUpdate()` 가 스레드 문맥에서 한다. **ISR 에서 발행해도 되지만 뿌리는 것은 항상 스레드 문맥이라**, 구독자가 로그를 찍거나 블로킹해도 안전하다.

> **전용 이벤트 스레드는 두지 않는다.** `stm32h5-bd`(2025-07)에는 있었지만, 그보다 최신인
> `weact-h750-mini`(2026-08-30)와 `stm32h5-w6300` 은 `eventUpdate()` 를 기존 루프에서 부른다.
> 이벤트 처리는 대부분 짧고, 스레드를 늘리면 스택과 컨텍스트 스위치만 늘어난다.

여기서는 `main` 스레드의 루프에서 10 ms 마다 부른다.

```c
while (1)
{
  eventUpdate();     // 큐에 쌓인 이벤트를 구독자에게 뿌린다
  moduleUpdate();    // update() 를 등록한 모듈들
  delay(10);
}
```

`eventPub` / `eventSub` 는 매크로다. 코드 이름과 함수 이름을 문자열로 같이 넘겨서 로그와 CLI 에 숫자 대신 이름이 보인다.

```
[  ] Event EVENT_BOOT_DONE : 0

cli# event info
sub   : 1 / 8
queue : 0 / 16
pub   : 1
drop  : 0

0 : indicatorEvent
```

큐가 넘치면 조용히 버리지 않고 `drop` 을 센다.

CLI 로 이벤트를 흉내 낼 수 있어서 하드웨어 없이 구독자를 시험할 수 있다.

```
cli# event pub 3 1        # EVENT_ETH_LINK, up
[  ] Event cli : 1
cli# indicator info
state : RUN
link  : up
```

### RGB LED 채널 배정

용도별로 채널을 나눠 두면 표시가 겹치지 않는다.

| 채널 | 용도 |
|---|---|
| LED1 (RED) | 상태 하트비트 |
| LED2 (GREEN) | 네트워크 링크 |
| LED3 (BLUE) | 활동 표시 (예약) |

`ERROR` 상태에서는 깜빡이지 않고 켜 둔다. 깜빡이면 정상 동작과 구분이 안 된다.

## 9. 정적 할당 콜백

`configSUPPORT_STATIC_ALLOCATION` 을 켜면 커널이 idle / timer 태스크 메모리를 애플리케이션에서 받아 간다. 두 콜백을 주지 않으면 링크가 깨진다.

```
undefined reference to `vApplicationGetIdleTaskMemory'
undefined reference to `vApplicationGetTimerTaskMemory'
```

`rtos.c` 에 넣었다. 정적 할당을 켜 두는 이유는 나중에 결정적인 버퍼가 필요할 때를 위해서다.

## 10. 훅

조용히 넘어가면 안 되는 상황이라 로그를 남기고 멈춘다. 디버거가 붙어 있으면 `__BKPT(0)` 로 잡힌다.

| 훅 | 설정 |
|---|---|
| `vApplicationStackOverflowHook` | `configCHECK_FOR_STACK_OVERFLOW 2` |
| `vApplicationMallocFailedHook` | `configUSE_MALLOC_FAILED_HOOK 1` |
| `vAssertCalled` | `configASSERT` |

## 11. 인터럽트 우선순위

RA8P1 의 `__NVIC_PRIO_BITS` 는 4 다(0~15, 숫자가 작을수록 높다).

```c
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY  2
```

**FreeRTOS API 를 부르는 ISR 은 `MAX_SYSCALL` 보다 낮은(= 숫자가 큰) 우선순위여야 한다.** `ra_gen` 의 UART 인터럽트는 `priority12` 로 잡혀 있어 조건을 만족한다. 새 페리페럴을 추가할 때 `configuration.xml` 의 `*_ipl` 값이 2 보다 큰지 확인한다.

## 12. 검증

```
[ Firmware Begin... ]
Booting..Name  		: TITAN-MINI-CM85
Booting..Ver   		: V260905R1
Booting..Clock 		: 1000 MHz

cli# thread info
thread cnt : 2 / 8
heap free  : 57016 / 65536 bytes
heap min   : 57016 bytes

name             pri  stack  free(word)
main             2    4096   996
cli              3    4096   852
```

`thread info` 는 `uxTaskGetStackHighWaterMark()` 로 각 스레드의 스택 여유를 워드 단위로 보여준다.

### CPU 점유율

```
cli# thread cpu
name              pri     cpu
cli                 3    0.0 %
IDLE                0   99.9 %
main                2    0.0 %
Tmr Svc             7    0.0 %
```

`vTaskGetRunTimeStats()` 는 부팅 이후 누적이라 "방금 무엇이 CPU 를 먹는가" 를 알 수 없다.
그래서 `uxTaskGetSystemState()` 로 **1 초 간격 스냅샷 두 장을 떠서 차이**를 본다.

카운터 원천은 **Cortex-M85 의 DWT 사이클 카운터**를 8비트 내린 값이다(1 GHz 에서 약 3.9 MHz).

```c
uint32_t bspGetRunTimeCounter(void)
{
  return DWT->CYCCNT >> 8;
}
```

`micros()` 를 쓰지 않는 이유는, 이 함수가 **태스크 전환 문맥에서 불리는데** `micros()` 는 내부에서 `xTaskGetTickCount()` 를 부르기 때문이다. DWT 는 틱과 무관하게 단조 증가한다.

검증은 `main` 스레드가 50 ms 중 30 ms 를 태우게 해서 했다.

```
name              pri     cpu
main                2   59.9 %      ← 30/50 = 60% 예상과 일치
IDLE                0   40.0 %
```

빌드는 FLASH 34,712 B (3.31%), RAM 78,008 B (4.07%)다. RAM 이 늘어난 것은 대부분 `configTOTAL_HEAP_SIZE` 64 KB 다.

디버거로도 확인할 수 있다. `halt` 후 PC 가 `prvIdleTask` 면 스케줄러가 도는 것이다.

```bash
pyocd cmd -t r7ka8p1kf --pack "$P" -c halt -c "reg pc"
arm-none-eabi-addr2line -f -e build/cm85/titan-mini-cm85.elf <PC>
```

> ### 시리얼 포트를 두 프로세스가 열 수 없다
>
> `minicom` 을 띄워 둔 채로 스크립트에서 같은 포트를 열면 이렇게 된다.
>
> ```
> SerialException: device reports readiness to read but returned no data
>                  (device disconnected or multiple access on port?)
> ```
>
> `lsof /dev/cu.usbmodem*` 로 누가 잡고 있는지 확인한다.

## 13. 다음

`23-cm33-boot.md` — CM33 기동. 각 코어가 독립된 FreeRTOS 인스턴스를 갖게 되므로 이 다음이 자연스럽다. 파티션 매크로를 직접 정의해야 한다([04장 3절](04-dualcore.md#3-파티션-매크로를-직접-정의해야-한다)).
