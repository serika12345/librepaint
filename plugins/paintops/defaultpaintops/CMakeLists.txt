add_subdirectory(brush/tests)

include_directories(brush
                    duplicate
                    ${CMAKE_CURRENT_BINARY_DIR})

set(kritapixelbrush_SOURCES
    brush/kis_brushop.cpp
    brush/KisBrushOpResources.cpp
    brush/KisBrushOpSettings.cpp
    brush/KisDabRenderingQueue.cpp
    brush/KisDabRenderingQueueCache.cpp
    brush/KisDabRenderingJob.cpp
    brush/KisDabRenderingExecutor.cpp
)

set(kritadefaultpaintops_SOURCES
    brush/kis_brushop_settings_widget.cpp
    duplicate/kis_duplicateop.cpp
    duplicate/kis_duplicateop_settings.cpp
    duplicate/kis_duplicateop_settings_widget.cpp
    duplicate/KisDuplicateOptionData.cpp
    duplicate/KisDuplicateOptionModel.cpp
    duplicate/KisDuplicateOptionWidget.cpp
)

ki18n_wrap_ui(kritadefaultpaintops_SOURCES duplicate/wdgduplicateop.ui )

kis_add_library(kritapixelbrush STATIC ${kritapixelbrush_SOURCES})
target_link_libraries(kritapixelbrush
    PUBLIC
        kritaglobal
        kritaimage
        kritalibbrush
        kritalibpaintop
        kritapainting
        kritapigment
        Qt${QT_MAJOR_VERSION}::Core
)

kis_add_library(kritadefaultpaintops_static STATIC ${kritadefaultpaintops_SOURCES})
generate_export_header(kritadefaultpaintops_static BASE_NAME kritadefaultpaintops EXPORT_MACRO_NAME KRITADEFAULTPAINTOPS_EXPORT)

target_link_libraries(kritadefaultpaintops_static
    PUBLIC
        kritaimage
        kritalibpaintop
        kritapainting
        kritapixelbrush
        Qt${QT_MAJOR_VERSION}::Core
    PRIVATE
        kritaglobal
        kritalibbrush
        kritapigment
        kritawidgets
        kritawidgetutils
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
)

kis_add_library(kritadefaultpaintops MODULE defaultpaintops_plugin.cc)

target_link_libraries(kritadefaultpaintops kritadefaultpaintops_static)

if (TARGET FreehandStrokeContractTest)
    target_include_directories(FreehandStrokeContractTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/brush
        ${CMAKE_SOURCE_DIR}/plugins/paintops/libpaintop
    )
    target_link_libraries(FreehandStrokeContractTest kritapixelbrush kritalibpaintop)
endif()

install(TARGETS kritadefaultpaintops  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})
install( FILES
    krita-paintbrush.png
    krita-eraser.png
    krita-duplicate.png
     DESTINATION ${KDE_INSTALL_DATADIR}/krita/images)
