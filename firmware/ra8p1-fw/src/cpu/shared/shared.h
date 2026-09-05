#ifndef SHARED_H_
#define SHARED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


/*
 * 코어간 공유 블록의 레이아웃.
 *
 * src/cpu/shared/ 는 **두 코어가 합의해야 하는 것**의 자리다. 어느 한 코어가
 * 소유하지 않는다. 앞으로 IPC 메시지 정의처럼 양쪽이 같아야 하는 것이 여기 모인다.
 *
 * common/ 에 두지 않은 이유가 있다. common/ 은 MCU 가 바뀌어도 그대로 가는 포터블
 * 코드 자리인데, 이 헤더는 이 보드 두 코어 사이의 규약이라 프로젝트마다 다르다.
 *
 * 코어별 bsp/ 에 각각 두지 않은 이유도 있다. 두 벌이 되면 **바이트 단위로 같아야**
 * 하는데 그걸 강제할 방법이 없다. version 이 잡아주는 것은 버전을 올렸을 때뿐이고,
 * 필드 순서만 바꾸고 버전을 그대로 두면 한쪽은 peer_alive 를 다른 쪽은 peer_tick 을
 * 같은 오프셋으로 읽는다. 컴파일도 링크도 통과한다.
 *
 * 공개 인터페이스는 hw/include/ipc.h 의 함수들이다. 여기에는 extern 을 두지 않는다 -
 * shared 는 각 코어의 hw/driver/ipc.c 안에 static 으로 숨어 있고 바깥은 함수로만 연다.
 *
 * 실제 주소는 partition.h 의 BSP_PARTITION_RAM_SHARED_START 와 두 코어의 링커
 * 스크립트가 정한다. SRAM 끝의 non-cacheable 구간에 둔다 - Cortex-M85 에 D-cache 가
 * 있어서 캐시 가능 영역을 공유하면 유지보수를 한 번만 빼먹어도 낡은 데이터를 본다.
 *
 * 구조체를 고치면 두 코어를 함께 다시 빌드해야 한다. version 으로 그걸 잡는다 -
 * 상대 코어가 자기 버전을 쓰고 주 코어가 확인한다.
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
