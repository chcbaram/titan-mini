# 21. UART + log / CLI

> SCI2(P801/P802)로 콘솔을 띄우고 `logPrintf` / CLI 를 붙인다. 이후 모든 단계의 검증 수단이 된다.
> 관련: [03-board-mapping.md](03-board-mapping.md) · [11-fsp-config.md](11-fsp-config.md)
>
> **상태: 완료.** 부팅 배너 · CLI 프롬프트 · `help` / `uart` / `log` 명령 동작 확인.

---

## 1. 어느 SCI 채널인가

회로도에 UART 가 나오는 곳은 세 군데다.

| 커넥터 | 네트 | 핀 | SCI 채널 |
|---|---|---|---|
| U18 40핀 RPi 헤더 (GPIO14/15 위치) | `TXD1` / `RXD1` | P707 / P706 | **SCI1** |
| **H1 3핀 헤더** | `TXD2` / `RXD2` | **P801 / P802** | **SCI2** ← 여기를 쓴다 |
| J6 FTSH-105 | — | — | SWD 전용, UART 없음 |

**HSLink 디버거의 가상 시리얼이 H1 에 연결돼 있다.** 회로도에는 H1 이 `DNP`(미실장)로 표기돼 있지만 실물에는 배선돼 있다. 케이블 하나로 적재와 콘솔이 동시에 된다.

핀이 어느 SCI 인지는 RASC 핀 매핑 정본에서 확인한다. 데이터시트 표를 뒤지지 않는 게 정확하다.

```
$RENESAS_RA_TOOLS/device/ra8p1_mcu/.mcu/.pinmapping/PinCfgR7KA8P1KxxCAC.xml
```

각 핀 `<component id="pNNN">` 의 `capabilitylist` 에 기능 목록이 있다.

```
=== P801 ===          === P802 ===
   OSPI0: OM_0_DQS       OSPI0: OM_0_SIO6
   SCI2:  TXD2           SCI2:  RXD2
```

> P801/P802 는 OSPI0 의 DQS · SIO6 과 겸용이다. 지금은 OSPI 를 4비트 모드로 쓸 계획이라
> 충돌하지 않지만, 8비트(옥탈)로 가려면 UART 를 SCI1(40핀 헤더)로 옮겨야 한다.
> 옮기는 방법은 아래 2장 그대로에 채널과 핀만 바꾸면 된다.

## 2. FSP 설정 — RASC 재생성

RA8P1 의 SCI UART 드라이버는 **`r_sci_b_uart`** 다(`r_sci_uart` 가 아니다). `configuration.xml` 에 네 가지를 넣고 재생성했다.

**① 컴포넌트**

```xml
<component class="HAL Drivers" group="all" subgroup="r_sci_b_uart" vendor="Renesas" version="6.6.0">
```

**② 모듈** — 채널 2, 115200 8N1, 플로우 컨트롤 없음, 콜백 `uartCallback2`

```xml
<module id="module.driver.uart_on_sci_b_uart.0">
  <property id="module.driver.uart.name"     value="g_uart2" />
  <property id="module.driver.uart.channel"  value="2" />
  <property id="module.driver.uart.baud"     value="115200" />
  <property id="module.driver.uart.callback" value="uartCallback2" />
  ...
</module>
```

**③ 스택 등록과 드라이버 공통 설정**

```xml
<context id="_hal.0">
  <stack module="module.driver.ioport_on_ioport.0" />
  <stack module="module.driver.uart_on_sci_b_uart.0" />
</context>

<config id="config.driver.sci_b_uart">
  <property id="config.driver.sci_b_uart.fifo_support" value="...disabled" />
  <property id="config.driver.sci_b_uart.dtc_support"  value="...disabled" />
  ...
</config>
```

**④ 핀**

```xml
<configSetting altId="p801.sci2.txd2" configurationId="p801" />
<configSetting altId="p802.sci2.rxd2" configurationId="p802" />
<configSetting altId="sci2.mode.asynchronousuart.free" configurationId="sci2.mode" />
<configSetting altId="sci2.txd2.p801" configurationId="sci2.txd2" />
<configSetting altId="sci2.rxd2.p802" configurationId="sci2.rxd2" />
```

재생성하면 `ra_gen/hal_data.c/h` 에 `g_uart2` 인스턴스가, `ra_gen/vector_data.c/h` 에 SCI2 의 RXI/TXI/TEI/ERI 네 벡터가 들어간다. CMake 의 FSP 모듈 목록에도 한 줄 추가한다.

```cmake
${RA_SDK_DIR}/ra/fsp/src/r_sci_b_uart/*.c
```

