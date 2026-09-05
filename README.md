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

## 빌드

```bash
export RENESAS_RA_TOOLS=~/hdd/tools/renesas-ra   # 셸 프로파일에 넣는다

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
python3 firmware/ra8p1-fw/tools/check_layers.py   # ap/common 이 벤더 HAL 을 안 쓰는지
python3 firmware/docs/check_svg.py                # 문서 그림의 글자 겹침/이탈
```

## 라이선스

이 저장소의 코드는 [LICENSE](LICENSE) 를 따른다.
`firmware/ra8p1-fw/src/lib/` 아래의 Renesas FSP 와 FreeRTOS 는 각자의 라이선스를 따른다.
