##### Tests that currently fail and should be fixed #####

# Works under Linux but does not work under Windows
krita_add_broken_unit_test( kis_exif_test.cpp
    TEST_NAME KisExifTest
    LINK_LIBRARIES kritapaintingmetadata kritaapplicationui kritatestsdk
    )

kis_add_test(
    KisExiv2IODeviceSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisExiv2IODeviceSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_BINARY_DIR}/../common
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/metadata
    ${CMAKE_BINARY_DIR}/libs/painting/metadata
    $<TARGET_PROPERTY:LibExiv2::LibExiv2,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisExiv2IODeviceSchemaContractTest PRIVATE
    kritaexifcommon_EXPORTS
    kritaglobal_EXPORTS
    kritapaintingmetadata_EXPORTS
)
