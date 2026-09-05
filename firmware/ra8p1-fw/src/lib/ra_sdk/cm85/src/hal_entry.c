/* FSP 진입점. bspInit() 이 호출한다. */
#include "hal_data.h"


void hal_entry(void)
{
#if (1 == BSP_MULTICORE_PROJECT) && !BSP_SECONDARY_CORE_BUILD
  /*
   * CPU1 (Cortex-M33) 을 깨운다.
   *
   * 레지스터 세 개를 쓴다.
   *   CPU1INITVTOR  벡터 테이블 주소. 하위 7비트가 버려지므로 128 바이트 정렬 필수
   *   CPU1WAITCR    디버거가 CPU1 을 미리 붙잡아 둔 경우를 풀어준다
   *   CPU1ACTCSR    키코드 0xA5 와 함께 기동 요청
   *
   * 주소는 BSP_PARTITION_FLASH_CPU1_S_START 에서 온다.
   * (src/lib/ra_sdk/partition.h, docs/23-cm33-boot.md)
   */
  R_BSP_SecondaryCoreStart();
#endif
}
