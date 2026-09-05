# 13. 새 PC 에서 환경 구축

> 저장소를 처음 받은 기계에서 빌드 · 적재 · 디버깅까지 되게 만드는 절차.
> 운영체제별로 나눠 두었으니 자기 것만 따라가면 된다.
>
> 각 도구가 무엇이고 왜 필요한지는 [10-dev-environment.md](10-dev-environment.md) 에 있다.
> 여기서는 순서만 다룬다.

> ### 검증 범위
>
> **macOS(Apple Silicon) 절차는 실제로 이 환경에서 동작을 확인한 것**이다.
> Windows 와 Linux 절차는 공식 배포 방식을 따라 정리했지만 **실기 검증은 하지 않았다.**
> 막히는 곳이 있으면 이 문서를 고쳐 가며 쓴다.

---

## 0. 무엇이 필요한가

| 도구 | 용도 | 필수 |
|---|---|---|
| Arm GNU Toolchain | 컴파일. **Cortex-M85 지원이 필요해 13.2 이상** | ✅ |
| CMake 3.20+ | 빌드 구성 | ✅ |
| Ninja | 빌드 실행 | ✅ |
| Python 3.9+ | 준비 스크립트, 검사 스크립트 | ✅ |
| pyOCD | 적재와 디버그 | 적재할 때 |
| Git | 저장소 | ✅ |
| VSCode + 확장 2개 | 편집과 디버깅 | 선택 |

**FSP 는 설치하지 않는다.** 저장소에 vendoring 되어 있다. RASC 로 FSP 설정을 다시 생성할 때만
따로 받는다([11-fsp-config.md](11-fsp-config.md)).

하드웨어는 **HSLink 디버거**(CMSIS-DAP)와 보드다. 드라이버 설치가 필요한지는 6장에 있다.

---

## 1. macOS

