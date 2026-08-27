set(kritaimpex_LIB_SRCS
    KisImportExportFilterFactory.cpp
)

add_library(kritaimpexfilterstateobjects OBJECT
    KisImportExportFilter.cpp
)

add_library(kritaimpexfilterprogressobjects OBJECT
    KisImportExportFilterProgress.cpp
)

add_library(kritaimpexfilterconfigurationobjects OBJECT
    KisImportExportFilterConfiguration.cpp
)

add_library(kritaimpexfiltersavedconfigurationobjects OBJECT
    KisImportExportFilterSavedConfiguration.cpp
)

add_library(kritaimpexfiltercapabilityobjects OBJECT
    KisImportExportFilterCapabilities.cpp
)

add_library(kritaimpexfiltercolormodelobjects OBJECT
    KisImportExportFilterColorModels.cpp
)

add_library(kritaimpexfilterverificationobjects OBJECT
    KisImportExportFilterVerification.cpp
)

add_library(kritaimpexfilterzipverificationobjects OBJECT
    KisImportExportFilterZipVerification.cpp
)

add_library(kritaimpexexportcheckbaseobjects OBJECT
    KisExportCheckBase.cpp
)

add_library(kritaimpexexportcheckregistryobjects OBJECT
    KisExportCheckRegistry.cpp
)

add_library(kritaimpexpreexportcheckerobjects OBJECT
    KisPreExportChecker.cpp
)

add_library(kritaimpexadditionalchecksobjects OBJECT
    KisImportExportAdditionalChecks.cpp
)

add_library(kritaimpexfilterregistryobjects OBJECT
    KisImportExportFilterRegistry.cpp
)

add_library(kritaimpexerrorobjects OBJECT
    KisImportExportErrorCode.cpp
)

add_library(kritaimpexmetadataobjects OBJECT
    metadata/KoDocumentInfo.cpp
)

set(kritaimpex_OBJECT_TARGETS
    kritaimpexfilterstateobjects
    kritaimpexfilterprogressobjects
    kritaimpexfilterconfigurationobjects
    kritaimpexfiltersavedconfigurationobjects
    kritaimpexfiltercapabilityobjects
    kritaimpexfiltercolormodelobjects
    kritaimpexfilterverificationobjects
    kritaimpexfilterzipverificationobjects
    kritaimpexexportcheckbaseobjects
    kritaimpexexportcheckregistryobjects
    kritaimpexpreexportcheckerobjects
    kritaimpexadditionalchecksobjects
    kritaimpexfilterregistryobjects
    kritaimpexerrorobjects
    kritaimpexmetadataobjects
)

foreach(target IN LISTS kritaimpex_OBJECT_TARGETS)
    target_compile_definitions(${target} PRIVATE kritaimpex_EXPORTS)
    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    )
    set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endforeach()

set(kritaimpex_FILTER_HEADER_INCLUDE_DIRECTORIES
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/flake>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/flake>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)

add_library(kritaimpexfilterheaders INTERFACE)
target_include_directories(kritaimpexfilterheaders INTERFACE ${kritaimpex_FILTER_HEADER_INCLUDE_DIRECTORIES})
target_link_libraries(kritaimpexfilterheaders
    INTERFACE
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::I18n
)

target_link_libraries(kritaimpexfilterstateobjects PUBLIC kritaimpexfilterheaders)

target_link_libraries(kritaimpexfilterprogressobjects
    PUBLIC
        kritaimpexfilterheaders
        kritawidgetutils
)

target_link_libraries(kritaimpexfilterconfigurationobjects
    PUBLIC
        kritaimpexfilterheaders
)

target_link_libraries(kritaimpexfiltersavedconfigurationobjects
    PUBLIC
        kritaimpexfilterheaders
        kritaimage
)

target_link_libraries(kritaimpexfiltercapabilityobjects
    PUBLIC
        kritaimpexfilterheaders
)

target_link_libraries(kritaimpexfiltercolormodelobjects
    PUBLIC
        kritaimpexfilterheaders
        kritaimage
        kritapigment
)

