# 12. 프로젝트 구조와 빌드

> 디렉터리 배치, CMake 구성, 그리고 세 OS 에서 빌드되게 유지하기 위한 규칙.
> 관련: [11-fsp-config.md](11-fsp-config.md) · [05-boot-architecture.md](05-boot-architecture.md)

---

## 1. 디렉터리

```
firmware/
├── docs/                          이 문서들
│   └── images/*.svg
└── ra8p1-fw/
    ├── CMakeLists.txt             툴체인 include + project() + add_subdirectory
    ├── .clang-format  .gitignore
    ├── .vscode/{tasks,launch,c_cpp_properties}.json
    ├── prj/*.code-workspace       코어별 VSCode 워크스페이스
    ├── tools/
    │   ├── arm-none-eabi-gcc.cmake
    │   └── flash.cmake            pyocd 적재를 CMake 타겟으로
    └── src/
        ├── common/                ── 코어 공유 ──
        │   ├── def.h  err_code.h  evt_code.h
        │   ├── core/{qbuffer,util_core}.{c,h}
        │   └── hw/include/        led.h uart.h cli.h ... 공용 드라이버 API 헤더
        ├── cpu/                   ── 코어별 손으로 쓰는 코드 ──
        │   ├── cm85/              CPU0 (Cortex-M85)
        │   │   ├── CMakeLists.txt
        │   │   ├── main.c  main.h
        │   │   ├── ap/    ap.c  ap.h  ap_def.h
        │   │   ├── bsp/   bsp.c  bsp.h
        │   │   └── hw/    hw.c  hw.h  hw_def.h  driver/
        │   └── cm33/              CPU1 (Cortex-M33) — 스켈레톤
        └── lib/                   ── 벤더 ──
            └── ra_sdk/
                ├── ra/            FSP 소스 + CMSIS 6 Core (코어 공유)
                └── cm85/          configuration.xml  ra_cfg/  ra_gen/  script/  src/
```

`src/` 최상위는 전부 **카테고리**다 — `common`(공유 코드) / `cpu`(코어별 코드) / `lib`(벤더). 코어 이름 같은 인스턴스가 카테고리와 같은 높이에 오지 않는다.

`src/common/core/`(qbuffer, util_core)와 겹치지 않도록 코어 디렉터리는 `core` 가 아니라 **`cpu`** 다. FSP 도 `_RA_CORE=CPU0` 처럼 CPU 로 부른다.

### 계층 규칙 — ap 는 벤더 HAL 을 모른다

이 프로젝트의 이식성은 규칙 하나에 걸려 있다. **MCU 가 바뀌면 `bsp` 와 `hw/driver` 는 다시
쓰지만 `ap` 와 `common` 은 그대로 간다.**

| 층 | 벤더 HAL(FSP/CMSIS) | 역할 |
|---|---|---|
| `ap/` | **금지** | 애플리케이션. `hw/` 의 공용 API 만 쓴다 |
| `common/` | **금지** | 다른 저장소와 그대로 공유하는 포터블 코드 |
| `hw/driver/` | 허용 | **여기가 MCU 의존을 가두는 층이다** |
| `bsp/` | 허용 | MCU 초기화, 클럭, 시간 |

`ap` 가 `hw` 드라이버를 부르는 것은 정상 경로다. 막는 것은 `ap` 가 `R_IOPORT_PinWrite()` 같은
FSP 함수를 **직접** 부르는 것이다.

> ### 컴파일러는 이걸 막아주지 않는다
>
> `ap_def.h` → `hw.h` → `hw_def.h` → `bsp.h` → `hal_data.h` 로 FSP 심볼이 `ap` 까지 전부
>보인다. 관행에만 의존하면 언젠가 새어 들어간다. 그래서 검사기를 둔다.
>
> ```bash
> python3 firmware/ra8p1-fw/tools/check_layers.py
> ```
>
> `R_*` / `FSP_*` / `BSP_*` / `fsp_err_t` / `g_ioport` 같은 벤더 심볼과 `hal_data.h`,
> `r_*.h` 같은 벤더 헤더를 금지 층에서 찾는다. 위반이 있으면 종료 코드 1 이다.

### 핀 번호는 드라이버 `.c` 에 적는다

`hw_def.h` 에는 **기능 스위치와 개수만** 둔다. 어느 핀을 쓰는지는 그 핀을 쓰는 드라이버가
안다. `led.c` 상단에 RGB LED 결선을, `uart.c` 상단에 SCI2 핀을 적는 식이다.

전체 핀 맵은 [03-board-mapping.md](03-board-mapping.md) 하나에만 둔다. 같은 정보를
`hw_def.h` 에도 복사해 두면 회로도가 바뀔 때 한쪽만 고치게 된다.

레이어 규약은 NUCLEO-N657X0 과 같다: `main → bsp → hw → ap`, `_USE_HW_*` 스위치, `common/hw/include` 의 공용 API 헤더와 `hw/driver` 의 칩별 구현 분리.

