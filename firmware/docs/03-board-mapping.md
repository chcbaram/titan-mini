# 03. Titan Mini 보드 핀 매핑

> 이 문서의 모든 핀 번호는 `hardware/Titan_Mini_schematic_v1.0.pdf` 에서 직접 확인한 값이다.
> 회로도는 KiCad 10.0.0, `Rev: HW:V1.0`, `Date: 2026-03-26`, 13페이지다.
> 관련: [01-boot-sequence.md](01-boot-sequence.md) · [02-memory-map.md](02-memory-map.md)

---

## 1. 회로도 페이지 구성

| 페이지 | 시트 이름 | 주요 부품 | MCU 인터페이스 |
|---|---|---|---|
| 1 | POWER | TLV62569 벅, AP22814ASN-7 로드스위치, RT9013-33/18 LDO | `SYS_PWR_ON` |
| 2 | MCU_PWR | 메인 XTAL, 서브클럭, MIPI/USB 전원 | — |
| 3 | MCU_PORTS | **R7KA8P1KFLCAC** (BGA) | 전 핀 배선 |
| 4 | SDRAM | **W9825G6KH-6** (256 Mbit ×16 = **32 MB**) | SDRAM 컨트롤러, 16bit |
| 5 | USB | USBFS + USBHS, TS3USB221ARSER 먹스, CH443K | `USB_CH_SEL` |
| 6 | RGB_LCD | AFC01-S40FCA-00 40핀 FPC, **RGB565 병렬** + 정전식 터치 | GLCDC |
| 7 | AUDIO | **ES8156** I2S DAC, LPA2103AQVF 스피커 앰프, PDM 마이크 | SSI + PDM |
| 8 | CAMERA | MIPI CSI 커넥터 | MIPI CSI |
| 9 | GIG_ENET | **RTL8211F-CG** RGMII PHY (PHY addr 001) | ETHER + MDIO |
| 10 | IMU | **LSM6DS3TR-C** 6축, **I2C addr 0x6A** | I2C1 |
| 11 | MEMORY | **W25Q64JVSSIQ** 8 MB NOR + **SD_CARD_01A** 소켓 | OSPI(§5), SDHI |
| 12 | CAN | TJA1042TK/3/1J 트랜시버 | CANFD |
| 13 | INTERFACE | RGB LED, RESET/USER 버튼, JTAG(FTSH-105), RPi 40핀 헤더 | — |

---

## 2. LED — 구동 확인 완료

RGB LED 한 개(`LED3`, `XL-1615RGBC-RF`)다. 공통 애노드가 `+3V3` 에 물려 있어 **전부 active LOW** 다.

| 채널 | 네트 | 핀 | 직렬 저항 | `def.h` |
|---|---|---|---|---|
| RED | `LED_R` | **P109** | R80 2 KΩ | `_DEF_LED1` |
| GREEN | `LED_G` | **P108** | R81 12 KΩ | `_DEF_LED2` |
| BLUE | `LED_B` | **P110** | R82 10 KΩ | `_DEF_LED3` |

저항값이 색마다 다른 것은 RGB 밝기 균형을 맞추려는 것이다.

> ### 함정 — LED 가 RA 기본 디버그 핀 자리에 있다
>
> RA 계열은 보통 P108(SWDIO) / P109(SWO) / P110(TDI) / P300(SWCLK) 을 디버그 핀으로 쓴다.
> 이 보드는 **디버그를 P208~P211 대체 핀으로 뺐고** 그 자리를 LED 에 내줬다.
> `ra_gen/pin_data.c` 에서 P208~P211 만 `IOPORT_PERIPHERAL_DEBUG` 로 잡고
> P108~P110 은 GPIO 출력으로 잡아야 한다. 반대로 하면 LED 가 안 켜지거나 디버거가 떨어진다.
>
> 참고로 EK-RA8P1 도 디버그가 P208~P211 이라 FSP 참조 프로젝트의 디버그 핀 설정은 그대로 쓸 수 있다.

## 3. 디버그 / 부트

