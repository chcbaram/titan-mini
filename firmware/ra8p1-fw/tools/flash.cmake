#-------------------------------------------------------------------------------
# pyOCD 적재를 CMake 타겟으로 만든다. 셸 스크립트를 두지 않는 이유는
# Windows / Linux / macOS 에서 같은 명령으로 돌리기 위해서다.
#
#   cmake --build build --target flash
#
# DFP 팩(94MB)과 SVD(21MB)는 저장소에 넣지 않는다. 아래 순서로 찾는다.
#   1. -DRA_DFP_PACK=<경로> 로 직접 지정
#   2. tools/ 아래의 Renesas.RA_DFP.*.pack
#   3. 환경변수 RENESAS_RA_TOOLS 아래의 packs/
#
# 주의: 릴리스 원본 팩은 존재하지 않는 FLM 3개를 참조해서 pyOCD 가 파싱 단계에서 죽는다.
#       수정본(-fixed)을 써야 한다. docs/10-dev-environment.md 참고.
#-------------------------------------------------------------------------------
find_program(PYOCD_EXECUTABLE NAMES pyocd pyocd.exe)

set(RA_DFP_PACK   "" CACHE FILEPATH "Renesas RA DFP CMSIS pack (수정본)")
set(PYOCD_TARGET  "r7ka8p1kf" CACHE STRING "pyOCD target name (DFP 가 Dname 을 9자로 자른다)")

if(NOT RA_DFP_PACK)
  set(_pack_globs "${CMAKE_CURRENT_SOURCE_DIR}/tools/Renesas.RA_DFP.*.pack")
  if(DEFINED ENV{RENESAS_RA_TOOLS})
    list(APPEND _pack_globs "$ENV{RENESAS_RA_TOOLS}/packs/Renesas.RA_DFP.*-fixed.pack")
  endif()

  file(GLOB _packs ${_pack_globs})
  if(_packs)
    list(SORT _packs)
    list(REVERSE _packs)
    list(GET _packs 0 RA_DFP_PACK)
  endif()
endif()

if(PYOCD_EXECUTABLE AND RA_DFP_PACK)
  message(STATUS "pyOCD pack: ${RA_DFP_PACK}")
  add_custom_target(flash
    COMMAND ${PYOCD_EXECUTABLE} flash
            --target ${PYOCD_TARGET}
            --pack  ${RA_DFP_PACK}
            $<TARGET_FILE:${PRJ_NAME}-cm85.elf>
    DEPENDS ${PRJ_NAME}-cm85.elf
    USES_TERMINAL
    COMMENT "pyocd flash (${PYOCD_TARGET})"
    )
else()
  add_custom_target(flash
    COMMAND ${CMAKE_COMMAND} -E echo
            "flash 타겟에는 pyocd 와 RA DFP 팩이 필요하다. -DRA_DFP_PACK=<경로> 또는 환경변수 RENESAS_RA_TOOLS 를 설정한다."
    COMMAND ${CMAKE_COMMAND} -E false
    )
endif()
