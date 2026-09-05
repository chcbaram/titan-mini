# Titan Mini (RA8P1) 펌웨어 문서

`R7KA8P1KFLCAC` (Cortex-M85 CPU0 + Cortex-M33 CPU1 듀얼코어) 기반 Titan Mini 보드 펌웨어 개발 문서.

## 현재 상태 (2026-09-05)

| | |
|---|---|
| 보드 | Titan Mini HW:V1.0 · MCU `R7KA8P1KFLCAC` (289 BGA) |
| 디버거 | HSLink (CMSIS-DAP). 가상 시리얼이 보드 UART1(P707/P706)에도 연결됨 |
| 펌웨어 | `firmware/ra8p1-fw` — CM85 부팅 + **RGB LED 점멸 동작 확인** |
| FSP | v6.6.0, RASC 생성물 커밋 방식. 보드는 `board.custom` |
| 빌드 | FLASH 4,764 B / 1 MB (0.45%) · RAM 1,210 B / 1872 KB (0.06%) |
| CM33 | 스켈레톤. `BUILD_CM33=OFF` |
| 부트로더 | 설계만 완료([05](05-boot-architecture.md)). 구현은 40번 |

### 바로 다시 시작하기

```bash
export RENESAS_RA_TOOLS=~/hdd/tools/renesas-ra
cd firmware/ra8p1-fw

cmake -S . -B build -G Ninja
cmake --build build -j8
cmake --build build --target flash      # LED3 빨강 500ms 점멸
```

VSCode 는 `firmware/ra8p1-fw/prj/titan-mini-cm85.code-workspace` 를 연다.

연결이 안 되면 `pyocd list` 로 HSLink 가 보이는지 먼저 확인한다.
`Error probing AP#2` 와 `Invalid coresight component` 경고는 **정상**이다 — CPU1 이 아직 안 깨어난 것이다.

### 다음 작업