| 기능 | 네트 | 핀 | 비고 |
|---|---|---|---|
| SWCLK / TCK | `SWCLK/TCK` | **P211** | J6 (FTSH-105-01-L-DV), R4 10 KΩ 풀업 |
| SWDIO / TMS | `SWDIO/TMS` | **P210** | R3 10 KΩ 풀업 |
| SWO / TDO | `TDO/SWO` | **P209** | |
| TDI | `TDI` | **P208** | |
| RESET | `RESET#` | RES 핀 | SW1 푸시버튼 |
| 부트 모드 | `P201/MD` | **P201** | **SW2 (USER/BOOT 버튼)**, R83 47 KΩ / R85 1K5 |
| NMI | `P200/NMI` | **P200** | R5 10 KΩ 풀업 |

`MD` 핀의 의미는 [01-boot-sequence.md](01-boot-sequence.md) 참고. `MD=L` 로 리셋을 풀면 SCI/USB 부트 모드로 들어간다.

## 4. 시리얼 / 버스

| 기능 | 네트 | 핀 | 연결처 |
|---|---|---|---|
| SCI1 TX / RX | `TXD1` / `RXD1` | P707 / P706 | RPi 40핀 헤더 U18 (GPIO14/15 위치) |
| **SCI2 TX / RX** | `TXD2` / `RXD2` | **P801 / P802** | **H1 3핀 헤더 ← HSLink 가상 시리얼이 여기 연결돼 있다** |
| I2C0 SCL | `SCL0` | P410 | R1/R2 10 KΩ 풀업 |
| I2C0 SDA | `SDA0` | P409 | |
| I2C1 SCL | `SCL1` | **P512** | IMU + 정전식 터치. R3/R4 10 KΩ 풀업 |
| I2C1 SDA | `SDA1` | **P511** | |
| SPI1 SCK | `RSPCK1` | P102 | RPi 헤더. **OSPI0_OM_0_SIO4 와 겸용**이라 8비트 OSPI 는 쓸 수 없다 |
| SPI1 MOSI | `MOSI1` | P708 | |
| SPI1 MISO | `MISO1` | P709 | |
| SPI1 CS | `SSLB1` / `SSLB2` / `SSLB3` | P712 / P105 / P106 | |
| CAN TX | `CTX0` | P312 | TJA1042 |
| CAN RX | `CRX0` | P311 | |

**콘솔은 SCI2(H1)를 쓴다.** 회로도에 H1 이 `DNP`(미실장)로 표기돼 있지만 실물에는 배선돼 있고,
여기에 HSLink 의 가상 시리얼이 붙어 있다. 디버거 케이블 하나로 적재와 콘솔이 동시에 된다.
([21-uart-cli.md](21-uart-cli.md))

> P801/P802 는 OSPI0 의 `OM_0_DQS` · `OM_0_SIO6` 과 겸용이다. OSPI 를 8비트(옥탈)로 쓰려면
> 콘솔을 SCI1(40핀 헤더)로 옮겨야 한다.

## 5. 외부 메모리

### SDRAM — W9825G6KH-6 (32 MB, 16bit)

| 신호 | 핀 |
|---|---|
| A0~A4 | PA04, PA03, PA02, PA01, PA00 |
| A5~A9 | P503, P504, P505, P506, P507 |
| A10~A12 | P508, P509, P510 |
| BA0 / BA1 | P608 / PD00 |
| DQ0~DQ7 | (P609 = DQ7 확인, 나머지는 27번 단계에서 확정) |
| DQ8~DQ15 | PA11, PA12, PA13, PA14, P610, P611, P612, P613 |
| DQM0 / DQM1 | P614 / PA05 |
| RAS / CAS / WE | PA10 / PA09 / PA08 |
| CKE / SDCLK / SDCS | PA06 / PA15 / (미확정) |

### NOR 플래시 — W25Q64JVSSIQ (8 MB), **OSPI0 CS1**

회로도의 네트 이름은 `QSPI_*` 지만 **RA8P1 에는 QSPI 페리페럴이 없다**(`BSP_FEATURE_QSPI_IS_AVAILABLE == 0`).
OSPI(OSPI_B) 2유닛만 있고, 이 플래시는 **OSPI0 의 OM_0 채널, CS1** 에 붙어 있다.

