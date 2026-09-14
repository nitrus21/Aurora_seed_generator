# The upstream IPA generator splits its JSON argument on whitespace. Resolve
# existing files to their Windows short aliases; no file is copied or relocated.
# Project includes run before component CMakeLists (including esp_ipa).
if(CMAKE_HOST_WIN32)
    idf_build_get_property(AURORA_IPA_PATHS ESP_IPA_JSON_CONFIG_FILE_PATH)
    if(AURORA_IPA_PATHS)
        idf_build_get_property(AURORA_PYTHON PYTHON)
        execute_process(COMMAND "${AURORA_PYTHON}"
            "${CMAKE_CURRENT_LIST_DIR}/../tools/short_paths.py" ${AURORA_IPA_PATHS}
            OUTPUT_VARIABLE AURORA_IPA_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE AURORA_SHORT_RESULT)
        if(NOT AURORA_SHORT_RESULT EQUAL 0)
            message(FATAL_ERROR "Cannot resolve space-free aliases for camera ISP configuration")
        endif()
        idf_build_set_property(ESP_IPA_JSON_CONFIG_FILE_PATH "${AURORA_IPA_SHORT}")
    endif()
endif()
