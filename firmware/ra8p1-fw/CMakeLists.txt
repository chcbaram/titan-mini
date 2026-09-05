cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tools")
include(arm-none-eabi-gcc)


# 프로젝트 이름은 최종 출력 파일 이름의 접두어로 사용된다.
#
set(PRJ_NAME titan-mini)


project(${PRJ_NAME}
  LANGUAGES ASM C CXX
)


# CPU1(Cortex-M33) 이미지는 아직 스켈레톤이다. 준비되면 -DBUILD_CM33=ON 으로 켠다.
#
option(BUILD_CM33 "build CPU1 (Cortex-M33) image" OFF)


# 코어별 CMakeLists 가 하위 디렉터리에 있어도 elf/bin/map 은 build/ 바로 아래에 모은다.
# .vscode/launch.json 과 flash 타겟이 얕고 고정된 경로를 쓰게 하려는 것이다.
#
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


#-------------------------------------------------------------------------------
# 코어가 공유하는 것
#   - src/common      : 포터블 코어와 드라이버 공개 헤더
#   - src/lib/ra_sdk  : FSP 소스 원본 (코어별로 다른 -mcpu 로 다시 컴파일된다)
#-------------------------------------------------------------------------------
set(SHARED_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(RA_SDK_DIR    ${SHARED_DIR}/lib/ra_sdk)

file(GLOB_RECURSE SHARED_SRC_FILES CONFIGURE_DEPENDS
  ${SHARED_DIR}/common/*.c
  )

set(SHARED_INC_DIRS
  ${SHARED_DIR}
  ${SHARED_DIR}/common
  ${SHARED_DIR}/common/core
  ${SHARED_DIR}/common/hw/include
  ${SHARED_DIR}/lib

  ${RA_SDK_DIR}/ra/arm/CMSIS_6/CMSIS/Core/Include
  ${RA_SDK_DIR}/ra/fsp/inc
  ${RA_SDK_DIR}/ra/fsp/inc/api
  ${RA_SDK_DIR}/ra/fsp/inc/instances
  )

# FSP 는 glob 하지 않고 쓰는 모듈만 명시한다.
#   - ra_cfg/fsp_cfg 에 해당 모듈의 설정 헤더가 있어야 컴파일된다
#   - 기능을 추가할 때 여기에 한 줄씩 늘린다
#   (전체 glob 을 하면 설정 헤더가 없는 모듈 소스가 같이 잡혀 빌드가 깨진다)
#
file(GLOB_RECURSE RA_SDK_SRC_FILES CONFIGURE_DEPENDS
  ${RA_SDK_DIR}/ra/fsp/src/bsp/*.c
  ${RA_SDK_DIR}/ra/fsp/src/r_ioport/*.c
  )


add_subdirectory(src/cpu/cm85)

if(BUILD_CM33)
  add_subdirectory(src/cpu/cm33)
endif()


include(flash)
