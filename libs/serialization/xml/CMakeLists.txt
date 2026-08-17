# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: BSD-3-Clause

set(kritaxmlserialization_LIB_SRCS
    KoXmlNS.cpp
    KoXmlWriter.cpp
)

kis_add_library(kritaxmlserialization SHARED ${kritaxmlserialization_LIB_SRCS})
generate_export_header(kritaxmlserialization BASE_NAME kritaxmlserialization)

target_link_libraries(kritaxmlserialization
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
)

target_include_directories(kritaxmlserialization
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

set_target_properties(kritaxmlserialization PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)
install(TARGETS kritaxmlserialization ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
