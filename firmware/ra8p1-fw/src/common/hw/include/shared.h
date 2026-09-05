#ifndef SHARED_H_
#define SHARED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


/*
 * 코어간 공유 블록.
 *
 * SRAM 끝의 non-cacheable 구간에 둔다(partition.h 의 BSP_PARTITION_RAM_SHARED_*).
 * Cortex-M85 에 D-cache 가 있어서 캐시 가능 영역을 공유하면 유지보수를 한 번만
 * 빼먹어도 상대 코어가 낡은 데이터를 본다.
 *
 * 두 코어가 같은 헤더를 보므로 구조체를 고치면 양쪽을 다시 빌드해야 한다.
 * magic 으로 그걸 잡는다 — CPU1 이 자기 버전을 쓰고 CPU0 이 확인한다.
 */
#define SHARED_MAGIC      0x544D5348UL      /* "TMSH" */
#define SHARED_VERSION    1


typedef struct
{
  volatile uint32_t magic;
  volatile uint32_t version;

  //-- CPU1 이 살아 있음을 알린다. CPU0 이 증가를 확인한다.
  volatile uint32_t cpu1_alive;
  volatile uint32_t cpu1_tick;

} shared_t;


extern shared_t g_shared;


#ifdef __cplusplus
}
#endif

#endif
