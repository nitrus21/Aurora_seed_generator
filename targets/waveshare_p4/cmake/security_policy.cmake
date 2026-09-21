# Build-time policy only: this file never enables security eFuses or claims that
# an unprovisioned device is resistant to physical access. Keep sensitive task
# stacks / model state and Mbed TLS contexts out of optional external placement.
if(NOT CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT OR CONFIG_ESP_DEBUG_OCDAWARE
   OR NOT CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_SILENT)
    message(FATAL_ERROR "AURORA P4 requires silent panic/assertion configuration")
endif()

if(NOT CONFIG_APP_REPRODUCIBLE_BUILD OR CONFIG_APP_COMPILE_TIME_DATE)
    message(FATAL_ERROR "AURORA P4 requires reproducible images without compile timestamps")
endif()

if(NOT CONFIG_COMPILER_STACK_CHECK_MODE_STRONG
   OR CONFIG_COMPILER_STACK_CHECK_MODE_NONE
   OR CONFIG_COMPILER_STACK_CHECK_MODE_NORM
   OR CONFIG_COMPILER_STACK_CHECK_MODE_ALL)
    message(FATAL_ERROR "AURORA P4 requires exactly strong compiler stack protection")
endif()

if(NOT CONFIG_MBEDTLS_INTERNAL_MEM_ALLOC OR CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC
   OR CONFIG_MBEDTLS_DEFAULT_MEM_ALLOC OR CONFIG_MBEDTLS_CUSTOM_MEM_ALLOC)
    message(FATAL_ERROR "AURORA P4 requires internal-only Mbed TLS allocation")
endif()

if(CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY
   OR CONFIG_SPIRAM_ALLOW_NOINIT_SEG_EXTERNAL_MEMORY
   OR CONFIG_ESP_LVGL_ADAPTER_LVGL_THREAD_STACK_IN_PSRAM)
    message(FATAL_ERROR "AURORA P4 forbids external model sections / LVGL task stack")
endif()

if(CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH OR CONFIG_ESP_COREDUMP_ENABLE_TO_UART)
    message(FATAL_ERROR "AURORA P4 forbids persistent / serial memory dumps")
endif()

if(CONFIG_SPIRAM_ENC_EXEMPT)
    message(FATAL_ERROR "AURORA P4 forbids PSRAM regions exempt from encryption")
endif()

# The pinned IDF marks P4 hardware ECDSA Secure Boot unsafe (ECDSA_DS-836/837).
# An eventual provisioned profile must use RSA, never force the unsafe option.
# This does not prohibit Bitcoin secp256k1 operations implemented in software.
if(CONFIG_SECURE_BOOT_V2_FORCE_ENABLE_ECDSA
   OR (CONFIG_SECURE_BOOT AND CONFIG_SECURE_SIGNED_APPS_ECDSA_V2_SCHEME))
    message(FATAL_ERROR "AURORA P4 forbids hardware ECDSA Secure Boot; review RSA provisioning")
endif()

# ROM-764: the generic rev3 profile includes v3.0, on which Espressif says not
# to enable Secure Boot. A provisioned build needs its actual revision bound.
if(CONFIG_SECURE_BOOT)
    if(NOT DEFINED CONFIG_ESP32P4_REV_MIN_FULL)
        message(FATAL_ERROR "AURORA P4 Secure Boot requires an explicit silicon revision")
    elseif(CONFIG_ESP32P4_REV_MIN_FULL EQUAL 300)
        message(FATAL_ERROR "AURORA P4 ROM-764: Secure Boot is forbidden on revision 3.0")
    endif()
endif()
