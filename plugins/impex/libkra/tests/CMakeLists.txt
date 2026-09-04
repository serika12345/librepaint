kis_add_tests(
    kis_kra_loader_test.cpp
    LINK_LIBRARIES kritaapplicationui kritalibkra kritatestsdk
    )


kis_add_test(
    kis_kra_saver_test.cpp
    LINK_LIBRARIES kritaapplicationui kritalibkra kritatransformmaskstubs
    )

kis_add_test(
    KisKraSaveXmlVisitorSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisKraSaveXmlVisitorSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Xml,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisKraSaveXmlVisitorSchemaContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibkra_EXPORTS
)
