/*
 * titan-mini CM33 (CPU1) 링커 스크립트
 *
 * CM85 와 같은 이유로 우리가 소유한다. memory_regions.ld / fsp_gen.ld 는
 * src/lib/ra_sdk/cm33/ 에 있고 CMake 가 그 경로를 -L 로 넘겨준다.
 *
 * CM33 에는 아직 모듈 구조가 없어서 .module 섹션이 없다.
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
   * 판단한다. (src/common/hw/include/shared.h)
   */
  .shared 0x221C0000 (NOLOAD) :
  {
    KEEP (*(.shared))
    . = ALIGN(4);
  }
}
