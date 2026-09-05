#!/usr/bin/env python3
"""
빌드와 적재에 필요한 외부 자산을 받아서 준비한다.

    python3 firmware/ra8p1-fw/tools/setup_tools.py            # 준비
    python3 firmware/ra8p1-fw/tools/setup_tools.py --check    # 확인만
    python3 firmware/ra8p1-fw/tools/setup_tools.py --with-fsp # FSP 소스도 (재생성용)

새 PC 에서 이 저장소를 받았을 때 한 번 돌리면 된다.

무엇을 왜 받는가
  DFP 팩   pyOCD 가 플래시 알고리즘과 메모리 맵을 여기서 읽는다. 적재와 디버그에 필수다.
  SVD      디버거의 레지스터 뷰. DFP 팩 안에 들어 있어 따로 받지 않는다.
  FSP 소스 저장소에 이미 vendoring 되어 있어 빌드에는 필요 없다. RASC 재생성과
           예제 참조용이라 --with-fsp 로만 받는다.

Renesas 배포본을 그대로 재배포하지 않는다. 공식 릴리스에서 받아 여기서 고친다.
무엇을 왜 고치는지는 아래 patch_pack() 에 적혀 있다.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import zipfile
from pathlib import Path

FSP_VERSION = '6.6.0'
FSP_TAG = f'v{FSP_VERSION}'
MDK_PACKS_URL = (
    f'https://github.com/renesas/fsp/releases/download/{FSP_TAG}'
    f'/MDK_Device_Packs_v{FSP_VERSION}.zip'
)
DFP_NAME = f'Renesas.RA_DFP.{FSP_VERSION}.pack'
DFP_FIXED = f'Renesas.RA_DFP.{FSP_VERSION}-fixed.pack'
SVD_NAME = 'R7KA8P1AD.svd'
FSP_REPO = 'https://github.com/renesas/fsp'

# 원본 pdsc 가 참조하지만 실제로 없는 FLM. pyOCD 가 파싱 단계에서 통째로 죽는다.
#   Error: There is no item named 'Flash/RA8M1_2M_NS.FLM' in the archive
# 내용은 non-NS 판과 같으므로 그대로 복사한다. 우리 타깃(RA8P1)은 쓰지 않는
# RA8M1 알고리즘이라 동작에 영향이 없고, 파싱만 통과시키면 된다.
MISSING_FLMS = {
    'Flash/RA8M1_2M_NS.FLM':        'Flash/RA8M1_2M.FLM',
    'Flash/RA8M1_CCONF_NS.FLM':     'Flash/RA8M1_CCONF.FLM',
    'Flash/RA8M1_DATA_C2M_NS.FLM':  'Flash/RA8M1_DATA_C2M.FLM',
}

# 듀얼코어 subFamily 에 코어별 AP 매핑을 넣는다. 원본에 없어서 CPU1 을 기동한 뒤부터
# pyOCD 가 연결에서 죽는다.
#   KeyError: <APv1Address #2 dp=0>   (pack_target.py:_pack_target_add_core)
# AP 번호는 실측값이다 - AHB-AP#0 에 SCS M85, AHB-AP#2 에 SCS M33.
#
# svd 를 두 줄 모두에 반복하는 것이 중요하다. Pname 있는 <debug> 가 하나라도 들어오면
# pyOCD 가 family 레벨의 Pname 없는 <debug> 를 담은 맵을 통째로 비우기 때문에,
# 생략하면 svd 경로가 None 이 되어 다른 곳에서 TypeError 가 난다.
DUALCORE_SUBFAMILIES = [
    'RA8P1_1M_DualCore',
    'RA8P1_1M_DualCore_SiP_8M',
    'RA8P1_1M_DualCore_SiP_4M',
]
AP_MAP_XML = (
    '\n\t\t\t\t<!-- pyOCD 코어별 AP 매핑. setup_tools.py 가 넣는다. -->'
    f'\n\t\t\t\t<debug Pname="CPU0" __ap="0" svd="SVD/{SVD_NAME}"/>'
    f'\n\t\t\t\t<debug Pname="CPU1" __ap="2" svd="SVD/{SVD_NAME}"/>'
)


def log(msg):
    print(f'  {msg}')


def default_dir() -> Path:
    env = os.environ.get('RENESAS_RA_TOOLS')
    if env:
        return Path(env).expanduser()
    return Path.home() / 'hdd' / 'tools' / 'renesas-ra'


# --------------------------------------------------------------- 호스트 도구 확인

def check_host_tools() -> bool:
    """빌드와 적재에 필요한 명령이 PATH 에 있는지 본다."""
    ok = True

    def probe(name, args, required, note=''):
        nonlocal ok
        path = shutil.which(name)
        if not path:
            mark = 'ㅡ 없음' if not required else 'X 없음 (필수)'
            print(f'  {name:20s} {mark}   {note}')
            if required:
                ok = False
            return
        try:
            out = subprocess.run([path] + args, capture_output=True, text=True,
                                 timeout=20).stdout.splitlines()
            ver = out[0].strip() if out else ''
        except Exception:
            ver = ''
        print(f'  {name:20s} O  {ver[:58]}')

    probe('arm-none-eabi-gcc', ['--version'], True,
          'Arm GNU Toolchain 13.2 이상 (Cortex-M85)')
    probe('cmake', ['--version'], True)
    probe('ninja', ['--version'], True)
    probe('pyocd', ['--version'], False, '적재/디버그에 필요')
    probe('git', ['--version'], False, '--with-fsp 에 필요')

    return ok


# --------------------------------------------------------------- DFP 팩

def download(url: str, dest: Path):
    log(f'받는 중 {url}')
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_suffix(dest.suffix + '.part')

    def hook(count, block, total):
        if total > 0:
            done = min(count * block, total)
            pct = done * 100 // total
            print(f'\r    {pct:3d}%  {done // 1048576} / {total // 1048576} MB',
                  end='', flush=True)

    urllib.request.urlretrieve(url, tmp, hook)
    print()
    tmp.replace(dest)


def patch_pack(src_pack: Path, dst_pack: Path):
    """원본 DFP 팩의 결함 두 개를 고쳐 새 팩을 만든다."""
    with tempfile.TemporaryDirectory() as td:
        work = Path(td) / 'pack'
        with zipfile.ZipFile(src_pack) as z:
            z.extractall(work)

        # 결함 1 - 존재하지 않는 FLM 참조
        added = 0
        for missing, source in MISSING_FLMS.items():
            tgt, src = work / missing, work / source
            if tgt.exists():
                continue
            if not src.exists():
                raise SystemExit(f'원본 FLM 이 없다: {source}')
            shutil.copy2(src, tgt)
            added += 1
        log(f'결함 1  FLM {added}개 보충')

        # 결함 2 - 듀얼코어 subFamily 에 코어별 AP 매핑 없음
        pdsc = work / 'Renesas.RA_DFP.pdsc'
        text = pdsc.read_text(errors='replace')
        patched = 0
        for sub in DUALCORE_SUBFAMILIES:
            key = f'<subFamily DsubFamily="{sub}">'
            i = text.find(key)
            if i < 0:
                log(f'        {sub} 없음 - 건너뜀')
                continue
            # 이미 들어가 있으면 다시 넣지 않는다
            tail = text[i:i + 2000]
            if 'Pname="CPU1"' in tail and '__ap=' in tail:
                continue
            j = i + len(key)
            text = text[:j] + AP_MAP_XML + text[j:]
            patched += 1
        pdsc.write_text(text)
        log(f'결함 2  듀얼코어 subFamily {patched}개에 AP 매핑 추가')

        # 다시 압축
        dst_pack.parent.mkdir(parents=True, exist_ok=True)
        tmp = dst_pack.with_suffix('.part')
        with zipfile.ZipFile(tmp, 'w', zipfile.ZIP_DEFLATED) as z:
            for f in sorted(work.rglob('*')):
                if f.is_file():
                    z.write(f, f.relative_to(work))
        tmp.replace(dst_pack)


def extract_svd(pack: Path, dest: Path):
    with zipfile.ZipFile(pack) as z:
        name = f'SVD/{SVD_NAME}'
        if name not in z.namelist():
            raise SystemExit(f'팩 안에 {name} 이 없다')
        dest.parent.mkdir(parents=True, exist_ok=True)
        with z.open(name) as src, open(dest, 'wb') as out:
            shutil.copyfileobj(src, out)


def setup_pack(root: Path, force: bool):
    fixed = root / 'packs' / DFP_FIXED
    svd = root / 'svd' / SVD_NAME

    if fixed.exists() and svd.exists() and not force:
        log(f'이미 있음 {fixed.relative_to(root)}  (--force 로 다시 만든다)')
        return

    cache = root / 'dist' / f'MDK_Device_Packs_v{FSP_VERSION}.zip'
    if not cache.exists():
        download(MDK_PACKS_URL, cache)
    else:
        log(f'받아 둔 것 사용 {cache.relative_to(root)}')

    orig = root / 'packs' / DFP_NAME
    if not orig.exists() or force:
        with zipfile.ZipFile(cache) as z:
            names = [n for n in z.namelist() if n.endswith(DFP_NAME)]
            if not names:
                raise SystemExit(f'{cache} 안에 {DFP_NAME} 이 없다')
            orig.parent.mkdir(parents=True, exist_ok=True)
            with z.open(names[0]) as src, open(orig, 'wb') as out:
                shutil.copyfileobj(src, out)
        log(f'꺼냄 {orig.relative_to(root)}')

    patch_pack(orig, fixed)
    log(f'만듦 {fixed.relative_to(root)}')

    extract_svd(fixed, svd)
    log(f'꺼냄 {svd.relative_to(root)}')


# --------------------------------------------------------------- FSP 소스

def setup_fsp(root: Path):
    dest = root / 'fsp'
    if dest.exists():
        log(f'이미 있음 {dest.relative_to(root)}')
        return
    if not shutil.which('git'):
        raise SystemExit('git 이 없다')
    log(f'clone {FSP_REPO} {FSP_TAG} (약 180 MB)')
    subprocess.run(['git', 'clone', '--depth', '1', '-b', FSP_TAG, FSP_REPO, str(dest)],
                   check=True)


# --------------------------------------------------------------- main

def main() -> int:
    ap = argparse.ArgumentParser(description='titan-mini 외부 자산 준비')
    ap.add_argument('--dir', type=Path, default=default_dir(),
                    help='자산을 둘 폴더 (기본: $RENESAS_RA_TOOLS 또는 ~/hdd/tools/renesas-ra)')
    ap.add_argument('--with-fsp', action='store_true', help='FSP 소스도 clone (RASC 재생성용)')
    ap.add_argument('--force', action='store_true', help='이미 있어도 다시 만든다')
    ap.add_argument('--check', action='store_true', help='확인만 하고 받지 않는다')
    args = ap.parse_args()

    root = args.dir.expanduser().resolve()

    print('== 호스트 도구 ==')
    tools_ok = check_host_tools()

    print(f'\n== 자산 폴더 ==\n  {root}')

    fixed = root / 'packs' / DFP_FIXED
    svd = root / 'svd' / SVD_NAME

    if args.check:
        print('\n== 자산 ==')
        for p in (fixed, svd):
            print(f'  {"O " if p.exists() else "X "} {p}')
        print(f'  {"O " if (root / "fsp").exists() else "ㅡ "} {root / "fsp"}  (재생성용, 선택)')
        return 0 if tools_ok and fixed.exists() and svd.exists() else 1

    if not tools_ok:
        print('\n필수 도구가 없다. 설치한 뒤 다시 돌린다.')
        return 1

    print('\n== DFP 팩 ==')
    setup_pack(root, args.force)

    if args.with_fsp:
        print('\n== FSP 소스 ==')
        setup_fsp(root)

    print(f'''
== 완료 ==

셸 프로파일(~/.zshrc 등)에 아래를 넣고 터미널을 다시 연다.
GUI 로 띄운 VSCode 에는 터미널의 export 가 넘어가지 않으므로 프로파일에 두어야 한다.

  export RENESAS_RA_TOOLS="{root}"

그 다음 빌드한다.

  cd firmware/ra8p1-fw
  cmake -S . -B build -G Ninja
  cmake --build build -j8
  cmake --build build --target flash
''')
    return 0


if __name__ == '__main__':
    sys.exit(main())
