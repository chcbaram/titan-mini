/*
 * titan-mini CM85 (CPU0) 링커 스크립트
 *
 * RASC 가 만든 script/fsp.ld 를 그대로 쓰지 않는 이유는, 거기에 우리 섹션을 넣으면
 * 재생성할 때마다 날아가기 때문이다. 생성물 두 개를 INCLUDE 하고 필요한 것만 덧붙인다.
 *
 * memory_regions.ld / fsp_gen.ld 는 src/lib/ra_sdk/cm85/ 에 있다.
 * CMake 가 그 경로를 -L 로 넘겨준다.
 */

INCLUDE memory_regions.ld
INCLUDE fsp_gen.ld

SECTIONS
{
  /*
   * 코어간 공유 블록.
   *
   * 두 코어가 같은 주소를 보게 고정한다. 주소는 partition.h 의
   * BSP_PARTITION_RAM_SHARED_START 와 같아야 한다.
   *
   * NOLOAD 인 이유는 이미지에 담지도 초기화하지도 않기 위해서다. 초기화하면
   * 나중에 부팅한 코어가 상대 코어가 써 둔 값을 밀어 버린다. 유효성은 magic 으로
   * 판단한다. (src/cpu/shared/shared.h)
   */
  .shared 0x221C0000 (NOLOAD) :
  {
    KEEP (*(.shared))
    . = ALIGN(4);
  }

  /*
   * 모듈 디스크립터 테이블.
   *
   * ap/modules 의 각 모듈이 MODULE_DEF() 로 자기 디스크립터를 여기에 심는다.
   * moduleInit() 이 _smodule ~ _emodule 을 훑어 우선순위 순으로 init() 을 부른다.
   * 파일을 추가하는 것만으로 등록되므로 목록을 따로 관리하지 않아도 된다.
   *
   * 디스크립터는 const 라 FLASH 에 그대로 둔다. RAM 으로 복사하면 FSP 의
   * SystemRuntimeInit() 복사 테이블에 들어가지 않아 부팅 시 쓰레기가 남는다.
   */
  .module : ALIGN(4)
  {
    _smodule = .;
    KEEP (*(.module))
    KEEP (*(.module*))
    _emodule = .;
    . = ALIGN(4);
  } > FLASH
}
