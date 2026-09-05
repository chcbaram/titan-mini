# 20. RGB LED 구동

> 부팅 가능한 최소 프로젝트를 세우고, 빌드 → 적재 → 검증 루프를 확립한다.
> 관련: [03-board-mapping.md](03-board-mapping.md) · [12-project-skeleton.md](12-project-skeleton.md)
>
> **상태: 완료.** 보드에서 P109(RED) 500 ms 점멸 확인.

---

## 1. 하드웨어

RGB LED 한 개(`LED3`, `XL-1615RGBC-RF`)다. **공통 애노드가 `+3V3` 이라 전부 active LOW** 다.

| 채널 | 네트 | 핀 | 저항 |
|---|---|---|---|
| `_DEF_LED1` | `LED_R` | **P109** | R80 2 KΩ |
| `_DEF_LED2` | `LED_G` | **P108** | R81 12 KΩ |
| `_DEF_LED3` | `LED_B` | **P110** | R82 10 KΩ |

> RA 계열 기본 디버그 핀이 P108~P110 이지만 이 보드는 디버그를 **P208~P211** 로 뺐다.
> 자세한 내용은 [03-board-mapping.md](03-board-mapping.md#2-led--구동-확인-완료).

## 2. 핀 설정

`ra_gen/pin_data.c` 가 전부다. `R_BSP_WarmStart(POST_C)` 의 `R_IOPORT_Open()` 이 `main()` 전에 적용한다.

```c
{
    .pin = BSP_IO_PORT_01_PIN_08,
    .pin_cfg = ((uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t) IOPORT_CFG_PORT_OUTPUT_HIGH)
},
```

`OUTPUT_HIGH` 가 **소등**이다. active low 라서 초기값을 HIGH 로 둬야 부팅 중에 LED 가 깜빡이지 않는다.

## 3. 드라이버

`src/cpu/cm85/hw/driver/led.c`. FSP 의 IOPORT 인스턴스를 **함수 포인터 테이블**로 부른다 (`R_IOPORT_*` 심볼을 직접 쓰지 않는다). RA4M1-CORE 와 같은 방식이다.

```c
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

void ledOn(uint8_t ch)
{
  if (ch >= LED_MAX_CH) return;

  g_ioport.p_api->pinWrite(g_ioport.p_ctrl, led_tbl[ch].pin, led_tbl[ch].on_state);
}
```

`on_state` 가 `BSP_IO_LEVEL_LOW` 인 것이 이 보드의 특징이다. 테이블에 on/off 를 둘 다 넣어 두면 active high 보드로 옮겨도 테이블만 바꾸면 된다.

`ledInit()` 은 방향을 잡지 않는다. `pin_data.c` 가 이미 했기 때문이다. 채널마다 `ledOff()` 만 부른다.

`hw_def.h`:

```c
#define _USE_HW_LED
#define      HW_LED_MAX_CH          3
```

## 4. 애플리케이션

```c
void apMain(void)
{
  uint32_t pre_time = millis();

  while (1)
  {
    if (millis() - pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }
  }
}
```

`millis()` 는 `bspInit()` 이 세운 SysTick(1 ms)이 끌고 간다.

```c
SysTick_Config(SystemCoreClock / 1000);
```

## 5. 빌드 · 적재

```bash
export RENESAS_RA_TOOLS=~/hdd/tools/renesas-ra
cd firmware/ra8p1-fw

cmake -S . -B build -G Ninja
cmake --build build -j8
cmake --build build --target flash
```

결과:

```
Memory region         Used Size  Region Size  %age Used
             RAM:        1210 B      1872 KB      0.06%
           FLASH:        4764 B         1 MB      0.45%
```

```
Erased 32768 bytes (1 sector), programmed 8192 bytes (1 page), skipped 0 bytes
```

## 6. 검증 — 눈으로 보지 않고 확인하는 법

LED 를 눈으로 보는 대신 포트 레지스터를 직접 샘플링하면 확실하다. `R_PORT1` 은 `0x4040_0020` 이고, `PCNTR1` 은 상위 16비트가 `PODR`, 하위 16비트가 `PDR` 이다.

```bash
P=$RENESAS_RA_TOOLS/packs/Renesas.RA_DFP.6.6.0-fixed.pack
for i in 1 2 3 4; do
  pyocd cmd -t r7ka8p1kf --pack "$P" --connect attach -c "read32 0x40400020" 2>/dev/null | grep '^4040'
  sleep 0.4
done
```

```
40400020:  07000700
40400020:  05000700
40400020:  07000700
40400020:  05000700
```

읽는 법:

- 하위 `0x0700` = `PDR` — 비트 8·9·10 이 서 있다. **P108 / P109 / P110 이 출력으로 잡혔다.**
- 상위가 `0x0700` ↔ `0x0500` 로 진동 — **비트 9(P109 = RED)가 토글된다.** `0x0500` 일 때 bit9 가 LOW 이므로 점등이다.

`--connect attach` 를 써야 타깃을 멈추지 않고 붙는다. 빼면 halt 돼서 토글이 멈춘다.

## 7. 걸렸던 것들

### 씨앗 프로젝트의 인터럽트가 링크를 깼다

EK 참조 프로젝트에 DOC 인터럽트가 하나 잡혀 있었다. `hal_entry.c` 를 갈아끼우면서 `baremetal_doc_isr` 이 사라졌는데 `vector_data.c` 는 그대로라 링크가 깨졌다.

```
undefined reference to `baremetal_doc_isr'
```

`vector_data.c/h` 를 IRQ 0개 형태로 다시 쓰고 `configuration.xml` 에서도 해당 항목을 지웠다([11-fsp-config.md](11-fsp-config.md#3-ek-ra8p1--custom-board-로-전환)).

### Ninja 인데 CMAKE_MAKE_PROGRAM 에 make 가 들어갔다

씨앗 툴체인 파일이 무조건 make 를 찾아서 `CMAKE_MAKE_PROGRAM` 에 넣었다. Ninja 제너레이터에서는 CMake 가 이 변수에서 ninja 를 기대하므로 `is less than the version of Ninja required by CMake (1.3)` 로 죽는다. Makefile 계열일 때만 찾도록 바꿨다([12-project-skeleton.md](12-project-skeleton.md#5-크로스-플랫폼-규칙)).

### board_cfg.h 의 상대 경로

씨앗의 `board_cfg.h` 가 `../../../ra/board/ra8p1_ek/board.h` 를 include 한다. `ra/` 를 코어 공유 트리로 옮기면서 이 경로가 깨졌다. 어차피 custom board 로 갈 것이라 보드 계층을 아예 제거했다. FSP 의 BSP 코어는 `board.h` 를 전혀 참조하지 않아서 안전하다.

## 8. 다음

[21-uart-cli.md](21-uart-cli.md) — UART1(P707/P706) + log/cli. HSLink 의 가상 시리얼이 이미 이 핀에 붙어 있어서, 케이블 추가 없이 부팅 배너를 볼 수 있다. 이후 모든 단계의 검증 수단이 된다.