> ### 재생성이 손으로 고친 핀 설정을 되돌린다
>
> 씨앗이 EK-RA8P1 이라 `configuration.xml` 의 LED 핀이 EK 것(P303 / P600 / **PA07**)으로 남아
> 있었다. **PA07 은 이 보드에서 이더넷 PHY 리셋이다.** 그대로 재생성했으면 `pin_data.c` 의
> titan-mini LED 설정이 날아가고 PHY 리셋이 출력으로 잡힐 뻔했다.
>
> 재생성 전에 `configuration.xml` 의 핀도 함께 고쳤다.
>
> ```xml
> <configSetting altId="p108.output.high" configurationId="p108" />
> <configSetting altId="p108.gpio_mode.gpio_mode_out.high" configurationId="p108.gpio_mode" />
> ... p109, p110 동일
> ```
>
> **손으로 고친 것은 반드시 `configuration.xml` 에도 반영한다.** 그래야 재생성이 재현 가능해진다.

> ### custom board 컴포넌트가 빠지면 board_cfg.h 가 안 나온다
>
> `#Board# = board.custom` 만 바꾸고 BSP Board 컴포넌트를 통째로 지웠더니, RASC 가
> `bsp_cfg.h` 는 `#include "board_cfg.h"` 를 넣으면서 정작 그 파일을 만들지 않았다.
>
> ```
> bsp_cfg.h:11:22: fatal error: board_cfg.h: No such file or directory
> ```
>
> Custom Board 컴포넌트를 명시해야 한다.
>
> ```xml
> <component class="BSP" group="Board" subgroup="custom" vendor="Renesas" version="6.6.0">
>   <description>Custom Board Support Files</description>
>   <originalPack>Renesas.RA_board_custom.6.6.0.pack</originalPack>
> </component>
> ```
>
> 그러면 `bsp_init()` 선언만 든 `board_cfg.h` 가 생성된다. `bsp_init` 은 `bsp_common.c` 에
> weak 정의가 있어 구현하지 않아도 된다.

## 3. 드라이버

공용 API 헤더(`src/common/hw/include/uart.h`)는 기존 프로젝트 것을 그대로 쓰고 `.c` 만 FSP 로 다시 썼다. `log.c` / `cli.c` 는 포터블해서 그대로 가져왔다.

### TX 는 완료를 기다린다

FSP 의 `write()` 는 인터럽트 구동이라 바로 리턴한다. 다음 호출이 진행 중인 전송을 덮어쓰지 않도록 `uartWrite()` 안에서 완료를 기다린다.

```c
uart_tbl[ch].tx_done = false;

if (p_uart->p_api->write(p_uart->p_ctrl, p_data, length) != FSP_SUCCESS) { ... }

uint32_t timeout  = 100 + (length * 10 * 1000) / uart_tbl[ch].baud;
uint32_t pre_time = millis();

while (uart_tbl[ch].tx_done == false)
{
  if (millis() - pre_time >= timeout) return 0;
}
```

`tx_done` 은 콜백의 `UART_EVENT_TX_COMPLETE` 에서 세운다. 타임아웃을 둔 것은 전송이 끝나지 않아도 영원히 멈추지 않게 하려는 것이다.

### RX 는 콜백에서 qbuffer 로

DTC/FIFO 를 안 쓰므로 FSP 가 문자마다 `UART_EVENT_RX_CHAR` 로 콜백한다.

```c
void uartCallback2(uart_callback_args_t *p_args)
{
  switch (p_args->event)
  {
    case UART_EVENT_RX_CHAR:
    {
      uint8_t rx_data = (uint8_t)p_args->data;
      qbufferWrite(&uart_tbl[ch].qbuffer, &rx_data, 1);
      uart_tbl[ch].rx_cnt++;
      break;
    }
    case UART_EVENT_TX_COMPLETE:
      uart_tbl[ch].tx_done = true;
      break;
    ...
```

콜백 이름은 `configuration.xml` 의 `module.driver.uart.callback` 과 같아야 한다. `ra_gen/hal_data.c` 가 그 이름으로 등록한다.

### 초기화 순서

```c
bool hwInit(void)
{
  cliInit();      // 버퍼만 잡는다. 이후 드라이버가 cliAdd() 로 명령을 등록할 수 있게
  logInit();      // 열리기 전 출력을 부트 버퍼에 모아 두려고 먼저 부른다

  ledInit();
  uartInit();

  uartOpen(HW_UART_CH_CLI, 115200);

  logOpen(HW_LOG_CH, 115200);
  logPrintf(...);   // 부팅 배너

  cliOpen(HW_UART_CH_CLI, 115200);
  return true;
}
```

`ap.c` 의 메인 루프에 `cliMain()` 을 넣는다.

## 4. 로그 덤프 순서 — 최신이 마지막에 오게

`log boot` / `log list` 는 링버퍼를 덤프한다. 그런데 `logBufPrintf()` 는 항목이 끝에 안 들어가면 `buf_index` 를 0 으로 되돌려 앞쪽부터 덮어쓴다. 덤프가 0번지부터 선형으로 읽으면 **한 바퀴 돈 뒤에는 최신이 앞에 나오고 그 뒤에 옛것이 이어진다.** 시간 순서가 깨진다.