## 2. 빌드 타겟

| 타겟 | 파일 | 플래그 |
|---|---|---|
| `titan-mini-cm85.elf` | `src/cpu/cm85/CMakeLists.txt` | `-mcpu=cortex-m85 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16` |
| `titan-mini-cm33.elf` | `src/cpu/cm33/CMakeLists.txt` | `-mcpu=cortex-m33 -mfpu=fpv5-sp-d16` (예정) |
| `flash` | `tools/flash.cmake` | pyocd 적재 |

컴파일 정의는 코어별로 `-D_RA_CORE=CPU0` / `CPU1`, 공통으로 `-D_RA_ORDINAL=1 -D_RENESAS_RA_`.

산출물은 **코어 이름으로 얕게** 모은다. 소스 트리 구조와 무관하므로 코어 디렉터리를
옮기거나 이름을 바꿔도 `launch.json` 과 `flash` 타겟이 깨지지 않는다.

```
build/
├── cm85/   titan-mini-cm85.elf · .bin · .map
├── cm33/   titan-mini-cm33.elf · .bin · .map
└──         titan-mini.fw        두 코어를 묶은 통합 펌웨어 이미지 (41번 단계)
```

`build/` 루트는 **통합 이미지 전용**으로 비워 둔다. `tools/mkimage.py` 가 두 코어의
`.bin` 을 섹션 컨테이너로 묶어 여기에 쓴다([05-boot-architecture.md](05-boot-architecture.md#2-펌웨어-이미지--섹션-컨테이너)).

코어 쪽 `CMakeLists.txt` 에서 이렇게 잡는다.

```cmake
set(CORE_OUT ${CMAKE_BINARY_DIR}/${CORE_NAME})
set_target_properties(${EXECUTABLE} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CORE_OUT})
```

`.map` 과 `.bin` 도 `${CORE_OUT}` 로 보낸다. `.bin` 은 POST_BUILD 에서
`$<TARGET_FILE:...>` 로 입력을 받으므로 출력 경로만 지정하면 된다.

CM33 은 아직 스켈레톤이라 기본이 꺼져 있다.

```bash
cmake -S . -B build -G Ninja -DBUILD_CM33=ON   # 준비되면
```

FSP 소스는 한 벌만 두고 두 타겟이 각각 다른 `-mcpu` 로 다시 컴파일한다. 프로젝트 코드는 `-Og -g3 -Wall`, FSP 와 생성물은 `-Os -w` 로 따로 간다.

## 3. 링크 — 반드시 지켜야 하는 두 가지

### ① `bsp_linker_info.h` 는 프로젝트 루트에 생성된다

RASC 가 `ra_gen/` 이 아니라 **루트**에 만든다. 이 디렉터리를 include path 에 넣지 않으면 `bsp_linker.c` 컴파일이 깨진다.

```cmake
target_include_directories(${EXECUTABLE} PRIVATE
  ${CORE_SDK}          # ← bsp_linker_info.h
  ${CORE_SDK}/ra_cfg/fsp_cfg
  ...
```

### ② `fsp.ld` 는 스텁이다 — `-L` 이 필요하다

`script/fsp.ld` 는 6줄짜리다.

```
INCLUDE memory_regions.ld
INCLUDE fsp_gen.ld
```

실제 내용은 이 둘에 있고, 둘은 `ra_sdk/cm85/` 루트에 있다. GNU ld 가 `INCLUDE` 를 찾으려면 그 경로가 `-L` 에 있어야 한다.

```cmake
target_link_directories(${EXECUTABLE} PRIVATE
  ${CORE_SDK}
  ${CORE_SDK}/script
  )
```

RA4M1-CORE 에 `-L src/lib/ra_sdk` 가 있는 것도 같은 이유다.

## 4. FSP 는 glob 하지 않는다

```cmake
file(GLOB_RECURSE RA_SDK_SRC_FILES CONFIGURE_DEPENDS
  ${RA_SDK_DIR}/ra/fsp/src/bsp/*.c
  ${RA_SDK_DIR}/ra/fsp/src/r_ioport/*.c
  )
```

전체를 glob 하면 설정 헤더가 없는 모듈 소스까지 잡혀 빌드가 깨진다. 모듈을 추가하는 절차는 [11-fsp-config.md](11-fsp-config.md#7-fsp-모듈을-추가할-때) 에 있다.

`ra_gen/main.c` 는 RASC 가 만든 것이라 제외한다. `main` 은 `src/cpu/cm85/main.c` 다.

```cmake
list(REMOVE_ITEM SRC_FILES_RA_GEN ${CORE_SDK}/ra_gen/main.c)
```

## 5. 크로스 플랫폼 규칙

빌드는 Windows / Linux / macOS 에서 모두 되어야 한다. 실제로는 macOS 에서만 돌려 볼 수 있으므로, **아래 정적 규칙으로 담보한다. 기능을 추가할 때도 이 규칙을 지킨다.**

| 규칙 | 이유 |
|---|---|
| CMake 안의 경로는 전부 `${CMAKE_CURRENT_SOURCE_DIR}` 기준, 구분자는 `/` 만 | 절대 경로는 다른 기계에서 깨진다 |
| 제너레이터는 **Ninja** 로 통일 | OS별 `tasks.json` 오버라이드가 필요 없어진다 |
| **`.sh` / `.bat` 를 만들지 않는다** | 적재는 `flash` CMake 타겟, 이미지 생성은 `mkimage.py`(python3) |
| POST_BUILD 는 `${CMAKE_OBJCOPY}` / `${Python3_EXECUTABLE}` 만 | 셸 파이프·리다이렉션은 셸마다 다르다 |
| `tasks.json` 은 `"type": "process"` + 인자 배열 | 셸 문법 차이를 피한다 |
| 툴체인은 환경변수 `ARM_TOOLCHAIN_DIR` 가 1순위, 없으면 PATH | 하드코딩된 경로에 의존하지 않는다 |
| `.gitattributes` 에 `text=auto eol=lf`, `*.ld` 는 LF 고정 | 링커 스크립트의 CRLF 는 문제를 일으킨다 |

> ### 함정 — Ninja 를 쓰는데 `CMAKE_MAKE_PROGRAM` 에 make 가 들어가면 안 된다
>
> 씨앗이 된 `arm-none-eabi-gcc.cmake` 는 무조건 `find_program(CMAKE_MAKE_PROGRAM NAMES make ...)` 를 했다.
> `CMAKE_MAKE_PROGRAM` 은 **제너레이터가 쓰는 빌드 도구**라, Ninja 제너레이터에서는 CMake 가 여기서
> ninja 를 기대한다. make 가 들어가면 이런 에러가 난다.
>
> ```
> ... is less than the version of Ninja required by CMake (1.3).
> ```
>
> 그래서 Makefile 계열 제너레이터일 때만 찾도록 감쌌다.
>
> ```cmake
> if(CMAKE_GENERATOR MATCHES "Makefiles")
>     find_program(CMAKE_MAKE_PROGRAM NAMES make make.exe ...)
> endif()
> ```

## 6. 저장소에 넣지 않는 것

| 항목 | 크기 | 대신 |
|---|---|---|
| `Renesas.RA_DFP.*.pack` | 94 MB | 환경변수 `RENESAS_RA_TOOLS` 또는 `-DRA_DFP_PACK=` |
| `R7KA8P1AD.svd` | 21 MB | 〃 |
| `build/` | — | `.gitignore` |

`.gitignore` 에 `tools/*.pack`, `tools/*.svd` 가 있다. 로컬에 두고 싶으면 `tools/` 에 복사해도 되고, `flash.cmake` 가 거기를 먼저 찾는다.

## 7. VSCode

`prj/` 에 워크스페이스가 셋 있다.

| 파일 | 보이는 것 |
|---|---|
| `titan-mini-cm85.code-workspace` | CM33 관련 폴더가 숨겨진다 |
| `titan-mini-cm33.code-workspace` | CM85 관련 폴더가 숨겨진다 |
| `titan-mini.code-workspace` | 전체 |

`files.exclude` 와 `search.exclude` 양쪽에 상대 코어의 `src/cpu/<코어>` 와 `src/lib/ra_sdk/<코어>` 를 넣어서, 탐색기에서도 검색에서도 안 걸린다.

`.vscode/` 의 태스크는 `build-configure` / `build-build` / `build-clean` / `flash` 넷이고, 디버그 구성은 `Debug CM85 (pyOCD)` 와 `Attach CM85 (pyOCD)` 다. IntelliSense 는 `build/compile_commands.json` 이 끌고 간다.

## 8. 현재 빌드 결과

```
Memory region         Used Size  Region Size  %age Used
             RAM:        1210 B      1872 KB      0.06%
           FLASH:        4764 B         1 MB      0.45%
           SDRAM:           0 B       128 MB      0.00%
```

링크된 라이브러리가 `thumb/v8-m.main+dp/hard` 인지 확인한다. Cortex-M85 용이 맞다.

```bash
grep -oE '[^ ]*/thumb/[^ ]*/lib[a-z_]*\.a' build/cm85/titan-mini-cm85.map | sort -u
```

> RA4M1-CORE 의 `CMakeLists.txt` 는 RA4M1(Cortex-M4)인데 `-mcpu=cortex-m33 -mfpu=fpv4-sp-d16` 으로
> 빌드되고 있다. 맵 파일에 Armv8-M 라이브러리가 링크된 게 보인다. 링크되고 돌기는 하지만 틀린 설정이다.
> 여기서는 그 실수를 가져오지 않았다.
