kis_add_test(
    KisQQuickWidgetsPublicApiTest.cpp
    TEST_NAME KisQQuickWidgetsPublicApiTest
    TEST_NAME_VAR qml_widgets_public_api_test_name
    LINK_LIBRARIES
        kritaqmlwidgets
        Qt${QT_MAJOR_VERSION}::Test
)

get_target_property(
    qt_quick_library
    Qt${QT_MAJOR_VERSION}::Quick
    IMPORTED_LOCATION_RELEASE
)
if (NOT qt_quick_library)
    get_target_property(
        qt_quick_library
        Qt${QT_MAJOR_VERSION}::Quick
        IMPORTED_LOCATION
    )
endif()

set(qt_quick_search_directory "${qt_quick_library}")
unset(qt_quick_qml_directory)
foreach(_level RANGE 0 6)
    file(GLOB qt_quick_qml_candidates
        LIST_DIRECTORIES true
        "${qt_quick_search_directory}/lib/qt*/qml"
    )
    if (qt_quick_qml_candidates)
        list(GET qt_quick_qml_candidates 0 qt_quick_qml_directory)
        break()
    endif()
    cmake_path(GET qt_quick_search_directory PARENT_PATH qt_quick_search_directory)
endforeach()

if (qt_quick_qml_directory)
    set_tests_properties(
        ${qml_widgets_public_api_test_name}
        PROPERTIES ENVIRONMENT
            "QML_IMPORT_PATH=${qt_quick_qml_directory};QML2_IMPORT_PATH=${qt_quick_qml_directory}"
    )
endif()
