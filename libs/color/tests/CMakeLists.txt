kis_add_test(
    KisColorManagerPublicApiTest.cpp
    TEST_NAME KisColorManagerPublicApiTest
    LINK_LIBRARIES
        kritacolor
        Qt${QT_MAJOR_VERSION}::Test
)

if (kritacolor_USES_DUMMY_BACKEND)
    target_compile_definitions(
        KisColorManagerPublicApiTest
        PRIVATE KRITA_COLOR_MANAGER_USES_DUMMY_BACKEND
    )
endif()

file(GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/KisColorLifecycleSchemaContractTest_include/kritacolord_export.h"
    CONTENT "#pragma once\n#define KRITACOLORD_EXPORT\n"
)
kis_add_test(
    KisColorLifecycleSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisColorLifecycleSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../colord
    ${CMAKE_CURRENT_BINARY_DIR}/../colord
    ${CMAKE_CURRENT_BINARY_DIR}/KisColorLifecycleSchemaContractTest_include
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::DBus,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisColorLifecycleSchemaContractTest PRIVATE
    kritacolor_EXPORTS
    kritacolord_EXPORTS
)

if (UNIX AND NOT APPLE AND HAVE_DBUS)
    add_executable(
        FakeColordService
        FakeColordService.cpp
    )
    target_link_libraries(
        FakeColordService
        PRIVATE
            Qt${QT_MAJOR_VERSION}::Core
            Qt${QT_MAJOR_VERSION}::DBus
    )

    kis_add_test(
        KisColordPublicApiTest.cpp
        TEST_NAME KisColordPublicApiTest
        LINK_LIBRARIES
            kritacolor
            kritacolord
            Qt${QT_MAJOR_VERSION}::Test
    )
    add_dependencies(KisColordPublicApiTest FakeColordService)
    target_compile_definitions(
        KisColordPublicApiTest
        PRIVATE FAKE_COLORD_SERVICE_PATH="$<TARGET_FILE:FakeColordService>"
    )
endif()
