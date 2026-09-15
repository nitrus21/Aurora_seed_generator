# Include from main after idf_component_register: this target is P4 only.
if(NOT IDF_TARGET STREQUAL "esp32p4" OR NOT TARGET mbedcrypto)
    message(FATAL_ERROR "AURORA crypto overlay requires the pinned P4 Mbed TLS target")
endif()
set(AURORA_CRYPTO_OVERLAY "${CMAKE_BINARY_DIR}/aurora-crypto-hardened")
execute_process(COMMAND "${PYTHON}" "${AURORA_ROOT}/tools/prepare_p4_crypto_overlay.py"
    --components "${IDF_PATH}/components" --output "${AURORA_CRYPTO_OVERLAY}"
    RESULT_VARIABLE AURORA_CRYPTO_OVERLAY_RESULT)
if(NOT AURORA_CRYPTO_OVERLAY_RESULT EQUAL 0)
    message(FATAL_ERROR "AURORA P4 crypto overlay validation failed")
endif()

function(aurora_crypto_replace_source target suffix replacement original_directory)
    get_target_property(crypto_sources ${target} SOURCES)
    set(updated_sources)
    set(matches 0)
    foreach(source IN LISTS crypto_sources)
        string(REPLACE "\\" "/" normalized_source "${source}")
        if(normalized_source MATCHES "(^|/)${suffix}$")
            math(EXPR matches "${matches} + 1")
            list(APPEND updated_sources "${AURORA_CRYPTO_OVERLAY}/${replacement}")
        else()
            list(APPEND updated_sources "${source}")
        endif()
    endforeach()
    if(NOT matches EQUAL 1)
        message(FATAL_ERROR "AURORA P4 expected exactly one crypto source ${suffix}, got ${matches}")
    endif()
    set_property(TARGET ${target} PROPERTY SOURCES "${updated_sources}")
    # Relative quoted includes in the SDK source keep their original context.
    target_include_directories(${target} PRIVATE "${IDF_PATH}/components/${original_directory}")
endfunction()

aurora_crypto_replace_source(mbedcrypto "md\\.c" "md.c" "mbedtls/mbedtls/library")
aurora_crypto_replace_source(mbedcrypto "platform_util\\.c" "platform_util.c" "mbedtls/mbedtls/library")
aurora_crypto_replace_source(mbedcrypto "sha/core/sha\\.c" "sha_core.c" "mbedtls/port/sha/core")
aurora_crypto_replace_source(mbedcrypto "aes/dma/esp_aes\\.c" "esp_aes_dma.c" "mbedtls/port/aes/dma")
idf_component_get_property(AURORA_ESP_SECURITY_LIB esp_security COMPONENT_LIB)
if(NOT TARGET ${AURORA_ESP_SECURITY_LIB})
    message(FATAL_ERROR "AURORA P4 HMAC overlay requires the esp_security target")
endif()
aurora_crypto_replace_source(${AURORA_ESP_SECURITY_LIB} "esp_hmac\\.c" "esp_hmac.c" "esp_security/src")
target_compile_definitions(mbedcrypto PRIVATE AURORA_BOARD_P4=1)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${AURORA_ROOT}/tools/prepare_p4_crypto_overlay.py"
    "${AURORA_ROOT}/tools/patch_ubitcoin_p4.py"
    "${IDF_PATH}/components/mbedtls/mbedtls/library/md.c"
    "${IDF_PATH}/components/mbedtls/mbedtls/library/platform_util.c"
    "${IDF_PATH}/components/mbedtls/port/sha/core/sha.c"
    "${IDF_PATH}/components/mbedtls/port/aes/dma/esp_aes.c"
    "${IDF_PATH}/components/esp_security/src/esp_crypto_periph_clk.c"
    "${IDF_PATH}/components/esp_security/src/esp_hmac.c"
    "${IDF_PATH}/components/esp_security/src/esp_crypto_lock.c"
    "${IDF_PATH}/components/hal/hmac_hal.c"
    "${IDF_PATH}/components/hal/esp32p4/include/hal/hmac_ll.h"
    "${IDF_PATH}/components/hal/esp32p4/include/hal/key_mgr_ll.h"
    "${IDF_PATH}/components/hal/esp32p4/include/hal/mmu_ll.h"
    "${IDF_PATH}/components/bootloader_support/src/secure_boot_v2/secure_boot_signatures_app.c"
    "${IDF_PATH}/components/bootloader_support/include/esp_secure_boot.h"
    "${IDF_PATH}/components/esp_rom/esp32p4/include/esp32p4/rom/secure_boot.h")
