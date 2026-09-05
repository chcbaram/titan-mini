#ifndef MODULE_H_
#define MODULE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ap_def.h"


typedef enum
{
  MODULE_PRI_HIGH = 1,
  MODULE_PRI_1,
  MODULE_PRI_2,
  MODULE_PRI_3,
  MODULE_PRI_4,
  MODULE_PRI_NORMAL,
  MODULE_PRI_LOWEST,
  MODULE_PRI_MAX,
} ModulePriority_t;


typedef struct
{
  const char       name[32];
  ModulePriority_t priority;
  bool           (*init)(void);
} module_t;


/*
 * 모듈 자기 등록.
 *
 * 디스크립터를 .module 섹션에 심으면 moduleInit() 이 링커가 만든 _smodule ~ _emodule
 * 범위를 훑어 우선순위 순으로 init() 을 부른다. 파일을 추가하는 것만으로 등록되므로
 * 목록을 따로 관리할 필요가 없다.
 *
 * const 인 이유는 FLASH 에 그대로 두기 위해서다. RAM 으로 복사되는 섹션에 넣으면
 * FSP 의 SystemRuntimeInit() 복사 테이블에 안 들어가서 부팅 시 쓰레기가 남는다.
 * (bsp/ldscript/titan-mini-cm85.ld 참고)
 */
#define MODULE_DEF(x_name) \
  static const __attribute__((section(".module"), used)) module_t module_##x_name =


bool moduleInit(void);


#ifdef __cplusplus
}
#endif

#endif
