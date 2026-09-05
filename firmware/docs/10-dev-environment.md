# 10. 개발 환경

> 툴 경로, 보드 연결, 빌드·적재 확인. 여기서 걸린 함정은 전부 적어 둔다.
> 관련: [11-fsp-config.md](11-fsp-config.md) · [12-project-skeleton.md](12-project-skeleton.md)

---

## 1. 툴 목록

전부 `~/hdd/tools/renesas-ra/` 아래에 정리돼 있다. 환경변수 하나로 참조한다.

```bash
export RENESAS_RA_TOOLS=~/hdd/tools/renesas-ra
```

| 항목 | 경로 / 버전 |
|---|---|
| arm-none-eabi-gcc | `/opt/homebrew/bin/arm-none-eabi-gcc` → Arm GNU Toolchain **14.2.Rel1** |
| CMake / Ninja | `/opt/homebrew/bin` — CMake 4.4.3 / Ninja 1.13.2 |
| FSP 소스 | `$RENESAS_RA_TOOLS/fsp` — **v6.6.0** shallow clone (183 MB) |
| pyOCD | `/opt/homebrew/bin/pyocd` — **0.39.0** |
| DFP 팩 | `$RENESAS_RA_TOOLS/packs/Renesas.RA_DFP.6.6.0-fixed.pack` |
| SVD | `$RENESAS_RA_TOOLS/svd/R7KA8P1AD.svd` (21 MB, RA8P1 공용) |
| RASC (재생성용) | `$RENESAS_RA_TOOLS/bin/rasc-generate.sh`, `$RENESAS_RA_TOOLS/rasc/rasc.app` |
| FSP 예제 102종 | `$RENESAS_RA_TOOLS/examples/ek_ra8p1/` |
| 하드웨어 매뉴얼 | `$RENESAS_RA_TOOLS/docs/ra8p1/` — 텍스트 추출본 `docs/.text/ra8p1-hardware-manual.txt` (9.4 MB) |
| PDF 도구 | `$RENESAS_RA_TOOLS/bin/pdftotext`, `pdftoppm` (PyMuPDF 기반 대체품) |

**툴체인은 새로 설치하지 않는다.** 이미 있는 14.2.Rel1 을 쓴다. `-mcpu=cortex-m85 -mfpu=fpv5-d16` 컴파일이 확인됐다.

### 디버그 자산은 저장소에 넣지 않는다

DFP 팩(94 MB)과 SVD(21 MB)는 크기 때문에 커밋하지 않는다. 위치는 **환경변수
`RENESAS_RA_TOOLS` 하나로** 알려준다. CMake 와 `.vscode/launch.json` 이 같은 변수를 쓴다.

`.gitignore` 에 `tools/*.pack`, `tools/*.svd` 가 있어서 로컬에 복사해 두어도 커밋되지 않는다.

> ### 셸 프로파일에 넣어야 한다
>
> **터미널에서 그때그때 `export` 한 값은 GUI 로 띄운 VSCode 에 넘어가지 않는다.**
> 이걸 놓치면 `launch.json` 의 `${env:RENESAS_RA_TOOLS}` 가 빈 문자열이 되어,
> 앞부분이 통째로 날아간 경로가 pyocd 에 전달된다.
>
> ```
> pyocd gdbserver --pack /packs/Renesas.RA_DFP.6.6.0-fixed.pack
> Error: [Errno 2] No such file or directory: '/packs/Renesas.RA_DFP.6.6.0-fixed.pack'
> ```
>
> `~/.zshrc` (또는 `~/.bashrc`) 에 넣고 **VSCode 를 다시 띄운다.**
>
> ```bash
> export RENESAS_RA_TOOLS=~/hdd/tools/renesas-ra
> ```
>
> Windows 는 사용자 환경 변수로 등록하고 VSCode 를 재시작한다.

변수를 안 쓰고 직접 지정해도 된다.

```bash
cmake -S . -B build -G Ninja -DRA_DFP_PACK=/path/to/Renesas.RA_DFP.6.6.0-fixed.pack
```

변수가 없으면 configure 가 경고를 띄우고 `flash` 타겟은 실패한다. 조용히 넘어가면
나중에 pyocd 가 빈 경로를 받아 엉뚱한 에러를 내기 때문이다.

## 2. 함정 세 가지

### DFP 팩 원본은 깨져 있다

`MDK_Device_Packs_v6.6.0.zip` 안의 `Renesas.RA_DFP.6.6.0.pack` 은 **존재하지 않는 FLM 3개**(`RA8M1_2M_NS` / `RA8M1_CCONF_NS` / `RA8M1_DATA_C2M_NS`)를 참조한다. pyOCD 가 파싱 단계에서 통째로 죽는다.

```
Error: "There is no item named 'Flash/RA8M1_2M_NS.FLM' in the archive"
```

