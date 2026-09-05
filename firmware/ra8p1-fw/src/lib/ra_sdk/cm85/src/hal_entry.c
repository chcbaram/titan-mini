/* FSP 진입점. bspInit() 이 호출한다. */
#include "hal_data.h"


void hal_entry(void)
{
  /*
   * CPU1 기동은 여기서 하지 않는다.
   *
   * 이 함수는 부팅 아주 초기(bspInit)에 불리므로 UART 도 로그도 아직 없다.
   * 기동을 시도했는지, 성공했는지를 남길 방법이 없어서
   * hw/driver/ipc.c 의 ipcInit() 으로 옮겼다. hwInit() 이 로그를 연 뒤에 부른다.
   */
}