1. **UART + CLI** — UART1(P707/P706) + `logPrintf` + 부팅 배너 ([21](21-uart-cli.md) 예정)
   - HSLink 가상 시리얼이 이미 붙어 있어 케이블 추가가 필요 없다
   - FSP `r_sci_uart` 모듈 추가 절차는 [11장 7절](11-fsp-config.md#7-fsp-모듈을-추가할-때)
2. **FreeRTOS** — 이후 FatFs·lwIP·USB·LVGL 이 전부 전제하므로 일찍 넣는다. 포트는 FSP `rm_freertos_port`
3. **CM33 기동** — 파티션 매크로를 직접 정의해야 한다 ([04장 3절](04-dualcore.md#3-파티션-매크로를-직접-정의해야-한다))
4. GPIO / 버튼 / swtimer → MRAM / NVS → OSPI 플래시 → SDRAM → SD/FatFs

### 미해결 과제

| 과제 | 왜 중요한가 | 언제 |
|---|---|---|
| ITCM 재배치 실측 | MRAM 이 단일 뱅크라 부트로더 본체를 ITCM 에서 돌린다(설계 완료). 링크·복사가 실제로 되는지 확인 필요 | 40번 전 |
| CM33 파티션 값 확정 | MRAM/SRAM 분할 크기. 지금은 초안 | 23번 |
| SDRAM DQ0~DQ7 핀 | 회로도 파서가 일부를 놓쳤다 | 27번 |
| 오디오 DSIN 핀 | 〃. 카메라 VIO_D2 와 충돌한다 | 34번 |
| RA8P1 TinyUSB 포팅 유무 | 없으면 `r_usb_basic` + PCDC/PMSC 로 간다 | 35번 |

## 문서 번호 규칙

| 대역 | 성격 |
|---|---|
| `00~09` | 하드웨어 / 부팅 레퍼런스 — 데이터시트·매뉴얼·회로도에서 확정한 사실 |
| `10~19` | 개발 환경 / 프로젝트 구조 |
| `20~` | **기능별 구현 기록** — 기능 하나당 문서 하나 |

기능 문서에는 "무엇을 왜 그렇게 했는지 + 막혔던 지점 + 검증 방법" 을 남긴다. 코드만 봐서는 알 수 없는 것(RA8P1 특유의 제약, MRAM 특성, FSP/RASC 함정, 툴 버전 이슈)이 대상이다.

## 목차

### 레퍼런스

| 문서 | 내용 |
|---|---|
| [01-boot-sequence.md](01-boot-sequence.md) | 리셋 → CPU0 실행 → CPU1 기동. MD 핀, FSBL(OTP), FSP 스타트업 |
| [02-memory-map.md](02-memory-map.md) | 전체 주소 공간, MRAM 특성과 RWW 제약, MRAM/SRAM 파티션 계획 |
| [03-board-mapping.md](03-board-mapping.md) | 회로도에서 확정한 전 핀 매핑. 보드 부품 인벤토리 |
| [04-dualcore.md](04-dualcore.md) | CPU1 기동 시퀀스, 파티션 매크로, IPC, D-cache 규칙 |
| [05-boot-architecture.md](05-boot-architecture.md) | 부트로더/펌웨어 분리, 섹션 컨테이너 이미지, 업데이트 흐름 |

### 개발 환경

| 문서 | 내용 | 상태 |
|---|---|---|
| [10-dev-environment.md](10-dev-environment.md) | 툴 경로, pyOCD + HSLink, DFP 팩 함정, 핀 조회 | ✅ |
| [11-fsp-config.md](11-fsp-config.md) | 생성물 커밋 방식, custom board 전환, 재생성 절차, 모듈 추가 | ✅ |
| [12-project-skeleton.md](12-project-skeleton.md) | 디렉터리·CMake 구조, 링크 주의점, 크로스 플랫폼 규칙 | ✅ |

### 구현 기록

| 문서 | 기능 | 참조 구현 | 상태 |
|---|---|---|---|
| [20-led.md](20-led.md) | RGB LED + 빌드/적재/검증 루프 | — | ✅ |
| `21-uart-cli.md` | UART + log/cli | `weact-h750-mini` (2026-08-30), `qmk-link` cli.c | 예정 |
| `22-freertos.md` | FreeRTOS + 스레드 OSAL | `stm32h7-lvgl` `bsp/rtos/`, FSP `rm_freertos_port` | 예정 |
| `23-cm33-boot.md` | CM33 기동, 코어간 IPC | FSP `bsp_ipc.c`, `NU87-TinyDK` ipc.c | 예정 |
| `24-gpio-button-swtimer.md` | GPIO / 버튼 / 소프트타이머 | `weact-h750-mini`, `qmk-link` | 예정 |
| `25-mram-nvs.md` | 내부 MRAM R/W + 설정 저장 | `NU87-TinyDK` nvs.c, FSP `mram` 예제 | 예정 |
| `26-ospi-flash.md` | OSPI NOR (W25Q64 8 MB, CS1) | `weact-h750-mini` qspi.c, FSP `ospi_b` 예제 | 예정 |
| `27-sdram.md` | SDRAM 32 MB + 힙 | `stm32h7-lvgl` sdram.c·mem.c | 예정 |
| `28-sdcard-fatfs.md` | SDHI + FatFs + 파일 API | `stm32h7-lvgl` sd.c·fatfs.c·fs.c·files.c | 예정 |
| `29-i2c-imu.md` | I2C + LSM6DS3TR-C | `stm32h7-lvgl` i2c.c (IMU 는 신규) | 예정 |
| `30-spi.md` | SPI 마스터 | `weact-h750-mini` spi.c | 예정 |
| `31-lcd-glcdc.md` | GLCDC RGB565 출력 | `stm32h7-lvgl` ltdc.c·lcd.c | 예정 |
| `32-touch.md` | 정전식 터치 | `stm32h7-lvgl` touch.c | 예정 |
| `33-lvgl.md` | LVGL 포팅 | `stm32h7-lvgl` lvgl.c | 예정 |
| `34-audio.md` | I2S DAC + PDM 마이크 + 믹서 | `stm32h7-lvgl` i2s.c·pdm.c·mixer.c | 예정 |
| `35-usb.md` | USB CDC / MSC | `weact-h750-mini` usb/ | 예정 |
| `40-bootloader.md` | **부트로더** (RTOS 없음, TinyUSB) | `weact-h750-boot` 전체 | 예정 |
| `41-fw-update.md` | 섹션 컨테이너 이미지 · 롤백 | 〃 | 예정 |
| `50-ethernet.md` | 기가비트 이더넷 + lwIP | `NUCLEO-C5A3ZG` eth/ | 예정 |
| `51-can.md` | CAN FD | `master_prime/rmc-gfx-r2` can.c | 예정 |
| `52-camera.md` | MIPI CSI / 병렬 CEU | 신규 | 예정 |
| `53-npu.md` | Ethos-U55 NPU | 신규 | 예정 |

**순서 근거** — UART/CLI 가 이후 모든 검증 수단이라 먼저다. FreeRTOS 는 FatFs·lwIP·USB·LVGL 이 전부 전제하므로 일찍 넣는다(늦게 넣으면 만든 드라이버를 스레드 안전하게 다시 손봐야 한다). CM33 은 링커 파티션이 구조에 영향을 주므로 앞쪽. SDRAM 이 LCD 프레임버퍼·LVGL·카메라의 전제, SD/FatFs 가 파일 기반 기능의 전제다. 부트로더는 MRAM·OSPI·USB 가 갖춰진 뒤이고, 그게 생기면 pyOCD 없이 업데이트가 되므로 개발 사이클이 긴 대형 기능을 그 뒤로 미룬다.

## 그림

| 그림 | 설명 |
|---|---|
| [boot-sequence.svg](images/boot-sequence.svg) | 리셋 → MD 판정 → FSP 스타트업 → main() → CPU1 |
| [memory-map.svg](images/memory-map.svg) | 전체 주소 공간 |
| [mram-layout.svg](images/mram-layout.svg) | MRAM 1 MB 파티션 계획과 하드웨어 제약 |
| [sram-partition.svg](images/sram-partition.svg) | SRAM 분할, 코어별 TCM, non-cacheable 공유 영역 |
| [dualcore-start.svg](images/dualcore-start.svg) | CPU1 기동 순서와 IPC 핸드셰이크 |
| [fw-image-format.svg](images/fw-image-format.svg) | 섹션 컨테이너 이미지 포맷 |
| [update-flow.svg](images/update-flow.svg) | 수신 → QSPI 스테이징 → 검증 → MRAM 기록 → 롤백 |

> 그림은 전부 SVG 다. 한글이 2칸 폭이라 코드블록 ASCII 아트는 정렬이 깨진다.

손으로 쓴 SVG 라 브라우저로 열기 전에는 글자가 박스를 넘거나 서로 겹치는 것을 알기 어렵다.
그림을 고치면 검사기를 돌린다 — viewBox 이탈 · 텍스트 겹침 · 박스 밖 삐져나옴을 잡는다.

```bash
python3 firmware/docs/check_svg.py
```

## 출처

| 약칭 | 문서 | 비고 |
|---|---|---|
| HW 매뉴얼 | RA8P1 Group User's Manual: Hardware | 텍스트 추출본 `$RENESAS_RA_TOOLS/docs/.text/ra8p1-hardware-manual.txt` |
| 데이터시트 | RA8P1 Datasheet | SRAM 크기는 [02장 2절](02-memory-map.md#2-sram-크기--세-군데가-서로-다르다) 참고 — 매뉴얼이 맞다 |
| 회로도 | `hardware/Titan_Mini_schematic_v1.0.pdf` | KiCad 10.0.0, HW:V1.0, 2026-03-26, 13페이지 |
| FSP | [renesas/fsp](https://github.com/renesas/fsp) v6.6.0 | 로컬 `$RENESAS_RA_TOOLS/fsp` |
| FSP 예제 | [renesas/ra-fsp-examples](https://github.com/renesas/ra-fsp-examples) | 로컬 `$RENESAS_RA_TOOLS/examples/ek_ra8p1/` (102종) |
| 핀 매핑 | `$RENESAS_RA_TOOLS/device/ra8p1_mcu/.mcu/.pinmapping/PinCfgR7KA8P1KxxCAC.xml` | 패키지별 정본 |

### 참조한 기존 프로젝트

레이어 규약(`main → bsp → hw → ap`, `_USE_HW_*`, `common/hw/include`)과 드라이버는 기존 저장소에서 가져온다. 각 기능마다 **파일 단위 마지막 커밋일이 가장 최근인 구현본**을 참조한다.

| 저장소 | 무엇을 |
|---|---|
| `NUCLEO-N657X0` | 레이어 규약, `src/common/*`, `.clang-format`, 툴체인 CMake |
| `RA4M1-CORE` | FSP vendoring 방식, IOPORT 드라이버 패턴, `bsp.c` 구조 |
| `stm32h7-gfx/firmware/stm32h7-lvgl` (2026-07~08) | SDRAM · SD · FatFs · I2S · PDM · 터치 · LVGL · mem |
| `weact-h750-mini` (2026-08-30) | UART · log · GPIO · RTC · flash · qspi · spi · USB · **부트로더** |
| `NUCLEO-C5A3ZG` (2026-07) | 이더넷(lwIP), `osal/thread.h` |
| `qmk-link` (2026-08) | cli · swtimer |
| `NU87-TinyDK` (2026-08) | nvs · ipc |
| `master_prime/rmc-gfx-r2` | CAN |
