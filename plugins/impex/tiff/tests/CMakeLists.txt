kis_add_test(
    kis_tiff_test.cpp
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

if (TIFF_HAS_PSD_TAGS)
    kis_add_test(
        PsdResourceIdContractTest.cpp
        LINK_LIBRARIES
            Qt${QT_MAJOR_VERSION}::Test
    )
    target_include_directories(PsdResourceIdContractTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_BINARY_DIR}/..
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
        ${CMAKE_SOURCE_DIR}/libs/image
        ${CMAKE_BINARY_DIR}/libs/image
        ${CMAKE_SOURCE_DIR}/libs/psd
        ${CMAKE_BINARY_DIR}/libs/psd
        ${CMAKE_SOURCE_DIR}/libs/psdutils
        ${CMAKE_BINARY_DIR}/libs/psdutils
    )
    target_compile_definitions(PsdResourceIdContractTest PRIVATE
        kritapsd_EXPORTS
        kritatiffpsd_EXPORTS
    )
endif()
