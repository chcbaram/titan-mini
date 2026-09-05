#-------------------------------------------------------------------------------
# CPU1 : Cortex-M33 (secondary core)
#
# 켜는 법: cmake -S . -B build -G Ninja -DBUILD_CM33=ON
#-------------------------------------------------------------------------------
set(CORE_NAME  cm33)
set(EXECUTABLE ${PRJ_NAME}-${CORE_NAME}.elf)

set(CORE_DIR   ${CMAKE_CURRENT_SOURCE_DIR})
set(CORE_SDK   ${RA_SDK_DIR}/${CORE_NAME})
set(CORE_OUT   ${CMAKE_BINARY_DIR}/${CORE_NAME})


file(GLOB SRC_FILES CONFIGURE_DEPENDS
  ${CORE_DIR}/*.c
  )

file(GLOB_RECURSE SRC_FILES_RECURSE CONFIGURE_DEPENDS
  ${CORE_DIR}/ap/*.c
  ${CORE_DIR}/bsp/*.c
  ${CORE_DIR}/hw/*.c
  )

file(GLOB SRC_FILES_RA_GEN CONFIGURE_DEPENDS
  ${CORE_SDK}/ra_gen/*.c
  ${CORE_SDK}/src/*.c
  )
list(REMOVE_ITEM SRC_FILES_RA_GEN ${CORE_SDK}/ra_gen/main.c)


# CPU1 은 아직 FreeRTOS 도 공용 드라이버도 쓰지 않는다. 기동 확인이 목적이라
# FSP 와 자기 소스만 넣는다. 필요해지면 SHARED_SRC_FILES 를 더한다.
#
add_executable(${EXECUTABLE}
  ${SRC_FILES}
  ${SRC_FILES_RECURSE}
  ${SRC_FILES_RA_GEN}
  ${RA_SDK_SRC_FILES}
  )

set_target_properties(${EXECUTABLE} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CORE_OUT}
  )

target_include_directories(${EXECUTABLE} PRIVATE
  ${CORE_DIR}
  ${CORE_DIR}/ap
  ${CORE_DIR}/bsp
  ${CORE_DIR}/hw
  ${CORE_DIR}/hw/driver

  ${CORE_SDK}
  ${CORE_SDK}/ra_cfg/fsp_cfg
  ${CORE_SDK}/ra_cfg/fsp_cfg/bsp
  ${CORE_SDK}/ra_gen
  ${CORE_SDK}/src

  ${SHARED_INC_DIRS}
  )

target_compile_definitions(${EXECUTABLE} PRIVATE
  -D_RA_CORE=CPU1
  -D_RA_ORDINAL=2
  -D_RENESAS_RA_
  )

target_compile_options(${EXECUTABLE} PRIVATE
  -mcpu=cortex-m33
  -mthumb
  -mfloat-abi=hard
  -mfpu=fpv5-sp-d16

  -fdata-sections
  -ffunction-sections

  -fstack-usage
  -Wmissing-field-initializers
  -Wuninitialized

  -Wall
  -g3
  -Og
  )

set_source_files_properties(
  ${RA_SDK_SRC_FILES}
  ${SRC_FILES_RA_GEN}
  TARGET_DIRECTORY ${EXECUTABLE}
  PROPERTIES
    COMPILE_OPTIONS "-Os;-w"
  )

target_link_directories(${EXECUTABLE} PRIVATE
  ${CORE_SDK}
  ${CORE_SDK}/script
  )

target_link_options(${EXECUTABLE} PRIVATE
  -T${CORE_DIR}/bsp/ldscript/${PRJ_NAME}-${CORE_NAME}.ld

  -mcpu=cortex-m33
  -mthumb
  -mfloat-abi=hard
  -mfpu=fpv5-sp-d16
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

add_custom_command(TARGET ${EXECUTABLE}
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} ARGS -O binary
          $<TARGET_FILE:${EXECUTABLE}>
          ${CORE_OUT}/${PRJ_NAME}-${CORE_NAME}.bin
  COMMENT "Invoking: Make Bin"
  )
