#!/usr/bin/env python3
"""
계층 규칙 검사.

    python3 firmware/ra8p1-fw/tools/check_layers.py

이 프로젝트의 이식성은 "ap 는 벤더 HAL 을 모른다" 는 규칙 하나에 걸려 있다.
MCU 가 바뀌면 bsp 와 hw/driver 는 다시 쓰지만 ap 는 그대로 간다.

그런데 컴파일러는 이걸 막아주지 않는다. ap_def.h -> hw.h -> hw_def.h -> bsp.h
-> hal_data.h 로 FSP 심볼이 ap 까지 전부 보이기 때문이다. 그래서 여기서 검사한다.

  ap/         벤더 HAL 금지. hw/ 의 공용 API 만 쓴다
  hw/driver/  벤더 HAL 허용. 여기가 MCU 의존을 가두는 층이다
  bsp/        벤더 HAL 허용
  common/     벤더 HAL 금지. 다른 저장소와 그대로 공유하는 포터블 코드다
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent / 'src'

# FSP / CMSIS 벤더 심볼. 헤더 이름과 식별자 양쪽을 본다.
VENDOR = re.compile(
    r'\b('
    r'R_[A-Z][A-Za-z0-9_]*'       # R_IOPORT_Open, R_PORT1, R_BSP_SoftwareDelay
                                  #   FSP 함수는 R_SCI_B_UART_Write 처럼 대소문자가
                                  #   섞이므로 뒤쪽에 소문자를 허용해야 한다
    r'|FSP_[A-Z_]+'               # FSP_SUCCESS
    r'|fsp_err_t|fsp_[a-z_]+_t'
    r'|BSP_[A-Z0-9_]+'            # BSP_IO_PORT_01_PIN_09
    r'|bsp_[a-z0-9_]+_t'          # bsp_io_level_t
    r'|g_ioport|g_uart[0-9]+'     # ra_gen 인스턴스
    r'|uart_instance_t|uart_callback_args_t|sci_b_[a-z_]+'
    r')\b'
)
VENDOR_HEADER = re.compile(
    r'#\s*include\s*[<"]('
    r'hal_data\.h|common_data\.h|bsp_api\.h|r_[a-z0-9_]+\.h|renesas\.h|R7KA8P1.*\.h'
    r')[">]'
)

# 층별 규칙. 경로 접두어 -> 벤더 HAL 허용 여부
RULES = [
    ('cpu/cm85/ap',      False),
    ('cpu/cm33/ap',      False),
    ('common',           False),
    ('cpu/cm85/hw/driver', True),
    ('cpu/cm33/hw/driver', True),
    ('cpu/cm85/bsp',     True),
    ('cpu/cm33/bsp',     True),
    ('cpu/cm85/hw',      True),   # hw.c / hw_def.h
    ('cpu/cm33/hw',      True),
    ('cpu/cm85/main.c',  True),
    ('cpu/cm33/main.c',  True),
]


def strip_comments(text: str) -> list:
    """줄 번호를 유지한 채 주석을 지운다.

    주석 안의 매크로 이름까지 잡으면 오탐이 난다. 설계 근거를 설명하려면 벤더
    심볼을 언급할 수밖에 없다.
    """
    out = []
    in_block = False

    for line in text.splitlines():
        buf = []
        i = 0
        while i < len(line):
            if in_block:
                end = line.find('*/', i)
                if end < 0:
                    i = len(line)
                else:
                    in_block = False
                    i = end + 2
            elif line.startswith('/*', i):
                in_block = True
                i += 2
            elif line.startswith('//', i):
                break
            else:
                buf.append(line[i])
                i += 1
        out.append(''.join(buf))

    return out


def rule_for(rel: str):
    for prefix, allowed in RULES:
        if rel.startswith(prefix):
            return prefix, allowed
    return None, None


def main() -> int:
    problems = []
    checked = 0

    for path in sorted(ROOT.rglob('*')):
        if path.suffix not in ('.c', '.h'):
            continue

        rel = str(path.relative_to(ROOT))

        # 벤더 트리는 검사 대상이 아니다
        if rel.startswith('lib/'):
            continue

        prefix, allowed = rule_for(rel)
        if prefix is None or allowed:
            continue

        checked += 1
        for num, code in enumerate(strip_comments(path.read_text(errors='replace')), 1):
            if not code.strip():
                continue

            hit = VENDOR_HEADER.search(code) or VENDOR.search(code)
            if hit:
                problems.append(f'{rel}:{num}  {hit.group(1)}   {code.strip()[:70]}')

    for p in problems:
        print(f'  위반  {p}')

    print(f'\n벤더 HAL 금지 층 {checked}개 파일 검사, 위반 {len(problems)}건')
    if problems:
        print('\nap 와 common 은 MCU 가 바뀌어도 그대로 가야 한다.')
        print('벤더 호출은 hw/driver 나 bsp 로 내리고, ap 에는 공용 API 만 노출한다.')
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
