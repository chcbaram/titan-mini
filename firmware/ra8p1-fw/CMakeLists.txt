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


# 산출물 배치
#   build/cm85/   CPU0 의 elf / bin / map
#   build/cm33/   CPU1 의 elf / bin / map
#   build/        두 코어를 묶은 통합 펌웨어 이미지 (41번 단계, tools/mkimage.py)
#
# 소스 트리 구조와 무관하게 코어 이름으로 얕게 잡는다. 코어 디렉터리를 옮겨도
# .vscode/launch.json 과 flash 타겟이 깨지지 않는다.


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
  ${RA_SDK_DIR}          # partition.h
  )

# FSP 는 glob 하지 않고 쓰는 모듈만 명시한다.
#   - ra_cfg/fsp_cfg 에 해당 모듈의 설정 헤더가 있어야 컴파일된다
#   - 전체 glob 을 하면 설정 헤더가 없는 모듈 소스가 같이 잡혀 빌드가 깨진다
#
# 목록은 코어별이다. 설정 헤더가 코어마다 다르기 때문이다. 예를 들어 SCI UART 는
# CPU0 만 쓰므로 cm33 의 ra_cfg 에는 r_sci_b_uart_cfg.h 가 없다. 공통 목록에 넣으면
# CPU1 빌드가 그 헤더를 못 찾고 깨진다.
#
# 여기에는 두 코어가 반드시 쓰는 것만 둔다. 나머지는 코어별 CMakeLists 에서 더한다.
#
file(GLOB_RECURSE RA_SDK_SRC_FILES CONFIGURE_DEPENDS
  ${RA_SDK_DIR}/ra/fsp/src/bsp/*.c
  ${RA_SDK_DIR}/ra/fsp/src/r_ioport/*.c
  )

# FreeRTOS 커널. RASC 가 ra/aws 로 가져온 것을 쓴다.
#   포트 레이어는 FSP 의 rm_freertos_port 다. 직접 포팅하지 않는다.
#   heap 은 heap_4 (병합 가능한 free 리스트). configuration.xml 에서 고른다.
#
set(FREERTOS_DIR ${RA_SDK_DIR}/ra/aws/FreeRTOS/FreeRTOS/Source)

file(GLOB FREERTOS_SRC_FILES CONFIGURE_DEPENDS
  ${FREERTOS_DIR}/*.c
  ${FREERTOS_DIR}/portable/MemMang/heap_4.c
  ${RA_SDK_DIR}/ra/fsp/src/rm_freertos_port/*.c
  )

list(APPEND SHARED_INC_DIRS
  ${FREERTOS_DIR}/include
  ${RA_SDK_DIR}/ra/fsp/src/rm_freertos_port
  )


add_subdirectory(src/cpu/cm85)

if(BUILD_CM33)
  add_subdirectory(src/cpu/cm33)
endif()


# 두 코어가 같은 FSP 소스를 서로 다른 -mcpu 로 컴파일한다. 통합 compile_commands.json
# 에는 공유 파일이 항목 두 개로 들어가고 IntelliSense 는 먼저 찾은 것을 쓰므로,
# 코어별로 갈라 놓아야 워크스페이스가 자기 코어 기준으로 해석한다.
#
find_package(Python3 COMPONENTS Interpreter)

if(Python3_FOUND)
  add_custom_target(compdb ALL
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/split_compdb.py
            ${CMAKE_BINARY_DIR}
    COMMENT "코어별 compile_commands.json 생성"
    VERBATIM
    )
else()
  message(STATUS "Python3 없음 - 코어별 compile_commands.json 을 만들지 않는다")
endif()


include(flash)
