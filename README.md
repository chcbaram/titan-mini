# titan-mini

르네사스 **RA8P1**(`R7KA8P1KFLCAC`) 듀얼코어 MCU 보드 펌웨어.

Cortex-M85 1 GHz(CPU0) + Cortex-M33(CPU1) + Ethos-U55 NPU. 보드에 실장된 하드웨어를
전부 구동하는 것이 목표다.

| | |
|---|---|
| MCU | `R7KA8P1KFLCAC` — 289 BGA, MRAM 1 MB, SRAM 1872 KB |
| 보드 | Titan Mini HW:V1.0 (회로도 `hardware/`) |
| SDK | Renesas FSP v6.6.0 (RASC 생성물 커밋, `board.custom`) |
| 툴체인 | Arm GNU Toolchain 14.2 · CMake + Ninja |
| 적재 | pyOCD + HSLink (CMSIS-DAP) |
| RTOS | FreeRTOS 11.1.0 |

## 구성

```
hardware/                       회로도 (KiCad PDF)
tools/setup_tools.py            외부 자산 준비 (툴체인 확인 + DFP 팩)
firmware/
├── docs/                       개발 문서 — 여기부터 읽는다
└── ra8p1-fw/                   펌웨어
    ├── CMakeLists.txt
    ├── prj/                    코어별 VSCode 워크스페이스
    ├── tools/                  툴체인 CMake · 적재 · 검사 스크립트
    └── src/
        ├── common/             코어 공유 — 포터블 코어와 드라이버 공개 헤더
        ├── cpu/                코어별 코드
        │   ├── cm85/           CPU0  main / ap / bsp / hw
        │   └── cm33/           CPU1
        └── lib/ra_sdk/         FSP 소스와 코어별 생성물
```

## 준비

빌드에 필요한 외부 자산을 받는다. 새 PC 에서 한 번만 하면 된다.

```bash
python3 tools/setup_tools.py
```

하는 일

| 항목 | 내용 |
|---|---|
| 호스트 도구 확인 | `arm-none-eabi-gcc`(Cortex-M85 지원 13.2 이상) · `cmake` · `ninja` · `pyocd` |
| DFP 팩 | Renesas 공식 릴리스에서 받아 **결함 두 개를 고쳐** 놓는다 (아래) |
| SVD | DFP 팩 안에 들어 있어 따로 받지 않는다 |

FSP 소스는 저장소에 이미 vendoring 되어 있어 **빌드에는 필요 없다.** RASC 재생성이나
예제 참조가 필요하면 `--with-fsp` 를 준다(약 180 MB).

```bash
python3 tools/setup_tools.py --check      # 확인만
python3 tools/setup_tools.py --with-fsp   # FSP 소스도
python3 tools/setup_tools.py --dir PATH   # 다른 폴더에
```

끝나면 안내하는 대로 셸 프로파일에 한 줄을 넣는다. **터미널에서 그때그때 `export` 하면
GUI 로 띄운 VSCode 에 넘어가지 않아 디버깅이 안 된다.**

```bash
export RENESAS_RA_TOOLS="$HOME/hdd/tools/renesas-ra"
```

> ### DFP 팩을 왜 고치는가
>
> Renesas 배포본을 그대로 재배포하지 않고 공식 릴리스에서 받아 스크립트가 고친다.
> 두 가지가 빠져 있다.
>
> **결함 1 — 없는 FLM 을 참조한다.** pdsc 가 `RA8M1_2M_NS.FLM` 등 세 개를 가리키는데
> 팩 안에 없다. pyOCD 가 파싱 단계에서 통째로 죽는다. 내용이 같은 non-NS 판을 복사한다.
>
> **결함 2 — 듀얼코어의 코어별 AP 매핑이 없다.** `<processor Pname="CPU0"/"CPU1">` 은
> 선언돼 있는데 어느 AP 에 붙는지가 없다. **CPU1 을 기동한 뒤부터** pyOCD 가 연결에서
> `KeyError: <APv1Address #2>` 로 죽는다. 자세한 것은
> [23장 8절](firmware/docs/23-cm33-boot.md#8-함정--cpu1-을-깨우면-pyocd-가-죽는다).

## 빌드

```bash
cd firmware/ra8p1-fw
cmake -S . -B build -G Ninja
cmake --build build -j8
cmake --build build --target flash
```

콘솔은 **115200 8N1**, HSLink 의 가상 시리얼이다.

```bash
screen /dev/cu.usbmodem* 115200      # 나갈 때 Ctrl-A K
```

VSCode 는 `firmware/ra8p1-fw/prj/titan-mini-cm85.code-workspace` 를 연다.

자세한 것은 **[firmware/docs/README.md](firmware/docs/README.md)** 를 본다 — 현재 상태,
다음 작업, 전체 문서 목차가 거기 있다.

## 문서

| 대역 | 성격 |
|---|---|
| `00~09` | 하드웨어 / 부팅 레퍼런스 — 데이터시트·매뉴얼·회로도에서 확정한 사실 |
| `10~19` | 개발 환경 / 프로젝트 구조 |
| `20~` | 기능별 구현 기록 — 기능 하나당 문서 하나 |

기능 문서에는 "무엇을 왜 그렇게 했는지 + 막혔던 지점 + 검증 방법" 을 남긴다.
코드만 봐서는 알 수 없는 것(RA8P1 특유의 제약, MRAM 특성, FSP/RASC 함정, 툴 버전 이슈)이 대상이다.

## 검사

```bash
python3 tools/setup_tools.py --check               # 툴체인과 자산이 갖춰졌는지
python3 firmware/ra8p1-fw/tools/check_layers.py    # ap/common 이 벤더 HAL 을 안 쓰는지
python3 firmware/docs/check_svg.py                 # 문서 그림의 글자 겹침/이탈
```

## 라이선스

이 저장소의 코드는 [LICENSE](LICENSE) 를 따른다.
`firmware/ra8p1-fw/src/lib/` 아래의 Renesas FSP 와 FreeRTOS 는 각자의 라이선스를 따른다.
