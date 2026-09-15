if(CMAKE_BUILD_EARLY_EXPANSION)
    idf_component_register(REQUIRES mbedtls esp_timer bootloader_support fatfs esp_driver_gpio)
    return()
endif()

include(FetchContent)
FetchContent_Declare(ubitcoin
    GIT_REPOSITORY https://github.com/micro-bitcoin/uBitcoin.git
    GIT_TAG 877542fdc16319dd92a7d2a679ea9dacce474bd2)
FetchContent_MakeAvailable(ubitcoin)
get_filename_component(AURORA_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
execute_process(COMMAND "${PYTHON}" "${AURORA_ROOT}/tools/patch_ubitcoin.py"
    --lib-root "${ubitcoin_SOURCE_DIR}/src" RESULT_VARIABLE PATCH_RESULT)
if(NOT PATCH_RESULT EQUAL 0)
    message(FATAL_ERROR "AURORA uBitcoin hardening failed")
endif()
file(GLOB_RECURSE BITCOIN_SOURCES "${ubitcoin_SOURCE_DIR}/src/*.cpp" "${ubitcoin_SOURCE_DIR}/src/*.c")
file(GLOB AURORA_ASSETS "${AURORA_ROOT}/src/assets/*.c")
file(GLOB PORTRAIT_ASSETS "${CMAKE_CURRENT_LIST_DIR}/assets/*.c")
idf_component_register(SRCS "main.cpp" "sensors.cpp" "storage.cpp" "ui_portrait.cpp"
    "${AURORA_ROOT}/src/wallet.cpp" "${AURORA_ROOT}/src/sd_export.cpp"
    "${AURORA_ROOT}/src/pin_security.cpp" "${AURORA_ROOT}/src/aezeed.cpp"
    "${AURORA_ROOT}/src/secure_lvgl_memory.c"
    "${AURORA_ROOT}/src/ui.cpp" ${AURORA_ASSETS} ${PORTRAIT_ASSETS} ${BITCOIN_SOURCES}
    INCLUDE_DIRS "." "${AURORA_ROOT}/include" "${ubitcoin_SOURCE_DIR}/src"
    REQUIRES mbedtls esp_timer bootloader_support fatfs esp_driver_gpio)
target_compile_definitions(${COMPONENT_LIB} PUBLIC AURORA_BOARD_P4=1 USE_STDONLY=1)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable)
