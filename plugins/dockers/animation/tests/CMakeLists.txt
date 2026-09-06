kis_add_tests(
    kis_animation_utils_test.cpp
    timeline_model_test.cpp
    LINK_LIBRARIES kritaanimationdocker_static kritaapplicationui kritaimage kritatestsdk
    )

kis_add_test(
    KisAnimUtilsSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisAnimUtilsSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisAnimUtilsSchemaContractTest PRIVATE
    kritaanimationdocker_static_EXPORTS
    kritaimage_EXPORTS
    kritaglobal_EXPORTS
)
