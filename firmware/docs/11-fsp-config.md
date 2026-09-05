# 11. FSP 설정 — 생성물을 커밋하고 RASC 는 쓰지 않는다

> `ra_cfg` / `ra_gen` / 링커 스크립트를 어떻게 얻고, 어떻게 관리하는지.
> 관련: [10-dev-environment.md](10-dev-environment.md) · [12-project-skeleton.md](12-project-skeleton.md)

---

## 1. 방침

1. RASC 로 **한 번** 생성한 결과물(`ra/`, `ra_cfg/`, `ra_gen/`, `script/`, `configuration.xml`, `bsp_linker_info.h`)을 저장소에 통째로 커밋한다.
2. **평소 빌드·개발에는 RASC 가 전혀 필요 없다.** CMake 는 생성물의 `.c` 를 glob 할 뿐 RASC 를 호출하지 않는다.
3. 핀 추가·변경은 `ra_gen/pin_data.c` 를, 클럭 변경은 `ra_gen/bsp_clock_cfg.h` 를 **직접 편집**한다.
4. 드라이버 모듈 추가처럼 구조가 바뀔 때만 재생성한다.

RA4M1-CORE 가 쓰던 방식과 같다.

### 왜 순수 수동 작성이 아닌가

처음에는 GitHub FSP 소스만으로 전부 손으로 쓰려 했다. 조사해 보니 안 된다.

- **RASC 의 `fsp_gen.ld` / `memory_regions.ld` 는 템플릿 파일로 존재하지 않는다.** freemarker 코드가 module description XML 안에 **임베드**돼 있다 (`device/ra8p1_mcu/.module_descriptions/Renesas##BSP##ra8p1##linker####6.6.0.xml`). 확장자로 `.ftl` 을 찾으면 안 나온다.
- **GitHub FSP 저장소에 `core_cm85.h` 가 없다.** Arm.CMSIS6 팩에만 있다.
- **GitHub FSP 저장소에 `script/fsp.ld` 가 없다.** RA_mcu_ra8p1 팩에만 있다.

생성물을 씨앗으로 쓰는 편이 훨씬 낫다. 그러면서도 목표였던 "외부 설정툴을 상시 쓰지 않는다" 는 그대로 지켜진다.

## 2. 씨앗

`tools-bd` 세션이 준비한, RASC 생성 + `arm-none-eabi-gcc 14.2.1` 링크까지 검증된 참조 프로젝트에서 출발했다.

```
$RENESAS_RA_TOOLS/reference/ra8p1_ek_minimal/    단일코어 (CPU0)
$RENESAS_RA_TOOLS/reference/ra8p1_ek_dualcore/   CPU0 / CPU1  ← 이쪽의 CPU0 를 썼다
```

배치는 이렇게 나눴다.

| 참조 프로젝트 | 이 저장소 |
|---|---|
| `ra/` (FSP 소스 + CMSIS 6 Core) | `src/lib/ra_sdk/ra/` — **코어 공유** |
| `ra_cfg/` `ra_gen/` `script/` | `src/lib/ra_sdk/cm85/` |
| `configuration.xml` `bsp_linker_info.h` `memory_regions.ld` `fsp_gen.ld` | 〃 |
| `src/hal_entry.c` | `src/lib/ra_sdk/cm85/src/` |

`ra/` 를 공유하는 이유는 CPU0 와 CPU1 의 FSP 소스 트리가 완전히 동일하기 때문이다(참조 프로젝트에서 `diff -rq` 로 확인). 두 타겟이 각각 다른 `-mcpu` 로 다시 컴파일할 뿐이다. 코어마다 다른 것은 `ra_cfg` / `ra_gen` / `script` 뿐이다.

## 3. EK-RA8P1 → custom board 로 전환

씨앗은 EK-RA8P1 보드 설정이었다. titan-mini 는 보드가 다르므로 `board.custom` 으로 바꿨다. 한 일은 네 가지다.

**① `board_cfg.h` 를 custom 형태로**

EK 판은 보드 헤더를 상대 경로로 끌어온다.

```c
#include "../../../ra/board/ra8p1_ek/board.h"
```

custom 판은 `bsp_init()` 선언만 남는다. RA4M1-CORE 의 것과 같은 형식이다.

```c
#ifndef BOARD_CFG_H_
#define BOARD_CFG_H_
#ifdef __cplusplus
        extern "C" {
        #endif

        void bsp_init(void * p_args);

        #ifdef __cplusplus
        }
        #endif
#endif /* BOARD_CFG_H_ */
```

`bsp_init` 은 `bsp_common.c` 에 weak 정의가 있으므로 따로 구현하지 않아도 된다.

**② `ra/board/ra8p1_ek/` 삭제** — FSP 의 BSP 코어는 `board.h` / `board_init` 을 전혀 참조하지 않는다. 보드 계층은 선택적 글루다.

**③ `configuration.xml` 수정**

```xml
<option key="#Board#" value="board.custom" />
<option key="#SELECTED_TOOLCHAIN#" value="com.renesas.cdt.managedbuild.gnuarm.toolchain." />
```

`#ConfigurationFragments#`(`Renesas##BSP##Board##ra8p1_ek##`), `#ToolchainVersion#`, 그리고 BSP Board 컴포넌트 블록을 지웠다. 씨앗이 LLVM 툴체인 기준이라 GCC 로 바꿨다.

**④ 인터럽트 제거** — 씨앗에 EK 예제용 DOC 인터럽트가 하나 잡혀 있었다.

