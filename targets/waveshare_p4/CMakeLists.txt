cmake_minimum_required(VERSION 3.16)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../../include/version.h")
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../../include/version.h" AURORA_VERSION_LINE
     REGEX "^#define AURORA_P4_FIRMWARE_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+\"$")
string(REGEX REPLACE
       "^#define AURORA_P4_FIRMWARE_VERSION \"([0-9]+\\.[0-9]+\\.[0-9]+)\"$"
       "\\1" PROJECT_VER "${AURORA_VERSION_LINE}")
if(NOT PROJECT_VER MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "Cannot derive the P4 application version from AURORA_P4_FIRMWARE_VERSION")
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(COMPONENTS main)
if(EXISTS "$ENV{IDF_PATH}/components/__pio_env")
    list(APPEND COMPONENTS __pio_env)
endif()
project(aurora_p4)

# Managed sources are version-pinned and validated before patching. A changed
# dependency fails configuration instead of silently omitting OOM cleanup.
idf_build_get_property(AURORA_SECURITY_PYTHON PYTHON)
execute_process(COMMAND "${AURORA_SECURITY_PYTHON}"
    "${CMAKE_SOURCE_DIR}/../../tools/patch_lvgl.py"
    --lvgl-root "${CMAKE_SOURCE_DIR}/managed_components/lvgl__lvgl"
    RESULT_VARIABLE AURORA_LVGL_PATCH_RESULT)
if(NOT AURORA_LVGL_PATCH_RESULT EQUAL 0)
    message(FATAL_ERROR "AURORA LVGL hardening failed")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/../../tools/patch_lvgl.py")

# Check effective SDK settings, not only defaults. This is a non-provisioning
# guard against unsafe configuration drift; it does not activate any eFuse.
include("${CMAKE_CURRENT_LIST_DIR}/cmake/security_policy.cmake")

# ESP-IDF's reproducible mode adds one debug-prefix mapping per component.
# pioarduino currently splits quoted options containing spaces while importing
# CMake's model into SCons. Use aliases of the same Windows directories so the
# security mode also remains buildable from a workspace whose name has spaces.
if(CMAKE_HOST_WIN32 AND CONFIG_APP_REPRODUCIBLE_BUILD)
    idf_build_get_property(AURORA_COMPILE_OPTIONS COMPILE_OPTIONS)
    set(AURORA_FIXED_COMPILE_OPTIONS "")
    foreach(AURORA_OPTION IN LISTS AURORA_COMPILE_OPTIONS)
        if(AURORA_OPTION MATCHES "^-f(debug|macro)-prefix-map=(.*)=(.*)$")
            set(AURORA_PREFIX_KIND "${CMAKE_MATCH_1}")
            set(AURORA_PREFIX_SOURCE "${CMAKE_MATCH_2}")
            set(AURORA_PREFIX_TARGET "${CMAKE_MATCH_3}")
            if(AURORA_PREFIX_SOURCE MATCHES "[ \\t]")
                execute_process(COMMAND "${AURORA_SECURITY_PYTHON}"
                    "${CMAKE_SOURCE_DIR}/tools/short_paths.py" "${AURORA_PREFIX_SOURCE}"
                    OUTPUT_VARIABLE AURORA_PREFIX_ALIAS OUTPUT_STRIP_TRAILING_WHITESPACE
                    RESULT_VARIABLE AURORA_PREFIX_RESULT)
                if(NOT AURORA_PREFIX_RESULT EQUAL 0)
                    message(FATAL_ERROR "Cannot resolve reproducible-build path alias")
                endif()
                list(APPEND AURORA_FIXED_COMPILE_OPTIONS
                    "-f${AURORA_PREFIX_KIND}-prefix-map=${AURORA_PREFIX_ALIAS}=${AURORA_PREFIX_TARGET}")
            else()
                list(APPEND AURORA_FIXED_COMPILE_OPTIONS "${AURORA_OPTION}")
            endif()
        else()
            list(APPEND AURORA_FIXED_COMPILE_OPTIONS "${AURORA_OPTION}")
        endif()
    endforeach()
    idf_build_set_property(COMPILE_OPTIONS "${AURORA_FIXED_COMPILE_OPTIONS}")
endif()

# Reject stale configuration caches instead of producing an image for the
# wrong, hardware-incompatible silicon family under a misleading filename.
if(SDKCONFIG MATCHES "waveshare-p4-rev1(-(kdf-benchmark|entropy-health|stack-health))?$")
    if(NOT CONFIG_ESP32P4_SELECTS_REV_LESS_V3 OR NOT CONFIG_ESP32P4_REV_MIN_100 OR NOT CONFIG_SPIRAM_SPEED_200M)
        message(FATAL_ERROR "AURORA rev1 profile mismatch: regenerate the selected rev1 sdkconfig")
    endif()
elseif(SDKCONFIG MATCHES "waveshare-p4$")
    if(CONFIG_ESP32P4_SELECTS_REV_LESS_V3 OR NOT CONFIG_ESP32P4_REV_MIN_300 OR NOT CONFIG_SPIRAM_SPEED_250M)
        message(FATAL_ERROR "AURORA rev3 profile mismatch: regenerate sdkconfig.waveshare-p4")
    endif()
endif()

