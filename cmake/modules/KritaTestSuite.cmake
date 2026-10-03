# SPDX-FileCopyrightText: 2023 L. E. Segovia <amy@amyspark.me>
# SPDX-License-Ref: BSD-3-Clause

macro(set_test_sdk_compile_definitions _tgt)
    if(ANDROID)
        target_compile_definitions(${_tgt} PUBLIC
            FILES_DATA_DIR="data/"
            FILES_OUTPUT_DIR="."
        )
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-data")
        file(GENERATE
            OUTPUT "${CMAKE_BINARY_DIR}/test-data/${_tgt}.path"
            CONTENT "${CMAKE_CURRENT_SOURCE_DIR}/data/\n"
        )
    else()
        target_compile_definitions(${_tgt} PUBLIC FILES_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/")
    endif()
    if(WIN32)
        target_compile_definitions(${_tgt} PUBLIC FILES_OUTPUT_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    elseif(NOT ANDROID)
        target_compile_definitions(${_tgt} PUBLIC FILES_OUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}")
    endif()
endmacro()
