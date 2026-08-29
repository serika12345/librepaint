if(APPLE AND NOT IOS)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(APPKIT_LIBRARY AppKit)
endif()

add_library(kritaapplicationsnapconfigobjects OBJECT
    kis_snap_config.cpp
)
target_compile_definitions(kritaapplicationsnapconfigobjects PRIVATE
    kritaapplication_EXPORTS
    kritaglobal_EXPORTS
    kritapigment_EXPORTS
)
target_include_directories(kritaapplicationsnapconfigobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/compositeops>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/resources>
)
target_link_libraries(kritaapplicationsnapconfigobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
        Boost::boost
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritaapplicationsnapconfigobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritaapplication_LIB_SRCS
    kis_config.cc
    platform-adapters/KisAndroidFileProxy.cpp
    $<TARGET_OBJECTS:kritaapplicationsnapconfigobjects>
)

kis_add_library(kritaapplication SHARED ${kritaapplication_LIB_SRCS})
generate_export_header(kritaapplication BASE_NAME kritaapplication)

target_link_libraries(kritaapplication
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::ConfigCore
        kritaglobal
        kritapigment
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Xml
        kritacanvas
        kritaflake
        kritaresources
)

target_include_directories(kritaapplication
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

if(WIN32)
    set_source_files_properties(
        kis_config.cc
        PROPERTIES
        INCLUDE_DIRECTORIES "${Qt${QT_MAJOR_VERSION}Gui_PRIVATE_INCLUDE_DIRS}"
    )
endif()

set_target_properties(kritaapplication
    PROPERTIES VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritaapplication ${INSTALL_TARGETS_DEFAULT_ARGS})

set(kritaapplicationui_LIB_SRCS
    ui/orchestration/KisActionPlugin.cpp
    ui/orchestration/KisActionPlugin.h
    ui/orchestration/KisApplication.cpp
    ui/orchestration/KisApplication.h
    ui/orchestration/KisApplicationArguments.cpp
    ui/orchestration/KisApplicationArguments.h
    ui/orchestration/KisPart.cpp
    ui/orchestration/KisPart.h
    ui/orchestration/KisPlatformPluginInterfaceFactory.cpp
    ui/orchestration/KisPlatformPluginInterfaceFactory.h
    ui/orchestration/KisQtWidgetsTweaker.cpp
    ui/orchestration/KisQtWidgetsTweaker.h
    ui/orchestration/KisResourceServerProvider.cpp
    ui/orchestration/KisResourceServerProvider.h
    ui/orchestration/kis_action.cpp
    ui/orchestration/kis_action.h
    ui/orchestration/kis_action_manager.cpp
    ui/orchestration/kis_action_manager.h

    ui/workspace/KisAndroidSplash.h
    ui/workspace/KisCanvasWindow.cpp
    ui/workspace/KisCanvasWindow.h
    ui/workspace/KisDetailsPane.cpp
    ui/workspace/KisDetailsPane.h
    ui/workspace/KisMainWindow.cpp
    ui/workspace/KisMainWindow.h
    ui/workspace/KisOpenPane.cpp
    ui/workspace/KisOpenPane.h
    ui/workspace/KisSessionResource.cpp
    ui/workspace/KisSessionResource.h
    ui/workspace/KisTemplate.cpp
    ui/workspace/KisTemplate.h
    ui/workspace/KisTemplateCreateDia.cpp
    ui/workspace/KisTemplateCreateDia.h
    ui/workspace/KisTemplateGroup.cpp
    ui/workspace/KisTemplateGroup.h
    ui/workspace/KisTemplateTree.cpp
    ui/workspace/KisTemplateTree.h
    ui/workspace/KisTemplates.cpp
    ui/workspace/KisTemplates.h
    ui/workspace/KisTemplatesPane.cpp
    ui/workspace/KisTemplatesPane.h
    ui/workspace/KisView.cpp
    ui/workspace/KisView.h
    ui/workspace/KisViewManager.cpp
    ui/workspace/KisViewManager.h
    ui/workspace/KisViewManagerToolActivation.cpp
    ui/workspace/KisView_p.h
    ui/workspace/KisWelcomePageWidget.cpp
    ui/workspace/KisWelcomePageWidget.h
    ui/workspace/KisWindowLayoutManager.cpp
    ui/workspace/KisWindowLayoutManager.h
    ui/workspace/KisWindowLayoutResource.cpp
    ui/workspace/KisWindowLayoutResource.h
    ui/workspace/kis_mainwindow_observer.cpp
    ui/workspace/kis_mainwindow_observer.h
    ui/workspace/kis_preference_set_registry.cpp
    ui/workspace/kis_preference_set_registry.h
    ui/workspace/kis_splash_screen.cpp
    ui/workspace/kis_splash_screen.h
)

ki18n_wrap_ui(kritaapplicationui_LIB_SRCS
    ../ui/forms/KisDetailsPaneBase.ui
    ../ui/forms/KisOpenPaneBase.ui
    ../ui/forms/KisWelcomePage.ui
    ui/workspace/wdgsplash.ui
)

if(ANDROID)
    list(APPEND kritaapplicationui_LIB_SRCS
        ui/workspace/KisAndroidSplash.cpp
    )
endif()

kis_add_library(kritaapplicationui SHARED ${kritaapplicationui_LIB_SRCS})
generate_export_header(kritaapplicationui BASE_NAME kritaui)

target_include_directories(kritaapplicationui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/impex/ui
        ${CMAKE_SOURCE_DIR}/libs/impex/animation
        ${CMAKE_SOURCE_DIR}/libs/ui
        ${CMAKE_SOURCE_DIR}/libs/ui/canvas
        ${CMAKE_SOURCE_DIR}/libs/ui/flake
        ${CMAKE_SOURCE_DIR}/libs/ui/ora
        ${CMAKE_SOURCE_DIR}/libs/ui/tool
        ${CMAKE_SOURCE_DIR}/libs/ui/utils
        ${CMAKE_SOURCE_DIR}/libs/ui/widgets
        ${CMAKE_SOURCE_DIR}/libs/ui/widgets/gradient
        ${CMAKE_SOURCE_DIR}/libs/input/ui/wintab
        ${CMAKE_BINARY_DIR}/libs/ui
)

target_link_libraries(kritaapplicationui
    PRIVATE
        KF${KF_MAJOR}::CoreAddons
        KF${KF_MAJOR}::Completion
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ItemViews
        Qt${QT_MAJOR_VERSION}::Concurrent
        Qt${QT_MAJOR_VERSION}::Network
        Qt${QT_MAJOR_VERSION}::Xml
        Eigen3::Eigen
        Boost::boost
        ${PNG_LIBRARIES}
        kritaapplication
        kritacanvas
        kritadocument
        kritadocumentfiles
        kritadocumentui
        kritaimpex
        kritainput
        kritainputui
        kritaresources
        kritaresourcestorage
        kritaresourceui
        kritatools
        kritatoolsui
        kritaversion
        kritawidgets
        kritawidgetutils
        kritaxmlserialization
)

if (KRITA_USE_SURFACE_COLOR_MANAGEMENT_API)
    target_link_libraries(kritaapplicationui PRIVATE kritasurfacecolormanagementapi)
endif()

if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(kritaapplicationui
        PRIVATE
            Qt6::OpenGLWidgets
            Qt6::SvgWidgets
            KF6::ColorScheme
    )
endif()

if (Mlt7_FOUND)
    target_link_libraries(kritaapplicationui PRIVATE SDL2::SDL2 Mlt7::mlt Mlt7::mlt++)
endif()

if (ANDROID)
    target_link_libraries(kritaapplicationui
        PRIVATE
            Qt${QT_MAJOR_VERSION}::AndroidExtras
    )
endif()

if(APPLE AND NOT IOS)
    target_link_libraries(kritaapplicationui
        PRIVATE
            kritamacosutils
            ${FOUNDATION_LIBRARY}
            ${APPKIT_LIBRARY}
    )
endif()

set_target_properties(kritaapplicationui
    PROPERTIES VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritaapplicationui ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
