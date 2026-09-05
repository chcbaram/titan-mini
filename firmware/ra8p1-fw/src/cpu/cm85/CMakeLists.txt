#-------------------------------------------------------------------------------
# CPU0 : Cortex-M85 (primary core)
#-------------------------------------------------------------------------------
set(CORE_NAME  cm85)
set(EXECUTABLE ${PRJ_NAME}-${CORE_NAME}.elf)

set(CORE_DIR   ${CMAKE_CURRENT_SOURCE_DIR})
set(CORE_SDK   ${RA_SDK_DIR}/${CORE_NAME})   # FSP 설정·생성물은 벤더 트리 아래에 둔다
set(CORE_OUT   ${CMAKE_BINARY_DIR}/${CORE_NAME})


# 이 코어의 소스
#
file(GLOB SRC_FILES CONFIGURE_DEPENDS
  ${CORE_DIR}/*.c
  ${CORE_DIR}/*.cpp
  )

file(GLOB_RECURSE SRC_FILES_RECURSE CONFIGURE_DEPENDS
  ${CORE_DIR}/ap/*.c
  ${CORE_DIR}/ap/*.cpp
  ${CORE_DIR}/bsp/*.c
  ${CORE_DIR}/bsp/*.s
  ${CORE_DIR}/hw/*.c
  )

# 이 코어의 FSP 생성물 (ra_gen) 과 사용자 훅 (ra_sdk/src)
#   ra_gen/main.c 는 RASC 가 만든 것이라 제외한다. main 은 src/cpu/cm85/main.c 다.
#
file(GLOB SRC_FILES_RA_GEN CONFIGURE_DEPENDS
  ${CORE_SDK}/ra_gen/*.c
  ${CORE_SDK}/src/*.c
  )
list(REMOVE_ITEM SRC_FILES_RA_GEN ${CORE_SDK}/ra_gen/main.c)


add_executable(${EXECUTABLE}
  ${SRC_FILES}
  ${SRC_FILES_RECURSE}
  ${SRC_FILES_RA_GEN}
  ${SHARED_SRC_FILES}
  ${RA_SDK_SRC_FILES}
  ${FREERTOS_SRC_FILES}
  )


# elf / bin / map 을 build/<코어>/ 로 모은다
#
set_target_properties(${EXECUTABLE} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CORE_OUT}
  )


target_include_directories(${EXECUTABLE} PRIVATE
  ${CORE_DIR}
  ${CORE_DIR}/ap
  ${CORE_DIR}/ap/modules
  ${CORE_DIR}/ap/modules/common/display
  ${CORE_DIR}/bsp
  ${CORE_DIR}/bsp/rtos
  ${CORE_DIR}/hw
  ${CORE_DIR}/hw/driver

  # bsp_linker_info.h 는 RASC 가 프로젝트 루트에 생성한다. 이 경로가 없으면 링크 전에
  # bsp_linker.c 컴파일이 깨진다.
  ${CORE_SDK}
  ${CORE_SDK}/ra_cfg/fsp_cfg
  ${CORE_SDK}/ra_cfg/fsp_cfg/bsp
  ${CORE_SDK}/ra_gen
  ${CORE_SDK}/src

  ${SHARED_INC_DIRS}
  )

target_compile_definitions(${EXECUTABLE} PRIVATE
  -D_RA_CORE=CPU0
  -D_RA_ORDINAL=1
  -D_RENESAS_RA_
  )

target_compile_options(${EXECUTABLE} PRIVATE
  -mcpu=cortex-m85
  -mthumb
  -mfloat-abi=hard
  -mfpu=fpv5-d16

  -fdata-sections
  -ffunction-sections

  -fstack-usage
  -Wmissing-field-initializers
  -Wuninitialized

  -Wall
  -g3
  -Og
  )

# FSP 소스는 프로젝트 코드와 경고 수준을 따로 간다.
#
set_source_files_properties(
  ${RA_SDK_SRC_FILES}
  ${FREERTOS_SRC_FILES}
  ${SRC_FILES_RA_GEN}
  TARGET_DIRECTORY ${EXECUTABLE}
  PROPERTIES
    COMPILE_OPTIONS "-Os;-w"
  )

# 링커 스크립트는 우리가 소유한다(bsp/ldscript/). RASC 의 script/fsp.ld 를 직접 쓰면
# .module 같은 우리 섹션이 재생성 때마다 날아간다.
# 그 스크립트가 memory_regions.ld / fsp_gen.ld 를 INCLUDE 하므로 두 파일이 있는
# ra_sdk 루트를 -L 로 알려줘야 GNU ld 가 찾는다.
#
target_link_directories(${EXECUTABLE} PRIVATE
  ${CORE_SDK}
  ${CORE_SDK}/script
  )

target_link_options(${EXECUTABLE} PRIVATE
  -T${CORE_DIR}/bsp/ldscript/${PRJ_NAME}-${CORE_NAME}.ld

  -mcpu=cortex-m85
  -mthumb
  -mfloat-abi=hard
  -mfpu=fpv5-d16
  -specs=nano.specs
  -specs=nosys.specs

  -lc
  -lm
  -fno-rtti

  -Wl,-Map=${CORE_OUT}/${PRJ_NAME}-${CORE_NAME}.map,--cref
  -Wl,--gc-sections
  -Wl,--no-warn-rwx-segments
  -Xlinker -print-memory-usage -Xlinker
  )


# .elf -> .bin
#
add_custom_command(TARGET ${EXECUTABLE}
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} ARGS -O binary
          $<TARGET_FILE:${EXECUTABLE}>
          ${CORE_OUT}/${PRJ_NAME}-${CORE_NAME}.bin
  COMMENT "Invoking: Make Bin"
  )