가장 오래된 자리부터 감아 읽도록 고쳤다.

```c
//-- buf_index 는 다음에 쓸 자리이고, 한 바퀴 돌았으면 그 자리가 곧 가장 오래된 데이터다.
//   아직 안 돌았으면 이 식이 0 이 된다.
index_begin = (log_buf_list.buf_length_max + log_buf_list.buf_index) - log_buf_list.buf_length;

...
buf_pos = (index_begin + index) % log_buf_list.buf_length_max;
if ((buf_pos + buf_len) > log_buf_list.buf_length_max)
{
  buf_len = log_buf_list.buf_length_max - buf_pos;   // 끝에서 잘라 다음 바퀴로
}
cliWrite((uint8_t *)&log_buf_list.buf[buf_pos], buf_len);
```

> 이 수정은 `ti-am263`(2026-02-17)에 있고 **그보다 최신인 `weact-h750-mini`(2026-08-30)에는 없다.**
> "가장 최근 커밋된 구현본" 만 보고 고르면 놓친다. 드라이버를 이식할 때는 저장소 몇 개를
> 비교해 보는 편이 낫다.

바이트 링버퍼라 덤프 시작점이 항목 경계에 안 맞아 첫 줄이 중간부터 잘려 나온다. 참조 구현도 같다.

## 5. 검증

```
[ Firmware Begin... ]
Booting..Name  		: TITAN-MINI-CM85
Booting..Ver   		: V260905R1
Booting..Clock 		: 1000 MHz
```

**Cortex-M85 가 1 GHz 로 돈다.**

```
cli# help

---------- cmd list ---------
HELP
MD
LOG
UART
-----------------------------

cli# uart info
_DEF_UART1 : SCI2 HSLink   115200 bps  rx 16, tx 275
```

덤프 순서는 버퍼(1 KB)를 넘치게 만들어 확인했다. `apInit()` 에서 60줄을 찍고 `log list` 를 보면 가장 오래 남은 `line 46` 부터 시작해 **최신인 `line 59` 로 끝난다.**

```bash
python3 - <<'PY'
import serial, time
s = serial.Serial('/dev/cu.usbmodem1412302', 115200, timeout=0.1)
s.write(b'help\r'); time.sleep(1); print(s.read(4096).decode())
PY
```

`screen /dev/cu.usbmodem1412302 115200` 으로도 된다. 나갈 때는 `Ctrl-A` `K`.

## 6. 걸렸던 것

### 처음에 SCI1(40핀 헤더)로 잡았다가 아무것도 안 나왔다

MCU 쪽은 전부 정상이었다. 그래서 원인을 찾는 데 시간이 걸렸다. 확인한 것들:

| 항목 | 값 | 판정 |
|---|---|---|
| P707 PFS | `0x05010002` — PMR=1, PSEL=5 | 페리페럴로 넘어감 ✓ |
| SCI1 CCR0 | `0x00010401` = RE + RIE + IDSEL | 열려 있음 ✓ |
| `uart_tbl[0].tx_cnt` | 122 | 배너를 실제로 전송 완료 ✓ |
| 호스트 포트 | `usbmodem1412302` = CherryUSB CMSIS-DAP | HSLink 맞음 ✓ |

전부 정상인데 호스트에 0 바이트였다. **HSLink 의 시리얼이 40핀 헤더가 아니라 H1 에 물려 있었다.** 회로도에 H1 이 DNP 로 표기돼 있어서 후보에서 뺐던 것이 원인이다.

### CCR0 오프셋을 0 으로 착각했다

디버깅 중에 `R_SCI1_BASE`(`0x40358100`)를 그대로 읽고 "CCR0 = 0, TE/RE 가 꺼져 있다" 고 오판했다. **CCR0 은 구조체 오프셋 `0x08` 이다.**

```c
__IOM uint32_t CCR0;    /*!< (@ 0x00000008) Common Control Register 0 */
```

레지스터를 직접 읽을 때는 베이스가 아니라 구조체 오프셋을 확인한다.

### TE 는 평소에 0 인 게 정상이다

FSP 는 `Write()` 에서 TE 를 켜고 TEI 인터럽트에서 다시 끈다. 유휴 상태에서 CCR0 의 TE(bit 4)가 0 인 것은 고장이 아니다.

### `log` 명령이 안 보였다

`hw_def.h` 에 `_USE_CLI_HW_LOG` 를 안 넣어서다. `CLI_USE(module)` 은 `((_USE_CLI_ ## module) && defined(_USE_HW_CLI))` 로 전개되므로, 드라이버마다 스위치를 명시해야 CLI 명령이 등록된다.

## 7. 다음

`22-freertos.md` — FreeRTOS 적용. 뒤따르는 FatFs · lwIP · USB 스택 · LVGL 이 전부 RTOS 를 전제하므로 일찍 넣는다. 포트 레이어는 FSP 의 `ra/fsp/src/rm_freertos_port/` 를 쓴다.