target_link_libraries(kritaimpexfilterverificationobjects
    PUBLIC
        kritaimpexfilterheaders
)

target_link_libraries(kritaimpexfilterzipverificationobjects
    PUBLIC
        kritaimpexfilterheaders
        kritaresourcestorage
)

set_target_properties(
    kritaimpexfilterprogressobjects
    kritaimpexfilterconfigurationobjects
    kritaimpexfiltersavedconfigurationobjects
    kritaimpexfiltercapabilityobjects
    kritaimpexfiltercolormodelobjects
    kritaimpexfilterverificationobjects
    kritaimpexfilterzipverificationobjects
    PROPERTIES AUTOMOC OFF
)

target_link_libraries(kritaimpexexportcheckbaseobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
)

target_link_libraries(kritaimpexexportcheckregistryobjects
    PUBLIC
        kritaimage
        kritapigment
        Qt${QT_MAJOR_VERSION}::Core
        KF${KF_MAJOR}::I18n
)

target_link_libraries(kritaimpexpreexportcheckerobjects
    PUBLIC
        kritaimage
        Qt${QT_MAJOR_VERSION}::Core
)

target_link_libraries(kritaimpexadditionalchecksobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
)

target_link_libraries(kritaimpexfilterregistryobjects
    PUBLIC
        kritaplugin
        Qt${QT_MAJOR_VERSION}::Core
)

target_link_libraries(kritaimpexerrorobjects
    PUBLIC
        kritaglobal
        Qt${QT_MAJOR_VERSION}::Core
        KF${KF_MAJOR}::I18n
)

target_link_libraries(kritaimpexmetadataobjects
    PUBLIC
        kritaresources
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
)

kis_add_library(kritaimpex SHARED
    ${kritaimpex_HEADERS_MOC}
    ${kritaimpex_LIB_SRCS}
    $<TARGET_OBJECTS:kritaimpexfilterstateobjects>
    $<TARGET_OBJECTS:kritaimpexfilterprogressobjects>
    $<TARGET_OBJECTS:kritaimpexfilterconfigurationobjects>
    $<TARGET_OBJECTS:kritaimpexfiltersavedconfigurationobjects>
    $<TARGET_OBJECTS:kritaimpexfiltercapabilityobjects>
    $<TARGET_OBJECTS:kritaimpexfiltercolormodelobjects>
    $<TARGET_OBJECTS:kritaimpexfilterverificationobjects>
    $<TARGET_OBJECTS:kritaimpexfilterzipverificationobjects>
    $<TARGET_OBJECTS:kritaimpexexportcheckbaseobjects>
    $<TARGET_OBJECTS:kritaimpexexportcheckregistryobjects>
    $<TARGET_OBJECTS:kritaimpexpreexportcheckerobjects>
    $<TARGET_OBJECTS:kritaimpexadditionalchecksobjects>
    $<TARGET_OBJECTS:kritaimpexfilterregistryobjects>
    $<TARGET_OBJECTS:kritaimpexerrorobjects>
    $<TARGET_OBJECTS:kritaimpexmetadataobjects>
)

generate_export_header(kritaimpex BASE_NAME kritaimpex)

target_link_libraries(kritaimpex
    PUBLIC
        KF${KF_MAJOR}::CoreAddons
        KF${KF_MAJOR}::I18n
        kritaimage
        kritaresourcestorage
    PRIVATE
        kritaplugin
        kritaresources
        kritawidgetutils
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::ConfigCore
)

set_target_properties(kritaimpex
    PROPERTIES VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritaimpex  ${INSTALL_TARGETS_DEFAULT_ARGS})

