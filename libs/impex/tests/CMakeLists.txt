kis_add_tests(
    TestImportExportBoundary.cpp
    LINK_LIBRARIES kritaimpexadditionalchecksobjects kritaimpexfilterregistryobjects Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(TestImportExportBoundary PRIVATE kritaimpex_EXPORTS)

kis_add_tests(
    kis_import_export_filter_factory_test.cpp
    LINK_LIBRARIES kritaimpex Qt${QT_MAJOR_VERSION}::Test
)

kis_add_tests(
    kis_import_export_filter_test.cpp
    LINK_LIBRARIES
        kritaimpexfilterstateobjects
        kritaimpexfilterprogressobjects
        kritaimpexfilterconfigurationobjects
        kritaimpexfiltercapabilityobjects
        kritaimpexfilterverificationobjects
        kritaimpexexportcheckbaseobjects
        kritaimpexerrorobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_import_export_filter_test PRIVATE kritaimpex_EXPORTS)

kis_add_tests(
    kis_import_export_filter_saved_configuration_test.cpp
    LINK_LIBRARIES
        kritaimpexfilterstateobjects
        kritaimpexfilterprogressobjects
        kritaimpexfilterconfigurationobjects
        kritaimpexfiltersavedconfigurationobjects
        kritaimpexfiltercapabilityobjects
        kritaimpexfilterverificationobjects
        kritaimpexexportcheckbaseobjects
        kritaimpexerrorobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_import_export_filter_saved_configuration_test PRIVATE kritaimpex_EXPORTS)

kis_add_tests(
    kis_import_user_feedback_test.cpp
    TEST_NAMES_VAR import_user_feedback_test_names
    LINK_LIBRARIES
        kritaimpexuserfeedbackobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(kis_import_user_feedback_test PRIVATE kritaapplicationui_EXPORTS)
if(UNIX AND NOT APPLE)
    set_tests_properties(
        ${import_user_feedback_test_names}
        PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

kis_add_tests(
    kis_animation_rendering_options_test.cpp
    LINK_LIBRARIES
        kritaimpexanimationoptionsstateobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_animation_rendering_options_test PRIVATE kritaapplicationui_EXPORTS)

kis_add_tests(
    kis_animation_rendering_options_persistence_test.cpp
    LINK_LIBRARIES
        kritaimpexanimationoptionsstateobjects
        kritaimpexanimationoptionspersistenceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_animation_rendering_options_persistence_test PRIVATE kritaapplicationui_EXPORTS)

kis_add_tests(
    kis_animation_render_dimensions_test.cpp
    LINK_LIBRARIES
        kritaimpexanimationrenderdimensionsobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_animation_render_dimensions_test PRIVATE kritaapplicationui_EXPORTS)

kis_add_test(
    KisMediaEncoderFormatAndSettingsContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisMediaEncoderFormatAndSettingsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../animation
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_BINARY_DIR}/libs/application
    ${CMAKE_BINARY_DIR}/libs/impex
)
target_compile_definitions(KisMediaEncoderFormatAndSettingsContractTest PRIVATE
    kritaapplicationui_EXPORTS
    kritaimpex_EXPORTS
)

kis_add_test(
    KisPNGConverterSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisPNGConverterSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../ui
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_BINARY_DIR}/libs/impex
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/application
    ${PNG_INCLUDE_DIR}
)
target_compile_definitions(KisPNGConverterSchemaContractTest PRIVATE
    kritaapplicationui_EXPORTS
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritaimpex_EXPORTS
)

kis_add_test(
    KisClipboardSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisClipboardSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../ui
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/application
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisClipboardSchemaContractTest PRIVATE
    kritaapplicationui_EXPORTS
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisDlgImportVideoAnimationSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
ki18n_wrap_ui(KisDlgImportVideoAnimationSchemaContractTest
    ${CMAKE_CURRENT_SOURCE_DIR}/../animation/VideoImportDialog.ui
)
target_include_directories(KisDlgImportVideoAnimationSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../animation
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_SOURCE_DIR}/libs/application
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/ui
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/widgets
    ${CMAKE_BINARY_DIR}/libs/application
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/impex
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/ui
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgets
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::ConfigCore,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::WidgetsAddons,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisDlgImportVideoAnimationSchemaContractTest PRIVATE
    kritaapplicationui_EXPORTS
    kritaimpex_EXPORTS
    kritaui_EXPORTS
)

kis_add_test(
    KisFFMpegWrapperSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisFFMpegWrapperSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../animation
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/application
    ${CMAKE_BINARY_DIR}/libs/impex
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisFFMpegWrapperSchemaContractTest PRIVATE
    kritaapplicationui_EXPORTS
    kritaimpex_EXPORTS
)

kis_add_tests(
    kis_remote_file_fetcher_test.cpp
    TEST_NAMES_VAR remote_file_fetcher_test_names
    LINK_LIBRARIES
        kritaimpexremotefilefetcherobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Network
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(kis_remote_file_fetcher_test PRIVATE kritaapplicationui_EXPORTS)
if(UNIX AND NOT APPLE)
    set_tests_properties(
        ${remote_file_fetcher_test_names}
        PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()

kis_add_tests(
    kis_import_export_utils_values_test.cpp
    LINK_LIBRARIES
        kritaimpexfilterheaders
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_import_export_utils_values_test PRIVATE kritaapplicationui_EXPORTS)
target_include_directories(kis_import_export_utils_values_test
    PRIVATE
        ${CMAKE_BINARY_DIR}/libs/application
        ${CMAKE_CURRENT_SOURCE_DIR}/../ui
)

kis_add_tests(
    kis_import_export_utils_test.cpp
    LINK_LIBRARIES
        kritaimpexerrorobjects
        kritaimpexuserfeedbackobjects
        kritaimpexutilsobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_import_export_utils_test PRIVATE kritaapplicationui_EXPORTS)

kis_add_tests(
    TestImportExportPublicHeaders.cpp
    LINK_LIBRARIES kritaresourcestorage Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(TestImportExportPublicHeaders
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/image
        ${CMAKE_BINARY_DIR}/libs/image
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_tests(
    kis_store_paintdevice_writer_test.cpp
    LINK_LIBRARIES kritaresourcestorage Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(kis_store_paintdevice_writer_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/image
        ${CMAKE_BINARY_DIR}/libs/image
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_tests(
    kis_import_export_error_code_test.cpp
    LINK_LIBRARIES kritaimpexerrorobjects Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_import_export_error_code_test PRIVATE kritaimpex_EXPORTS)

kis_add_tests(
    kis_document_metadata_test.cpp
    LINK_LIBRARIES kritaimpexmetadataobjects Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Xml
)
target_compile_definitions(kis_document_metadata_test PRIVATE kritaimpex_EXPORTS)

kis_add_tests(
    kis_export_check_base_test.cpp
    LINK_LIBRARIES kritaimpexexportcheckbaseobjects Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_export_check_base_test PRIVATE kritaimpex_EXPORTS)
target_include_directories(kis_export_check_base_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_tests(
    kis_export_checks_test.cpp
    LINK_LIBRARIES
        kritaimpexexportcheckbaseobjects
        kritaimpexexportcheckregistryobjects
        kritaimpexpreexportcheckerobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_export_checks_test PRIVATE kritaimpex_EXPORTS)

kis_add_tests(
    TestImportExportUiBoundary.cpp
    LINK_LIBRARIES kritaapplicationui Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Widgets
)
