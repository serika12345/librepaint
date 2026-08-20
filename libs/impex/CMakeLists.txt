set(kritaimpex_LIB_SRCS
    KisImportExportAdditionalChecks.cpp
    KisImportExportErrorCode.cpp
    KisImportExportFilter.cpp
    KisImportExportFilterRegistry.cpp
    KisExportCheckBase.cpp
    KisPreExportChecker.cpp
    KisExportCheckRegistry.cpp
    metadata/KoDocumentInfo.cpp
)

kis_add_library(kritaimpex SHARED ${kritaimpex_HEADERS_MOC} ${kritaimpex_LIB_SRCS})

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

# These sources are separated as one ownership unit while their established
# ABI is still published by kritaui. The object library ensures every public
# implementation is incorporated into kritaui, including entry points used
# only by dynamically loaded import/export plugins. Later document and
# application package splits can turn this unit into an independently linked
# shared library without moving the import-export sources again.
kis_add_library(kritaimpexui OBJECT ${kritaimpexui_LIB_SRCS})
target_compile_definitions(kritaimpexui PRIVATE kritaui_EXPORTS)

target_include_directories(kritaimpexui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/animation>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/ui>
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