**반드시 `-fixed` 수정본을 쓴다.** `pyocd pack install` 로 받으면 6.5.1 이 오는데 같은 버그가 있을 가능성이 높다.

### 타깃 이름은 `r7ka8p1kf` 다

DFP 가 `Dname` 을 9자로 자른다. `R7KA8P1KFLCAC` → `R7KA8P1KF`.

### pyOCD 의 SVD 파서가 죽는다

pyOCD 0.39.0 이 `R7KA8P1AD.svd` 의 `dim` 표기에서 `AttributeError` 로 죽는다.

```
File ".../pyocd/debug/svd/parser.py", line 170, in _parse_registers
  dim_indices = range(int(m.group(1)), int(m.group(2)) + 1)
AttributeError: 'NoneType' object has no attribute 'group'
```

**백그라운드 스레드만 죽고 플래시 · 리셋 · 메모리 접근은 전부 정상이다.** pyocd 안에서 레지스터 심볼로 조회하는 기능만 못 쓴다. SVD 는 cortex-debug 의 `svdFile` 로만 물린다.

## 3. macOS 12 에서는 poppler 가 안 깔린다

Homebrew 가 macOS 12 용 bottle 제공을 중단해서 소스 빌드로 떨어지고, 그 빌드가 실패한다.

```
You are using macOS 12...
make: *** [all] Error 2
```

PyMuPDF 기반 대체 CLI 를 쓴다. 인터페이스가 같다.

```bash
$RENESAS_RA_TOOLS/bin/pdftotext [-f 첫] [-l 끝] in.pdf [out.txt|-]
$RENESAS_RA_TOOLS/bin/pdftoppm  [-r DPI] [-f 첫] [-l 끝] in.pdf 접두사
```

## 4. 보드 연결

HSLink 디버거가 USB 로 붙어 있고, **HSLink 의 가상 시리얼 포트가 보드의 UART1(P707/P706)에도 연결돼 있다.** 케이블 하나로 적재와 콘솔이 동시에 된다.

```bash
$ pyocd list
  #   Probe/Board                     Unique ID                          Target
---------------------------------------------------------------------------------
  0   CherryUSB CherryUSB CMSIS-DAP   3E49097AFCE4040CEAB0323B56D6C0F2   n/a
```

HSLink 는 CMSIS-DAP 펌웨어라 pyOCD 로 그대로 붙는다.

타깃 인식 확인:

```bash
pyocd cmd -t r7ka8p1kf --pack $RENESAS_RA_TOOLS/packs/Renesas.RA_DFP.6.6.0-fixed.pack
> show map
```

MRAM 1 MB @ `0x0200_0000`, SRAM @ `0x2200_0000` 이 보이면 된다.

> ### 정상인 경고 두 개
>
> ```
> Error probing AP#2: SWD/JTAG communication failure (WAIT ACK)
> Invalid coresight component, cidr=0x5900d00
> ```
>
> DFP 에 CPU0(M85)/CPU1(M33) 두 프로세서가 선언돼 있어 코어 AP 가 최소 둘인데,
> **CPU1 이 아직 기동되지 않아 응답이 없다.** 무해하다. [04-dualcore.md](04-dualcore.md#7-디버깅-시-알아둘-것) 참고.

## 5. 빌드와 적재

```bash
export RENESAS_RA_TOOLS=~/hdd/tools/renesas-ra
cd firmware/ra8p1-fw

cmake -S . -B build -G Ninja
cmake --build build -j8
cmake --build build --target flash
```

`flash` 타깃은 셸 스크립트가 아니라 CMake custom target 이라 세 OS 에서 같은 명령으로 돌아간다.
팩은 `-DRA_DFP_PACK=<경로>` 를 먼저 보고, 없으면 `$RENESAS_RA_TOOLS/packs/*-fixed.pack` 에서 찾는다.

VSCode 에서는 `prj/titan-mini-cm85.code-workspace` 를 연다. 상대 코어 폴더가 숨겨진다.

## 6. OpenOCD 는 쓸 수 없다

Homebrew 의 OpenOCD 0.12.0 에 Renesas RA 타깃 설정 파일이 아예 없다. pyOCD 가 막히면 폴백은 J-Link 검토다.

## 7. 핀 조회

데이터시트 표를 뒤지지 않고 RASC 의 핀 매핑 정본에서 바로 찾는 게 정확하다. 패키지별로 다르므로 이 파일을 본다.

```
$RENESAS_RA_TOOLS/device/ra8p1_mcu/.mcu/.pinmapping/PinCfgR7KA8P1KxxCAC.xml
```

`R7KA8P1KFLCAC` 는 289 BGA 다. 이 패키지에서는 OSPI 신호처럼 **갈 수 있는 핀이 하나씩뿐인 것들이 있어서**, 회로도에서 일부만 읽어도 나머지가 역으로 확정된다([03-board-mapping.md](03-board-mapping.md#5-외부-메모리)).
