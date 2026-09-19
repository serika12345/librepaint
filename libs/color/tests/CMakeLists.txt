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
