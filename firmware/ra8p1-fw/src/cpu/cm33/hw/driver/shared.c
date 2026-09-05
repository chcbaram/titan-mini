#include "shared.h"

/*
 * 공유 블록의 실체.
 *
 * 링커 스크립트가 .shared 섹션을 SRAM 의 고정 주소(BSP_PARTITION_RAM_SHARED_START)에
 * 놓는다. 두 코어가 같은 주소를 보게 하려는 것이다.
 *
 * CPU1 이 정의하고 CPU0 은 같은 주소에 심볼만 얹는다. 초기화하지 않는다 —
 * CPU0 이 먼저 부팅해서 이 영역을 읽는데, CPU1 이 나중에 0 으로 밀어 버리면
 * 순서에 따라 값이 사라진다. magic 으로 유효성을 판단한다.
 */
__attribute__((section(".shared"), used))
shared_t g_shared;
