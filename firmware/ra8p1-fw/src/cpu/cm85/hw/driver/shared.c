#include "shared.h"

/*
 * 공유 블록 참조.
 *
 * 실체는 CPU1 이 정의하고 여기서는 같은 주소에 심볼만 얹는다. 링커 스크립트가
 * .shared 섹션을 두 코어 모두 0x221C0000 에 놓기 때문에 주소가 일치한다.
 *
 * NOLOAD 라 이미지에 담기지 않고 초기화도 되지 않는다. 그래야 나중에 부팅한
 * 코어가 상대가 써 둔 값을 밀어 버리지 않는다.
 */
__attribute__((section(".shared"), used))
shared_t g_shared;
