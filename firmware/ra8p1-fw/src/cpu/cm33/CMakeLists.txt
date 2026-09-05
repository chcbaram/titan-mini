#-------------------------------------------------------------------------------
# CPU1 : Cortex-M33 (secondary core)
#
# 아직 스켈레톤이다. docs/04-dualcore.md 의 파티션이 확정되면 채운다.
#   - ra_sdk/ (configuration.xml, ra_cfg, ra_gen, script) 를 이 코어용으로 생성
#   - memory_regions.ld 의 FLASH/RAM 을 CPU1 파티션으로 좁힌다
#   - bsp_linker_info.h 에 BSP_PARTITION_* 매크로를 넣어야
#     BSP_MULTICORE_PROJECT 가 1 이 되고 R_BSP_SecondaryCoreStart() 가 컴파일된다
#
# 켜는 법: cmake -S . -B build -G Ninja -DBUILD_CM33=ON
#-------------------------------------------------------------------------------
message(FATAL_ERROR "CM33 이미지는 아직 준비되지 않았다. docs/04-dualcore.md 참고.")
