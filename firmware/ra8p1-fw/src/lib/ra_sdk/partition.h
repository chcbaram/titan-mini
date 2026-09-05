/*
 * 듀얼코어 메모리 파티션.
 *
 * RASC 는 Solution 프로젝트에서만 이 매크로들을 bsp_linker_info.h 의
 * "Solution Definitions" 블록에 넣어 준다. 그런데 standalone RASC 로는 Solution 을
 * 헤드리스로 생성할 수 없어서(docs/11-fsp-config.md 6장) 여기서 직접 정의한다.
 *
 * 이름 규칙은 RASC 의 freemarker 템플릿 원문에서 읽어낸 것이라 정식 형식과 같다.
 *   BSP_PARTITION_<RESOURCE>_<CPU0|CPU1>_<S|NS>_START / _SIZE
 *
 * FSP 는 BSP_PARTITION_FLASH_CPU1_S_START 가 정의돼 있는지로 멀티코어 프로젝트를
 * 판정한다(bsp_common.h). 이 파일이 없으면 BSP_MULTICORE_PROJECT 가 0 이 되고
 * R_BSP_SecondaryCoreStart() 자체가 컴파일되지 않는다.
 *
 * 두 코어의 bsp_linker_info.h 가 이 파일을 include 하고, 링커 스크립트의
 * memory_regions.ld 도 같은 값을 쓴다. 한곳에서 관리하려는 것이다.
 */
#ifndef PARTITION_H_
#define PARTITION_H_


/*------------------------------------------------------------------ MRAM 1 MB
 *
 *   0x0200_0000  CPU0   768 KB    <- CPU0 초기 벡터는 하드웨어 고정이라 맨 앞
 *   0x020C_0000  CPU1   256 KB    <- CPU1INITVTOR. 128 바이트 정렬 필수
 *
 * 40번 단계에서 부트로더가 들어오면 CPU0 영역 앞을 128 KB 떼어 준다.
 * CPU1 시작 주소는 그대로 두면 되므로 지금 이 값을 잡아도 안전하다.
 */
#define BSP_PARTITION_FLASH_CPU0_S_START      (0x02000000)
#define BSP_PARTITION_FLASH_CPU0_S_SIZE       (0x000C0000)

#define BSP_PARTITION_FLASH_CPU1_S_START      (0x020C0000)
#define BSP_PARTITION_FLASH_CPU1_S_SIZE       (0x00040000)


/*------------------------------------------------------------------ SRAM 1872 KB
 *
 *   0x2200_0000  CPU0    1408 KB
 *   0x2216_0000  CPU1     384 KB
 *   0x221C_0000  SHARED    80 KB   <- 코어간 공유. non-cacheable 로 잡는다
 *
 * 합계 0x1D_4000 = 1872 KB. (docs/02-memory-map.md)
 */
#define BSP_PARTITION_RAM_CPU0_S_START        (0x22000000)
#define BSP_PARTITION_RAM_CPU0_S_SIZE         (0x00160000)

#define BSP_PARTITION_RAM_CPU1_S_START        (0x22160000)
#define BSP_PARTITION_RAM_CPU1_S_SIZE         (0x00060000)

/*
 * 공유 영역.
 *
 * Cortex-M85 에 D-cache 가 있어서 캐시 가능 영역을 공유하면 유지보수를 한 번만
 * 빼먹어도 상대 코어가 낡은 데이터를 본다. 증상이 산발적이라 디버깅이 아주 어렵다.
 * 그래서 MPU 로 non-cacheable 로 잡는다. (docs/04-dualcore.md)
 */
#define BSP_PARTITION_RAM_SHARED_START        (0x221C0000)
#define BSP_PARTITION_RAM_SHARED_SIZE         (0x00014000)


#endif /* PARTITION_H_ */