# pioarduino sorts a forced-include option separately from its operand on every
# host. Keep the vendor alignment header enabled by combining both into one
# compiler argument. Windows additionally needs the existing, space-free alias.
if(CMAKE_HOST_WIN32)
    idf_build_get_property(AURORA_IDF_PYTHON PYTHON)
    execute_process(COMMAND "${AURORA_IDF_PYTHON}"
        "${CMAKE_SOURCE_DIR}/tools/short_paths.py"
        "${CMAKE_SOURCE_DIR}/managed_components/espressif__esp_lvgl_adapter/src/display/ports/lvgl_port_alignment.h"
        OUTPUT_VARIABLE AURORA_ALIGNMENT_HEADER OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE AURORA_ALIGNMENT_RESULT)
    if(NOT AURORA_ALIGNMENT_RESULT EQUAL 0)
        message(FATAL_ERROR "Cannot resolve LVGL alignment header alias")
    endif()
else()
    set(AURORA_ALIGNMENT_HEADER
        "${CMAKE_SOURCE_DIR}/managed_components/espressif__esp_lvgl_adapter/src/display/ports/lvgl_port_alignment.h")
endif()
foreach(AURORA_COMPONENT espressif__esp_lvgl_adapter lvgl__lvgl)
    idf_component_get_property(AURORA_LIB ${AURORA_COMPONENT} COMPONENT_LIB)
    foreach(AURORA_PROPERTY COMPILE_OPTIONS INTERFACE_COMPILE_OPTIONS)
        get_target_property(AURORA_OPTIONS ${AURORA_LIB} ${AURORA_PROPERTY})
        set(AURORA_FIXED_OPTIONS "")
        foreach(AURORA_OPTION IN LISTS AURORA_OPTIONS)
            if(AURORA_OPTION STREQUAL "-include")
                # Keep the option and its operand together: pioarduino
                # sorts extracted options before passing them to GCC.
                continue()
            elseif(AURORA_OPTION MATCHES "/lvgl_port_alignment\\.h$")
                list(APPEND AURORA_FIXED_OPTIONS "-include${AURORA_ALIGNMENT_HEADER}")
            else()
                list(APPEND AURORA_FIXED_OPTIONS "${AURORA_OPTION}")
            endif()
        endforeach()
        set_property(TARGET ${AURORA_LIB} PROPERTY ${AURORA_PROPERTY} "${AURORA_FIXED_OPTIONS}")
    endforeach()
endforeach()

# pioarduino drops quoted prebuilt-archive fragments from CMake's model.
# Supply the SAME vendor ISP archive through explicit -L / -l arguments.
if(CMAKE_HOST_WIN32)
    execute_process(COMMAND "${AURORA_IDF_PYTHON}"
        "${CMAKE_SOURCE_DIR}/tools/short_paths.py"
        "${CMAKE_SOURCE_DIR}/managed_components/espressif__esp_ipa/lib/esp32p4/v5.5"
        OUTPUT_VARIABLE AURORA_IPA_LIBRARY_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE AURORA_IPA_LIBRARY_RESULT)
    if(NOT AURORA_IPA_LIBRARY_RESULT EQUAL 0)
        message(FATAL_ERROR "Cannot resolve vendor ISP library directory")
    endif()
else()
    set(AURORA_IPA_LIBRARY_DIR
        "${CMAKE_SOURCE_DIR}/managed_components/espressif__esp_ipa/lib/esp32p4/v5.5")
endif()
idf_component_get_property(AURORA_IPA_LIB espressif__esp_ipa COMPONENT_LIB)
target_link_directories(${AURORA_IPA_LIB} INTERFACE "${AURORA_IPA_LIBRARY_DIR}")
target_link_libraries(${AURORA_IPA_LIB} INTERFACE "-lesp_ipa")

# CMake on Windows can canonicalize 8.3 and long paths differently when matching
# esp_ipa's generated source to its custom command. Generate the REAL ISP config
# at configure time too, with the official generator and selected sensor JSON.
if(CMAKE_HOST_WIN32 AND CONFIG_ESP_VIDEO_ENABLE_ISP_PIPELINE_CONTROLLER)
    idf_build_get_property(AURORA_IDF_PYTHON PYTHON)
    idf_build_get_property(AURORA_IPA_JSON ESP_IPA_JSON_CONFIG_FILE_PATH)
    set(AURORA_IPA_ARGS -o "${CMAKE_BINARY_DIR}/esp-idf/espressif__esp_ipa/esp_video_ipa_config.c"
        -v "${CONFIG_ESP_IPA_CONFIG_PARAM_VERSION}")
    if(AURORA_IPA_JSON)
        list(APPEND AURORA_IPA_ARGS -i ${AURORA_IPA_JSON})
    endif()
    execute_process(COMMAND "${AURORA_IDF_PYTHON}"
        "${CMAKE_SOURCE_DIR}/managed_components/espressif__esp_ipa/tools/config/esp_ipa_config.py"
        ${AURORA_IPA_ARGS} RESULT_VARIABLE AURORA_IPA_RESULT)
    if(NOT AURORA_IPA_RESULT EQUAL 0)
        message(FATAL_ERROR "AURORA: ISP configuration generation failed")
    endif()
endif()

# PlatformIO uses this generated file's timestamp as its configuration stamp.
# CMake otherwise leaves an unchanged cache older than edited CMakeLists files,
# causing a full dependency/configuration pass on every incremental build.
file(TOUCH_NOCREATE "${CMAKE_BINARY_DIR}/CMakeCache.txt")