Apple Silicon / Intel 공통. [Homebrew](https://brew.sh) 를 쓴다.

### 1-1. 도구 설치

```bash
# Homebrew 가 없다면
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

brew install cmake ninja python git
brew install --cask gcc-arm-embedded      # Arm GNU Toolchain
python3 -m pip install --user pyocd pyserial
```

`--cask gcc-arm-embedded` 는 `/Applications/ArmGNUToolchain/<버전>/` 에 설치하고
`/opt/homebrew/bin/arm-none-eabi-*` 로 심볼릭 링크를 걸어 준다. 확인:

```bash
$ arm-none-eabi-gcc --version
arm-none-eabi-gcc (Arm GNU Toolchain 14.2.Rel1 (Build arm-14.52)) 14.2.1 20241119
```

> `pip install --user` 로 넣은 실행 파일이 PATH 에 없으면
> `python3 -m site --user-base` 의 `bin` 을 PATH 에 추가한다.
> Homebrew 파이썬을 쓰면 `/opt/homebrew/bin` 에 바로 들어간다.

### 1-2. 저장소와 자산

```bash
git clone https://github.com/chcbaram/titan-mini
cd titan-mini
python3 tools/setup_tools.py
```

### 1-3. 환경변수

**`~/.zshrc` 에 넣는다.** 터미널에서 그때그때 `export` 하면 GUI 로 띄운 VSCode 에 넘어가지
않아 디버깅이 안 된다.

```bash
echo 'export RENESAS_RA_TOOLS="$HOME/hdd/tools/renesas-ra"' >> ~/.zshrc
```

경로는 `setup_tools.py` 가 마지막에 알려주는 것을 쓴다. 넣은 뒤 **VSCode 를 완전히 종료
(Cmd+Q) 후 다시 띄운다.** macOS 는 앱 실행 시 로그인 셸 환경을 한 번만 읽는다.

### 1-4. 시리얼 콘솔

```bash
ls /dev/cu.usbmodem*
screen /dev/cu.usbmodem1412302 115200      # 나갈 때 Ctrl-A K
```

---

## 2. Windows

> 실기 검증하지 않았다.

### 2-1. 도구 설치

**Arm GNU Toolchain** — [developer.arm.com](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
에서 `arm-gnu-toolchain-<버전>-mingw-w64-x86_64-arm-none-eabi.exe` 를 받아 설치한다.
설치 마지막에 **"Add path to environment variable"** 을 체크한다.

나머지는 [winget](https://learn.microsoft.com/windows/package-manager/) 으로 받는 게 편하다.

```powershell
winget install Kitware.CMake
winget install Ninja-build.Ninja
winget install Python.Python.3.12
winget install Git.Git
pip install pyocd pyserial
```

PowerShell 을 새로 열고 확인한다.

```powershell
arm-none-eabi-gcc --version
cmake --version
ninja --version
pyocd --version
```

### 2-2. 저장소와 자산

```powershell
git clone https://github.com/chcbaram/titan-mini
cd titan-mini
python tools\setup_tools.py --dir C:\tools\renesas-ra
```

### 2-3. 환경변수

**사용자 환경 변수로 등록한다.** 그래야 GUI 프로그램에 전달된다.

```powershell
[Environment]::SetEnvironmentVariable('RENESAS_RA_TOOLS', 'C:\tools\renesas-ra', 'User')
```

또는 `설정 > 시스템 > 정보 > 고급 시스템 설정 > 환경 변수`.

**등록 후 VSCode 와 터미널을 모두 다시 띄운다.**

### 2-4. 시리얼 콘솔

`장치 관리자 > 포트(COM & LPT)` 에서 HSLink 의 COM 번호를 확인하고
[PuTTY](https://www.putty.org/) 나 [Tera Term](https://teratermproject.github.io/) 으로
**115200 8N1** 로 연다.

### 2-5. Windows 특이사항

- **경로 구분자** — 이 저장소의 CMake 는 `/` 만 쓰고 절대 경로를 두지 않는다.
  빌드에는 문제가 없다([12-project-skeleton.md](12-project-skeleton.md#5-크로스-플랫폼-규칙)).
- **줄바꿈** — `.gitattributes` 가 `text=auto eol=lf` 로 잡아 둔다. `*.ld` 도 LF 고정이다.
  `core.autocrlf` 를 따로 만지지 않는다.
- **제너레이터** — Ninja 로 통일한다. `-G Ninja` 를 빼면 Visual Studio 제너레이터가 잡혀
  크로스 컴파일이 깨진다.
- **USB 드라이버** — HSLink 는 CMSIS-DAP(WinUSB)라 보통 그대로 잡힌다. 안 잡히면
  [Zadig](https://zadig.akeo.ie/) 로 WinUSB 드라이버를 씌운다.

---

## 3. Linux (Ubuntu / Debian)

> 실기 검증하지 않았다.

### 3-1. 도구 설치

배포판 저장소의 `gcc-arm-none-eabi` 는 버전이 낮아 **Cortex-M85 를 지원하지 않는 경우가
많다.** Arm 공식 배포본을 받는다.

```bash
sudo apt update
sudo apt install cmake ninja-build python3 python3-pip git

# Arm GNU Toolchain 14.2 (x86_64)
ARM_VER=14.2.rel1
wget https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_VER}/binrel/arm-gnu-toolchain-${ARM_VER}-x86_64-arm-none-eabi.tar.xz
sudo tar xf arm-gnu-toolchain-${ARM_VER}-x86_64-arm-none-eabi.tar.xz -C /opt
echo 'export PATH="/opt/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin:$PATH"' >> ~/.bashrc

pip3 install --user pyocd pyserial
```

aarch64 호스트면 파일 이름의 `x86_64` 를 `aarch64` 로 바꾼다.

새 셸을 열고 확인한다.

```bash
arm-none-eabi-gcc --version    # 13.2 이상이어야 한다
```

### 3-2. USB 권한

디버거에 접근하려면 udev 규칙이 필요하다. 없으면 `pyocd list` 가 프로브를 못 본다.

```bash
sudo curl -fsSL https://raw.githubusercontent.com/pyocd/pyOCD/main/udev/50-cmsis-dap.rules \
  -o /etc/udev/rules.d/50-cmsis-dap.rules
sudo udevadm control --reload-rules && sudo udevadm trigger
```

시리얼 포트는 `dialout` 그룹이다. 넣은 뒤 **로그아웃/로그인** 해야 반영된다.

```bash
sudo usermod -aG dialout $USER
```

### 3-3. 저장소와 자산

```bash
git clone https://github.com/chcbaram/titan-mini
cd titan-mini
python3 tools/setup_tools.py
```

### 3-4. 환경변수

```bash
echo 'export RENESAS_RA_TOOLS="$HOME/tools/renesas-ra"' >> ~/.bashrc
```

경로는 `setup_tools.py` 가 알려주는 것을 쓴다. **VSCode 를 데스크톱에서 띄운다면 다시
로그인해야** 환경변수가 반영된다. 터미널에서 `code .` 로 띄우면 바로 상속된다.

### 3-5. 시리얼 콘솔

```bash
ls /dev/ttyACM*
screen /dev/ttyACM0 115200        # 나갈 때 Ctrl-A K
```

---

## 4. 공통 — 빌드와 적재

여기부터는 세 OS 가 같다.

```bash
cd firmware/ra8p1-fw

cmake -S . -B build -G Ninja
cmake --build build -j8
cmake --build build --target flash
```

성공하면 이렇게 나온다.

```
Memory region         Used Size  Region Size  %age Used
             RAM:       78332 B      1408 KB      5.43%
           FLASH:       38832 B       768 KB      4.94%
      DATA_FLASH:           0 B          0 B
           SDRAM:           0 B       128 MB      0.00%
```

`--target flash` 는 이렇게 끝난다.

```
I Loading .../cm85/titan-mini-cm85.elf
I Erased 65536 bytes (2 sectors), programmed 40960 bytes (5 pages), skipped 0 bytes
```

보드의 RGB LED 가 **500 ms 주기로 점멸**하고, 시리얼에 배너가 뜬다.

```
[ Firmware Begin... ]
Booting..Name  		: TITAN-MINI-CM85
Booting..Ver   		: V260905R1
Booting..Clock 		: 1000 MHz

cli#
```

`help` 를 쳐서 명령 목록이 나오면 끝이다.

### CM33(CPU1) 까지 올리려면

```bash
cmake -S . -B build -G Ninja -DBUILD_CM33=ON
cmake --build build -j8
cmake --build build --target flash    # 두 이미지를 함께 올린다
```

적재 로그에 `.elf` 가 두 번 나오면 맞다.

```
I Loading .../cm85/titan-mini-cm85.elf
I Erased 65536 bytes (2 sectors), programmed 40960 bytes (5 pages)
I Loading .../cm33/titan-mini-cm33.elf
I Erased 32768 bytes (1 sector), programmed 8192 bytes (1 page)
```

CM33 이 실제로 도는지는 CLI 로 본다. `state` 가 `RUNNING` 이고 `alive` 증분이 0 이 아니어야
한다. `BUILD_CM33=OFF` 로 빌드했다면 `DISABLED` 가 나온다 — CPU0 이 CPU1 을 아예 깨우지
않는다.

```
cli# ipc info
peer       : CPU1 (Cortex-M33)
image      : 있음
state      : RUNNING
boot time  : 1 ms
alive      : 69  (+4 / 500ms)
tick       : 8571 ms  (+504)
running    : 예
```

---

## 5. 공통 — VSCode

### 5-1. 확장

두 개만 있으면 된다.

```bash
code --install-extension ms-vscode.cpptools
code --install-extension marus25.cortex-debug
```

### 5-2. 워크스페이스

`firmware/ra8p1-fw/prj/` 의 것을 연다. 코어별로 나뉘어 있어서, 한 코어로 작업할 때 상대
코어 폴더가 탐색기와 검색 양쪽에서 안 보인다.

| 파일 | 보이는 것 |
|---|---|
| `titan-mini-cm85.code-workspace` | CM33 관련 폴더가 숨겨진다 |
| `titan-mini-cm33.code-workspace` | CM85 관련 폴더가 숨겨진다 |
| `titan-mini.code-workspace` | 전체 |

### 5-3. 빌드 태스크

`Cmd+Shift+B`(Windows/Linux 는 `Ctrl+Shift+B`)로 빌드한다. 태스크는 넷이다 —
`build-configure` / `build-build` / `build-clean` / `flash`.

IntelliSense 는 `build/<코어>/compile_commands.json` 이 끌고 간다. **한 번 빌드해야**
생기므로, 워크스페이스를 처음 열면 먼저 빌드하고 나서 코드를 본다. 코어별 설정은
`.code-workspace` 에 들어 있어 클론만 하면 따라온다 — 따로 만질 것이 없다.

### 5-4. 디버깅

`실행 및 디버그` 에서 **Debug CM85 (pyOCD)** 를 고르고 F5. `main` 에서 멈춘다.

`launch.json` 이 `${env:RENESAS_RA_TOOLS}` 를 쓰므로 **환경변수가 GUI 에 전달돼야 한다.**
안 되면 3장(OS별)의 환경변수 항목을 다시 본다.

---

## 6. 막혔을 때

### `pyocd list` 에 프로브가 안 보인다

```
$ pyocd list
  #   Probe/Board                     Unique ID                          Target
---------------------------------------------------------------------------------
  0   CherryUSB CherryUSB CMSIS-DAP   3E49097AFCE4040CEAB0323B56D6C0F2   n/a
```

이렇게 나와야 한다. 안 나오면 USB 케이블 · 허브 · (Linux) udev 규칙 · (Windows) 드라이버를 본다.

### `RA DFP 팩을 찾지 못했다` 경고

configure 때 이 경고가 나오면 환경변수가 안 잡힌 것이다.

```bash
python3 tools/setup_tools.py --check
```

로 확인하고, `RENESAS_RA_TOOLS` 를 셸 프로파일에 넣었는지 본다.

### 디버깅이 `No such file or directory: '/packs/...'` 로 실패

`${env:RENESAS_RA_TOOLS}` 가 빈 문자열이라 앞부분이 날아간 것이다. **터미널에서 export 한
값은 GUI 로 띄운 VSCode 에 넘어가지 않는다.** 셸 프로파일(또는 Windows 사용자 환경 변수)에
넣고 VSCode 를 완전히 종료 후 다시 띄운다.

### 정상인 경고들

아래는 고장이 아니다.

| 메시지 | 이유 |
|---|---|
| `Error probing AP#2: SWD/JTAG communication failure (WAIT ACK)` | CPU1 이 아직 안 깨어났다 |
| `Invalid coresight component, cidr=0x5900d00` | ROM 테이블 파싱 경고 |
| `Error during board uninit` / `Probe error during disconnect` | 종료 시점. 기록은 이미 끝났다 |
| `_close is not implemented and will always fail` | newlib nosys 스텁. 링크 경고다 |
| `AttributeError: 'NoneType' object has no attribute 'group'` (pyocd `svd/parser.py`) | pyOCD 0.39.0 의 SVD 파서가 RA8P1 SVD 를 못 읽는다. 별도 스레드라 적재는 그대로 끝난다 |

### 시리얼에 아무것도 안 온다

포트를 두 프로그램이 동시에 열 수 없다. `screen` 이나 `minicom` 을 띄워 둔 채 다른
스크립트로 같은 포트를 열면 이렇게 된다.

```
SerialException: device reports readiness to read but returned no data
                 (device disconnected or multiple access on port?)
```

macOS/Linux 는 `lsof /dev/cu.usbmodem*` 로 누가 잡고 있는지 확인한다.

### 그 밖

[10-dev-environment.md](10-dev-environment.md) 2장에 DFP 팩과 pyOCD 의 알려진 결함이 정리돼 있다.
