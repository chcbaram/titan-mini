# 23. CM33 기동

> CPU0(Cortex-M85)이 CPU1(Cortex-M33)을 깨우고, 두 코어가 메모리를 나눠 쓰게 한다.
> 관련: [02-memory-map.md](02-memory-map.md) · [04-dualcore.md](04-dualcore.md) · [11-fsp-config.md](11-fsp-config.md)
>
> **상태: 완료.** CPU1 기동 확인, 공유 블록으로 생존 확인.

---

## 1. 파티션

MRAM 과 SRAM 을 코어별로 나눈다. 값은 `src/lib/ra_sdk/partition.h` 한곳에서 관리한다.

| 영역 | 시작 | 크기 |
|---|---|---|
| MRAM CPU0 | `0x0200_0000` | 768 KB |
| MRAM CPU1 | `0x020C_0000` | 256 KB |
| SRAM CPU0 | `0x2200_0000` | 1408 KB |
| SRAM CPU1 | `0x2216_0000` | 384 KB |
| SRAM 공유 | `0x221C_0000` | 80 KB |

합계는 MRAM 1 MB, SRAM `0x1D_4000`(1872 KB)로 정확히 맞는다.

**CPU1 시작 주소를 `0x020C_0000` 으로 잡은 이유** — 40번 단계에서 부트로더가 들어오면
CPU0 영역 **앞쪽**을 128 KB 떼어 준다. CPU1 은 그대로 두면 되므로 지금 이 값을 정해도
나중에 흔들리지 않는다.

> `CPU1INITVTOR` 은 하위 7비트가 버려지므로 **128 바이트 정렬이 필수**다.
> `0x020C_0000` 은 조건을 만족한다.

## 2. 파티션 매크로를 직접 정의한다

FSP 는 `BSP_PARTITION_FLASH_CPU1_S_START` 가 정의돼 있는지로 멀티코어 프로젝트를 판정한다.

```c
/* bsp_common.h:207 */
#if defined(BSP_PARTITION_FLASH_CPU1_S_START)
 #define BSP_MULTICORE_PROJECT    (1)
#else
 #define BSP_MULTICORE_PROJECT    (0)
#endif
```

그리고 `R_BSP_SecondaryCoreStart()` 가 그 조건 안에 있어서, 매크로가 없으면 **함수 자체가
컴파일되지 않는다.**