이 패키지(289 BGA, `R7KA8P1KFLCAC`)에서 **OSPI 신호는 각각 갈 수 있는 핀이 하나씩뿐이다.**
선택지가 없으므로 회로도에서 직접 읽지 못한 핀도 역으로 확정된다.

| 신호 | 핀 | 회로도 네트 | 직렬 저항 |
|---|---|---|---|
| `OSPI0_OM_0_SCLK` | **P808** | `QSPI_SCK` | R33 36Ω |
| `OSPI0_OM_0_CS1` | **P104** | `QSPI_CE` | R34 36Ω |
| `OSPI0_OM_0_SIO0` | **P100** | `QSPI_SIO0` (DI) | R29 36Ω |
| `OSPI0_OM_0_SIO1` | **P803** | `QSPI_SIO1` (DO) | R30 36Ω |
| `OSPI0_OM_0_SIO2` | **P103** | `QSPI_SIO2` (WP#) | R31 36Ω |
| `OSPI0_OM_0_SIO3` | **P101** | `QSPI_SIO3` (HOLD#/RESET#) | R32 36Ω |

CS# 에 R28 10 KΩ 풀업이 있다.

**메모리 맵** — CS1 이므로 XIP 창은 **`0x9000_0000`** 이다. `0x8000_0000`(CS0) 이 아니다.
`memory_regions.ld` 의 `OSPI0_CS1_START` 와 일치한다.

**드라이버** — `r_ospi_b`, CS 는 `OSPI_B_DEVICE_NUMBER_1`.
W25Q64 는 옥탈이 아니라 쿼드이므로 `spi_flash_protocol_t` 를 `SPI_FLASH_PROTOCOL_1S_1S_1S`
(또는 `1S_1S_4S` / `1S_4S_4S` / `4S_4S_4S`) 중에서 고른다.
FSP 예제 `examples/ek_ra8p1/ospi_b/` 는 EK 가 옥탈 플래시를 CS0 에 달아서 `8D_8D_8D` 를 쓰는데,
프로토콜과 CS 번호만 바꾸면 된다.

<details>
<summary>OSPI0 OM_0 전체 핀 배정 (참고)</summary>

| 신호 | 핀 | | 신호 | 핀 |
|---|---|---|---|---|
| SCLK | P808 | | SIO0 | P100 |
| SCLKN | P809 | | SIO1 | P803 |
| CS0 | P107 | | SIO2 | P103 |
| CS1 | P104 | | SIO3 | P101 |
| DQS | P801 | | SIO4 | P102 |
| RESET | P106 | | SIO5 | P800 |
| ECSINT1 | P105 | | SIO6 | P802 |
| RSTO1 | P600 | | SIO7 | P804 |
| WP1 | P601 | | | |

OSPI1 OM_1 은 SCLK=P603 / SCLKN=P602, CS0=PC08 / CS1=PC05, DQS=P607,
SIO0~3 = PC01 / P605 / PC04 / PC02, SIO4~7 = PC03 / PC00 / P606 / P604 이다.

출처: `device/ra8p1_mcu/.mcu/.pinmapping/PinCfgR7KA8P1KxxCAC.xml` (RASC 핀 매핑 정본)

</details>

### SD 카드 — SD_CARD_01A 소켓 (SDHI)

| 신호 | 핀 |
|---|---|
| CLK | PD05 |
| CMD | PD04 |
| DAT0 ~ DAT2 | PD03 / PD02 / PD01 |
| DAT3 | (11페이지의 `CD/DAT3`) |
| CD (카드 검출) | PD07 |

VDD 라인에 C70 4.7 µF / C71 100 nF, DAT 라인에 R35/R36 47 KΩ 풀업, D6/D7 RClamp0524PATCT ESD 보호가 있다.

## 6. 디스플레이 / 카메라 / 오디오

### LCD (RGB565 병렬, 40핀 FPC)

| 신호 | 핀 |
|---|---|
| DATA0 ~ DATA3 | P914, P915, P903, P902 |
| DATA4 ~ DATA9 | P910, P911, P912, P913, P904, (P905) |
| DATA10 ~ DATA15 | PB07, PB06, PB05, PB01, PB04, PB03 |
| CLK | P515 |
| TCON0 / TCON1 / TCON3 | P806 / P805 / P513 |
| RST | P011 |
| 백라이트 | P303 |
| 터치 RST | P412 |
| 터치 IRQ | `CTP_IRQ_N` (핀 미확정) |

### 카메라 (병렬 CEU 배선)

| 신호 | 핀 |
|---|---|
| VIO_D0 ~ VIO_D1 | P400, P401 |
| VIO_D3 | P406 |
| VIO_D4 ~ VIO_D7 | P700, P701, P702, P703 |
| CLK / PCLK | P500 / P414 |
| HD / VD / FSIN | P415 / PB02 / P501 |
| RESET / PWDN | PB00 / P710 |
| CEU_EN | P013 |

8페이지에는 MIPI CSI 커넥터가 따로 있다. 병렬(CEU)과 MIPI 두 경로가 모두 배선돼 있는 것으로 보이며, 어느 쪽을 쓸지는 38번 단계에서 정한다.

### 오디오 (ES8156 I2S DAC + PDM 마이크)

| 신호 | 핀 |
|---|---|
| MCLK | PD06 |
| SCLK (BCLK) | P403 |
| LRCK | P404 |
| DSIN | P405 계열 (확정 필요) |
| PDM_CLK1 | P812 |
| PDM_DAT1 | P502 |

> ### 함정 — 오디오와 병렬 카메라는 동시에 못 쓴다
>
> 회로도 1페이지에 이런 메모가 있다.
>
> > `AUDIO_DSIN和VIO_D2冲突，需要切换`
> > (오디오 DSIN 과 카메라 VIO_D2 가 충돌하므로 전환이 필요하다)
>
> 34번(오디오)과 38번(카메라) 단계에서 `hw_def.h` 의 스위치를 배타로 만든다.

## 7. 이더넷 (RTL8211F-CG, RGMII)

| 신호 | 핀 |
|---|---|
| MDC / MDIO | PC11 / PC12 |
| PHY RESET | PA07 (`ET1_RESET`) |
| PHY INT | P107 (`ET1_INT`) |
| RGMII TX/RX | 36번 단계에서 확정 |

PHY 주소는 001, 외부 3.3 V 전원 + `CFG_EXT` 풀업 구성이다. TXC 지연 2 ns OFF / RXC 지연 2 ns ON 이 기본이다.

## 8. IMU (LSM6DS3TR-C)

| 항목 | 값 |
|---|---|
| 버스 | **I2C1** (SCL=P512, SDA=P511) |
| 주소 | **0x6A** (`SDO/SA0` 가 GND) |
| INT1 | `IMU_INT1` (핀 미확정) |
| CS | R77 0Ω 로 +3V3 에 고정 → I2C 모드 |

---

## 부록 — 회로도를 텍스트로 읽는 법

이 문서의 핀 번호는 PDF 의 텍스트 레이어를 직접 파싱해서 얻었다. KiCad 가 만든 PDF 는 `KiCadStroke0/1/2` Type3 폰트를 서브셋으로 쓰기 때문에 문자 코드가 그대로는 읽히지 않는다. 각 폰트의 `/ToUnicode` CMap 을 파싱해서 매핑을 만들고, 텍스트 위치는 `Tm` 이 아니라 `q ... cm BT ... Tj ET Q` 의 `cm` 행렬에서 뽑아야 한다. 같은 y 좌표의 항목을 모으면 "네트 이름 ↔ 핀 이름" 쌍이 복원된다.

`tools-bd` 세션이 준비한 PyMuPDF 기반 대체 CLI 로도 읽을 수 있다 (macOS 12 에서는 Homebrew poppler 가 빌드에 실패한다).

```bash
~/hdd/tools/renesas-ra/bin/pdftotext -f 3 -l 3 hardware/Titan_Mini_schematic_v1.0.pdf -
~/hdd/tools/renesas-ra/bin/pdftoppm  -r 200 -f 3 -l 3 hardware/Titan_Mini_schematic_v1.0.pdf /tmp/sch
```

"미확정" 으로 남긴 핀들은 해당 기능 구현 단계에서 페이지 이미지를 띄워 눈으로 확인하고 채운다.