set(kritaimpexui_LIB_SRCS
    ui/KisImportExportAsyncFeedback.cpp
    ui/KisImportExportManager.cpp
    ui/KisImportExportUtils.cpp
    ui/KisImportUserFeedbackInterface.cpp
    ui/KisRemoteFileFetcher.cpp
    ui/KisSynchronousImportUserFeedback.cpp
    ui/KisSaveGroupVisitor.cpp
    ui/KisImportExportDialogs.cpp
    ui/kis_dlg_missing_color_profile.cpp
    ui/kis_dlg_paste_format.cpp
    ui/kis_dlg_png_import.cpp
    ui/kis_animation_importer.cpp
    ui/kis_clipboard.cc
    ui/kis_import_catcher.cc
    ui/kis_mimedata.cpp
    ui/kis_png_converter.cpp
    ui/kis_safe_document_loader.cpp
)

ki18n_wrap_ui(kritaimpexui_LIB_SRCS
    ui/forms/wdgMissingColorProfile.ui
    ui/forms/wdgPasteFormat.ui
    ui/forms/wdgdlgpngimport.ui
)

if(NOT IOS)
    list(APPEND kritaimpexui_LIB_SRCS
        animation/KisFFMpegWrapper.cpp
        animation/KisVideoSaver.cpp
        animation/KisAnimationRenderingOptions.cpp
        animation/KisAnimationRender.cpp
        animation/KisDlgAnimationRenderer.cpp
        animation/VideoExportOptionsDialog.cpp
        animation/VideoHDRMetadataOptionsDialog.cpp
        animation/KisHDRMetadataOptions.cpp
        animation/KisDlgImportVideoAnimation.cpp
    )

    ki18n_wrap_ui(kritaimpexui_LIB_SRCS
        animation/wdg_animationrenderer.ui
        animation/video_export_options_dialog.ui
        animation/VideoHDRMetadataOptionsDialog.ui
        animation/VideoImportDialog.ui
    )
endif()

if(ANDROID)
    list(APPEND kritaimpexui_LIB_SRCS
        animation/KisAndroidMediaEncoderRunnable.cpp
        animation/KisLibavMediaEncoderRunnable.cpp
        animation/KisMediaEncoderFormatPreferencesDialog.cpp
        animation/KisMediaEncoderWrapper.cpp
    )
endif()

# Import and export presentation sources form one ownership unit. Their object
# implementation is incorporated into the application presentation library,
# including entry points used only by dynamically loaded plugins.
kis_add_library(kritaimpexui OBJECT ${kritaimpexui_LIB_SRCS})
target_compile_definitions(kritaimpexui PRIVATE kritaapplicationui_EXPORTS)

target_include_directories(kritaimpexui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/animation>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/application>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/ui
        ${CMAKE_SOURCE_DIR}/libs/ui/canvas
        ${CMAKE_SOURCE_DIR}/libs/ui/flake
        ${CMAKE_SOURCE_DIR}/libs/ui/ora
        ${CMAKE_SOURCE_DIR}/libs/ui/tool
        ${CMAKE_SOURCE_DIR}/libs/ui/utils
        ${CMAKE_SOURCE_DIR}/libs/ui/widgets
        ${CMAKE_SOURCE_DIR}/libs/ui/widgets/gradient
)

target_link_libraries(kritaimpexui
    PUBLIC
        kritaapplication
        kritaimpex
        kritaimage
        kritapainting
        kritapaintingmetadata
        kritapaintingundo
        kritalibbrush
        kritapigment
        kritaresources
        kritaresourcestorage
        kritaresourceui
        kritawidgets
        kritawidgetutils
        kritaxmlserialization
        KF${KF_MAJOR}::Completion
        KF${KF_MAJOR}::CoreAddons
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ItemViews
        Qt${QT_MAJOR_VERSION}::Concurrent
        Qt${QT_MAJOR_VERSION}::Network
        Qt${QT_MAJOR_VERSION}::Widgets
        ${PNG_LIBRARIES}
)

if(ANDROID)
    target_link_libraries(kritaimpexui
        PRIVATE
            Qt${QT_MAJOR_VERSION}::AndroidExtras
    )

    if(TARGET LibAV)
        target_link_libraries(kritaimpexui PRIVATE LibAV)
    endif()

    if(Mlt7_FOUND)
        target_link_libraries(kritaimpexui PRIVATE Mlt7::mlt Mlt7::mlt++)
    endif()
endif()

add_subdirectory(tests)