RASC 는 이 매크로를 Solution 프로젝트에서만 `bsp_linker_info.h` 의 `Solution Definitions`
블록에 넣어 준다. 그런데 standalone RASC 로는 Solution 을 헤드리스 생성할 수 없다
([11장 6절](11-fsp-config.md#6-solution듀얼코어-은-헤드리스로-생성할-수-없다)). 그래서 직접 정의한다.

`src/lib/ra_sdk/partition.h` 를 만들고, 두 코어의 `bsp_linker_info.h` 가 그 자리에서
include 하게 했다.

```c
/* bsp_linker_info.h */
/******* Solution Definitions *************/
#include "partition.h"
```

이름 규칙은 RASC 의 freemarker 템플릿 원문에서 읽어낸 것이라 정식 형식과 같다.

```
BSP_PARTITION_<RESOURCE>_<CPU0|CPU1>_<S|NS>_START / _SIZE
```

확인은 컴파일 시점에 한다.

```c
#if BSP_MULTICORE_PROJECT
#warning "MULTICORE = 1"
#else
#error "MULTICORE = 0 — 파티션 매크로가 안 보인다"
#endif
```

## 3. memory_regions.ld 를 코어별로 좁힌다

RASC 는 두 코어 모두에게 **MRAM/SRAM 전체**를 준다. 그대로 두면 두 이미지가 같은 주소에
링크된다. 파티션 값으로 좁힌다.

```
              FLASH_START   FLASH_LENGTH   RAM_START     RAM_LENGTH
  cm85        0x02000000    0x000C0000     0x22000000    0x00160000
  cm33        0x020C0000    0x00040000     0x22160000    0x00060000
```

> **재생성하면 되돌아간다.** `memory_regions.ld` 는 RASC 생성물이다. 파일 맨 위에 그 사실을
> 주석으로 적어 뒀다. 재생성 후에는 `partition.h` 값으로 다시 맞춰야 한다.

## 4. 기동

`hal_entry()` 에서 부른다. CPU0 만 해당한다.

```c
void hal_entry(void)
{
#if (1 == BSP_MULTICORE_PROJECT) && !BSP_SECONDARY_CORE_BUILD
    R_BSP_SecondaryCoreStart();
#endif
}
```

FSP 의 인라인 함수가 레지스터 세 개를 쓴다.

| 레지스터 | 값 | 의미 |
|---|---|---|
| `CPU1INITVTOR` | `BSP_PARTITION_FLASH_CPU1_S_START` | 벡터 테이블 주소 |
| `CPU1WAITCR` | `0` | 디버거가 CPU1 을 붙잡아 둔 경우를 푼다 |
| `CPU1ACTCSR` | `(0xA5 << KEY) \| ACTREQ` | 기동 요청 |

## 5. 공유 블록

CPU1 이 살아 있는지 CPU0 이 확인할 수단이 필요하다. SRAM 공유 구간에 구조체 하나를 둔다.

```c
/* src/common/hw/include/shared.h */
#define SHARED_MAGIC   0x544D5348UL   /* "TMSH" */

typedef struct
{
  volatile uint32_t magic;
  volatile uint32_t version;
  volatile uint32_t cpu1_alive;
  volatile uint32_t cpu1_tick;
} shared_t;
```

링커 스크립트가 두 코어 모두 같은 주소에 놓는다.

```
  .shared 0x221C0000 (NOLOAD) :
  {
    KEEP (*(.shared))
    . = ALIGN(4);
  }
```

> ### NOLOAD 여야 한다
>
> 이미지에 담지도, 부팅 시 0 으로 밀지도 않는다. 초기화하면 **나중에 부팅한 코어가 상대가
> 써 둔 값을 지운다.** CPU0 이 먼저 뜨고 CPU1 이 나중에 뜨는데, CPU1 의 스타트업이 이 영역을
> 밀어 버리면 순서에 따라 값이 사라진다.
>
> 대신 **자기 필드는 자기가 초기화한다.** 그리고 `magic` 을 **마지막에** 쓴다.
>
> ```c
> g_shared.cpu1_alive = 0;
> g_shared.cpu1_tick  = 0;
> g_shared.version    = SHARED_VERSION;
>
> __DMB();
> g_shared.magic = SHARED_MAGIC;
> ```
>
> CPU0 은 `magic` 만 보고 유효하다고 판단한다. 카운터가 쓰레기인 상태에서 `magic` 이 먼저
> 서면 CPU0 이 그걸 읽는다. 실제로 이 순서를 안 지켰을 때 `alive` 가 13억부터 시작했다.

`shared_t` 의 실체는 **CPU1 이 정의**하고 CPU0 은 같은 주소에 심볼만 얹는다.

## 6. CMake — FSP 모듈 목록은 코어별이다

CPU1 빌드가 이 에러로 깨졌다.

```
r_sci_b_uart.h:20:10: fatal error: r_sci_b_uart_cfg.h: No such file or directory
```

FSP 모듈 목록이 최상위에 하나뿐이라 CPU1 도 UART 소스를 가져갔는데, **설정 헤더는 코어마다
다르다.** 콘솔은 CPU0 만 쓰므로 `cm33/ra_cfg` 에는 `r_sci_b_uart_cfg.h` 가 없다.

최상위에는 두 코어가 반드시 쓰는 것만 두고, 나머지는 코어별 `CMakeLists.txt` 에서 더한다.

```cmake
# 최상위
file(GLOB_RECURSE RA_SDK_SRC_FILES ...
  ${RA_SDK_DIR}/ra/fsp/src/bsp/*.c
  ${RA_SDK_DIR}/ra/fsp/src/r_ioport/*.c)

# src/cpu/cm85/CMakeLists.txt
file(GLOB RA_SDK_SRC_CORE ...
  ${RA_SDK_DIR}/ra/fsp/src/r_sci_b_uart/*.c)
```

## 7. CPU1 은 핀을 소유하지 않는다

`cm33` 의 `configuration.xml` 에서 핀 설정을 전부 비웠다. 핀은 CPU0 의 `pin_data.c` 가 잡는다.

FSP 의 secondary 빌드는 `PMSAR`(핀 보안 속성)을 **AND 로만** 건드리도록 되어 있다.

```c
/* pin_data.c */
#if BSP_SECONDARY_CORE_BUILD
    R_PMISC->PMSAR[i].PMSAR &= (uint16_t) pmsar[i];   // AND — CPU0 설정을 보존
#else
    R_PMISC->PMSAR[i].PMSAR = (uint16_t) pmsar[i];
#endif
```

CPU1 이 쓰는 페리페럴의 핀도 CPU0 의 `pin_data.c` 에 넣는다.

## 8. 함정 — CPU1 을 깨우면 pyOCD 가 죽는다

CPU1 이 실제로 돌기 시작한 뒤부터 적재가 실패했다.

```
AHB-AP#2 ... [0]<e000e000:SCS M33 class=9 designer=43b:Arm part=d21 ...>
CPU core #0: Cortex-M85 r1p1, v8.1-M architecture
Error: <APv1Address@0x106734610 #2 dp=0>
```

`-O debug.traceback=true` 로 본 실제 예외다.

```
File ".../pyocd/target/pack/pack_target.py", line 513, in _pack_target_add_core
    pname = _self._pack_device.processors_ap_map[core.ap.address].name
KeyError: <APv1Address@... #2 dp=0>
```

**원인은 DFP 팩이다.** `Renesas.RA_DFP.pdsc` 에 `<processor Pname="CPU0"/>` 와 `Pname="CPU1"`
은 선언돼 있는데, **어느 AP 에 붙는지가 없다.** `<debug svd="..."/>` 하나뿐이다.

pyOCD 의 `cmsis_pack.py:_build_aps_map()` 은 `<debug>` 요소에서 `Pname` + `__apid` 또는
`__ap` 를 읽는다. 둘 다 없으면 첫 프로세서를 AP#0 에만 매핑한다. 그래서 AP#2 에서 M33 이
발견되는 순간 조회가 실패한다.

CPU1 이 잠들어 있을 때는 AP#2 가 `WAIT ACK` 로 응답을 안 해서 넘어갔던 것이고, 기동한
뒤부터 터진다.

**수정** — 두 subFamily 여는 태그 뒤에 AP 매핑을 넣는다.

```xml
<subFamily DsubFamily="RA8P1_1M_DualCore">
  <debug Pname="CPU0" __ap="0"/>
  <debug Pname="CPU1" __ap="2"/>
```

AP 번호는 실제 스캔 결과다 — AHB-AP#0 에 SCS M85, AHB-AP#2 에 SCS M33, APB-AP#1 은 디버그 APB.

> 이 수정은 `tools-bd` 세션에 넘겼다. 정본 팩(`packs/Renesas.RA_DFP.6.6.0-fixed.pack`)에
> 반영되면 `-DRA_DFP_PACK=` 임시 지정을 걷어낸다.

## 9. 검증

```bash
cmake -S . -B build -G Ninja -DBUILD_CM33=ON
cmake --build build -j8
cmake --build build --target flash        # 두 이미지를 한 번에 올린다
```

링크 주소 확인:

```
$ arm-none-eabi-objdump -h build/cm33/titan-mini-cm33.elf
  0 __flash_vectors$$ 00000040  020c0000     ← CPU1 파티션, 128 B 정렬
 12 .shared           00000010  221c0000

$ arm-none-eabi-objdump -h build/cm85/titan-mini-cm85.elf
 14 .shared           00000000  221c0000     ← 같은 주소
```

동작 확인:

```
cli# thread cpu1
magic   : 0x544D5348
version : 1
alive   : 28  (+4 / 500ms)
tick    : 3403 ms  (+504)
```

`alive` 가 0 부터 시작하고 `tick` 이 500 ms 창에서 504 증가한다 — CPU1 의 SysTick 이 1 kHz 로 돈다.

## 10. 다음

CPU1 은 지금 살아 있다는 표시만 남긴다. 앞으로 필요한 것:

- **IPC 세마포어** — FSP 의 `bsp_ipc.c`. 기동 핸드셰이크와 상호배제
- **공유 영역 non-cacheable** — MPU 설정. 지금은 D-cache 를 아직 안 켰기 때문에 문제가
  드러나지 않는다. 캐시를 켜기 전에 반드시 잡아야 한다 ([04장 5절](04-dualcore.md#5-d-cache--이걸-빼먹으면-조용히-깨진다))
- **CPU1 쪽 FreeRTOS** — 각 코어가 독립된 인스턴스를 갖는다
- **역할 분담** — 무엇을 CPU1 에 내릴지. NPU 추론이 유력한 후보다