```xml
<interrupt event="event.doc.int" isr="baremetal_doc_isr" />
```

이걸 지우지 않으면 `vector_data.c` 가 존재하지 않는 `baremetal_doc_isr` 를 참조해 **링크가 깨진다.**

```
undefined reference to `baremetal_doc_isr'
```

`vector_data.c` / `vector_data.h` 를 IRQ 0개 형태로 다시 썼다 (`VECTOR_DATA_IRQ_COUNT (0)`).

## 4. 핀 설정

`ra_gen/pin_data.c` 가 전부다. `R_BSP_WarmStart(POST_C)` 의 `R_IOPORT_Open()` 이 이걸 적용한다.

| 핀 | 설정 | 용도 |
|---|---|---|
| P208 ~ P211 | `IOPORT_CFG_PERIPHERAL_PIN \| IOPORT_PERIPHERAL_DEBUG` | TDI / SWO / SWDIO / SWCLK |
| P108 / P109 / P110 | `IOPORT_CFG_PORT_DIRECTION_OUTPUT \| IOPORT_CFG_PORT_OUTPUT_HIGH` | RGB LED (active low 라 HIGH 가 소등) |

**EK-RA8P1 도 디버그가 P208~P211 이라 씨앗의 디버그 설정을 그대로 쓸 수 있었다.** EK 의 LED 3개(P303 / P600 / P1007)만 titan-mini 의 P108~P110 으로 갈아끼웠다.

핀을 추가할 때는 이 배열에 항목을 늘리면 된다. 핀 이름은 `BSP_IO_PORT_<포트 2자리>_PIN_<핀 2자리>` 형식이다 — P109 는 `BSP_IO_PORT_01_PIN_09`.

## 5. 재생성

구조가 바뀔 때만 한다.

```bash
$RENESAS_RA_TOOLS/bin/rasc-generate.sh \
    firmware/ra8p1-fw/src/lib/ra_sdk/cm85/configuration.xml
```

> ### 함정 — 조용히 실패한다
>
> FSP 예제의 `configuration.xml` 은 버전이 6.4.0 으로 고정돼 있다. 그대로 돌리면
> `Failed to locate component ... in any software packs` 로 **아무것도 생성하지 않고 끝난다.**
> 에러 코드도 0 이다.
>
> ```bash
> sed -i '' 's/6\.4\.0/6.6.0/g' configuration.xml
> ```
>
> 재생성 후에는 반드시 **파일 타임스탬프를 확인**한다.

GUI 가 필요하면:

```bash
open $RENESAS_RA_TOOLS/rasc/rasc.app --args <경로>/configuration.xml
```

재생성하면 위 3장의 custom board 수정과 4장의 핀 설정이 되살아날 수 있다. 재생성 후 `git diff` 로 확인한다.

## 6. Solution(듀얼코어) 은 헤드리스로 생성할 수 없다

standalone RASC 의 `--generatesolution` 은 실제로는 no-op 이다.

- `--fspversion` 없이 부르면 `Invalid template ID, valid values are []`
- `--fspversion 6.6.0` 을 주면 `single_*_minimal` 계열 6개만 노출된다. `multi_flat_blinky` 같은 멀티코어 템플릿은 필터링돼 사라진다
- 유효 인자를 다 채워도 로그에 `Solution generation` 한 줄만 찍히고 출력 디렉터리가 빈 채로 끝난다. `--toolchainid` 에 쓰레기값을 넣어도 에러가 안 난다 — 검증을 하지 않는다

Solution 생성은 e2 studio GUI 전용으로 보인다. 그래서 **파티션 매크로는 직접 정의한다.** 이름 규칙과 계획은 [04-dualcore.md](04-dualcore.md#3-파티션-매크로를-직접-정의해야-한다) 에 있다.

## 7. FSP 모듈을 추가할 때

CMake 는 FSP 를 **glob 하지 않는다.** 쓰는 모듈만 명시한다.

```cmake
file(GLOB_RECURSE RA_SDK_SRC_FILES CONFIGURE_DEPENDS
  ${RA_SDK_DIR}/ra/fsp/src/bsp/*.c
  ${RA_SDK_DIR}/ra/fsp/src/r_ioport/*.c
  )
```

전체를 glob 하면 **설정 헤더(`ra_cfg/fsp_cfg/r_<모듈>_cfg.h`)가 없는 모듈 소스까지 잡혀서 빌드가 깨진다.** NUCLEO-N657X0 이 ST HAL 을 명시적으로 나열하는 것과 같은 이유다.

모듈 하나를 추가하는 순서는 이렇다.

1. `$RENESAS_RA_TOOLS/fsp/ra/fsp/src/r_<모듈>/` 을 `src/lib/ra_sdk/ra/fsp/src/` 로 복사
2. 해당 `ra/fsp/inc/instances/r_<모듈>.h` 와 `ra/fsp/inc/api/*_api.h` 도 복사
3. `ra_cfg/fsp_cfg/r_<모듈>_cfg.h` 를 만든다 (FSP 예제에서 가져오는 게 빠르다)
4. `ra_gen/hal_data.c/h` 에 인스턴스를 추가
5. 최상위 `CMakeLists.txt` 의 `RA_SDK_SRC_FILES` 에 한 줄 추가
6. 인터럽트를 쓴다면 `ra_gen/vector_data.c/h` 도 갱신

`$RENESAS_RA_TOOLS/examples/ek_ra8p1/` 의 102개 예제가 각 모듈의 `ra_cfg` / `ra_gen` 참고 자료가 된다.
