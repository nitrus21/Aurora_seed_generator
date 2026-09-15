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

function(aurora_crypto_replace_source suffix replacement original_directory)
    get_target_property(crypto_sources mbedcrypto SOURCES)
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
    set_property(TARGET mbedcrypto PROPERTY SOURCES "${updated_sources}")
    # Relative quoted includes in the SDK source keep their original context.
    target_include_directories(mbedcrypto PRIVATE "${IDF_PATH}/components/${original_directory}")
endfunction()

aurora_crypto_replace_source("md\\.c" "md.c" "mbedtls/mbedtls/library")
aurora_crypto_replace_source("platform_util\\.c" "platform_util.c" "mbedtls/mbedtls/library")
aurora_crypto_replace_source("sha/core/sha\\.c" "sha_core.c" "mbedtls/port/sha/core")
aurora_crypto_replace_source("aes/dma/esp_aes\\.c" "esp_aes_dma.c" "mbedtls/port/aes/dma")
target_compile_definitions(mbedcrypto PRIVATE AURORA_BOARD_P4=1)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${AURORA_ROOT}/tools/prepare_p4_crypto_overlay.py"
    "${AURORA_ROOT}/tools/patch_ubitcoin_p4.py"
    "${IDF_PATH}/components/mbedtls/mbedtls/library/md.c"
    "${IDF_PATH}/components/mbedtls/mbedtls/library/platform_util.c"
    "${IDF_PATH}/components/mbedtls/port/sha/core/sha.c"
    "${IDF_PATH}/components/mbedtls/port/aes/dma/esp_aes.c"
    "${IDF_PATH}/components/esp_security/src/esp_crypto_periph_clk.c")
