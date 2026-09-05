#ifndef SHARED_H_
#define SHARED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


/*
 * 코어간 공유 블록의 레이아웃.
 *
 * 이 파일이 common/hw/include 가 아니라 common/ 바로 아래 있는 이유가 있다.
 * common/hw/include 는 MCU 가 바뀌어도 그대로 쓰는 드라이버 API 자리다.
 * 이 헤더는 API 가 아니라 **이 프로젝트 두 코어 사이의 데이터 규약**이고,
 * 어떤 필드를 둘지는 프로젝트마다 다르다. def.h / evt_code.h 와 같은 성격이라
 * 같은 자리에 둔다.
 *
 * 공개 인터페이스는 hw/include/ipc.h 의 함수들이다. 여기에는 extern 을 두지 않는다 —
 * shared 는 각 코어의 hw/driver/ipc.c 안에 숨어 있고, 바깥은 함수로만 접근한다.
 *
 * 실제 주소는 partition.h 의 BSP_PARTITION_RAM_SHARED_START 와 두 코어의 링커
 * 스크립트가 정한다. SRAM 끝의 non-cacheable 구간에 둔다 - Cortex-M85 에 D-cache 가
 * 있어서 캐시 가능 영역을 공유하면 유지보수를 한 번만 빼먹어도 낡은 데이터를 본다.
 *
 * 두 코어가 같은 헤더를 보므로 구조체를 고치면 양쪽을 다시 빌드해야 한다.
 * version 으로 그걸 잡는다 - 상대 코어가 자기 버전을 쓰고 주 코어가 확인한다.
 */
#define SHARED_MAGIC      0x544D5348UL      /* "TMSH" */
#define SHARED_VERSION    1


typedef struct
{
  volatile uint32_t magic;
  volatile uint32_t version;

  //-- 상대 코어가 살아 있음을 알린다. 주 코어가 증가를 확인한다.
  volatile uint32_t peer_alive;
  volatile uint32_t peer_tick;

} shared_t;


#ifdef __cplusplus
}
#endif

#endif
