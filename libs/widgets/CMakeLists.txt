add_subdirectory( tests )

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(kritawidgetstitledtabobjects OBJECT
    KoTitledTabWidget.cpp
)
target_compile_definitions(kritawidgetstitledtabobjects PRIVATE kritawidgets_EXPORTS)
target_include_directories(kritawidgetstitledtabobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritawidgetstitledtabobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritawidgetstitledtabobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritawidgetswarningblockobjects OBJECT
    kis_warning_block.cpp
)
target_compile_definitions(kritawidgetswarningblockobjects PRIVATE kritawidgets_EXPORTS)
target_include_directories(kritawidgetswarningblockobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritawidgetswarningblockobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritawidgetswarningblockobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritawidgetsstretchediconbuttonobjects OBJECT
    KisStretchedIconButton.cpp
)
target_compile_definitions(kritawidgetsstretchediconbuttonobjects PRIVATE kritawidgets_EXPORTS)
target_include_directories(kritawidgetsstretchediconbuttonobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritawidgetsstretchediconbuttonobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritawidgetsstretchediconbuttonobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritawidgetsanglegaugeobjects OBJECT
    KisAngleGauge.cpp
)
target_compile_definitions(kritawidgetsanglegaugeobjects PRIVATE kritawidgets_EXPORTS)
target_include_directories(kritawidgetsanglegaugeobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritawidgetsanglegaugeobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritawidgetsanglegaugeobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritawidgetscolorlabelbuttonobjects OBJECT
    kis_color_label_button.cpp
)
target_compile_definitions(kritawidgetscolorlabelbuttonobjects PRIVATE kritawidgets_EXPORTS)
target_include_directories(kritawidgetscolorlabelbuttonobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
)
target_link_libraries(kritawidgetscolorlabelbuttonobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritawidgetscolorlabelbuttonobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritawidgetscolorlabelselectorobjects OBJECT
    kis_color_label_selector_widget.cpp
)
target_compile_definitions(kritawidgetscolorlabelselectorobjects PRIVATE kritawidgets_EXPORTS)
target_include_directories(kritawidgetscolorlabelselectorobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
        ${CMAKE_SOURCE_DIR}/libs/widgetutils
        ${CMAKE_BINARY_DIR}/libs/widgetutils
)
target_link_libraries(kritawidgetscolorlabelselectorobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritawidgetscolorlabelselectorobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritawidgetszoomhandlerobjects OBJECT
    KoZoomHandler.cpp
)
target_compile_definitions(kritawidgetszoomhandlerobjects PRIVATE kritawidgets_EXPORTS)
target_include_directories(kritawidgetszoomhandlerobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/flake>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/flake>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
)
target_link_libraries(kritawidgetszoomhandlerobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Gui
    PRIVATE
        Boost::boost
)
set_target_properties(kritawidgetszoomhandlerobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritawidgetslevelssliderobjects OBJECT
    KisLevelsSlider.cpp
)
target_compile_definitions(kritawidgetslevelssliderobjects PRIVATE kritawidgets_EXPORTS)
target_include_directories(kritawidgetslevelssliderobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritawidgetslevelssliderobjects
    PUBLIC
        kritaglobalpaintingtweaksobjects
        Qt${QT_MAJOR_VERSION}::Widgets
)
set_target_properties(kritawidgetslevelssliderobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(kritawidgets_LIB_SRCS
    KoVBox.cpp
    KoDialog.cpp
    KoZoomWidget.cpp
    KoAspectButton.cpp
    KoSliderCombo.cpp
    KoColorPopupButton.cpp
    KoConfigAuthorPage.cpp
    KoUnitDoubleSpinBox.cpp
    KoZoomAction.cpp
    KoZoomActionState.cpp
    KoZoomInput.cpp
    KoColorPatch.cpp
    KoColorPopupAction.cpp
    KoColorSetWidget.cpp
    KoColorSlider.cpp
    KisHsvColorSlider.cpp
    KoTriangleColorSelector.cpp
    KoResourcePopupAction.cpp
    KoRuler.cpp
    KoResourceServerProvider.cpp
    KoLineStyleSelector.cpp
    KoLineStyleItemDelegate.cpp
    KoLineStyleModel.cpp

    KoDockWidgetTitleBar.cpp
    KoDockWidgetTitleBarButton.cpp
    
    KoToolDocker.cpp
    
    KoMarkerSelector.cpp
    KoMarkerModel.cpp
    KoMarkerItemDelegate.cpp
    KoFillConfigWidget.cpp
    KoStrokeConfigWidget.cpp
    KisStopGradientEditor.cpp
    KisStopGradientSlider.cpp
    KisGradientColorEditor.cpp
    KisGradientWidgetsUtils.cpp

    kis_file_name_requester.cpp

    KisColorSelectorInterface.cpp
    KoAnchorSelectionWidget.cpp

    kis_color_input.cpp

    # classes used by internal color selector
    kis_spinbox_color_selector.cpp
    KisSpinboxHSXSelector.cpp
    KisVisualColorModel.cpp
    KisVisualColorSelector.cpp
    KisVisualColorSelectorShape.cpp
    KisVisualDiamondSelectorShape.cpp
    KisVisualEllipticalSelectorShape.cpp
    KisVisualRectangleSelectorShape.cpp
    KisVisualTriangleSelectorShape.cpp
    KisScreenColorSamplerBase.cpp
    KisDlgInternalColorSelector.cpp

    KisPaletteModel.cpp
    KisPaletteDelegate.cpp
    kis_palette_view.cpp
    KisPaletteChooser.cpp
    KisPaletteComboBox.cpp

    kis_color_button.cpp

    KisAngleSelector.cpp
    kis_aspect_ratio_locker.cpp

    KisTagSelectionWidget.cpp

    KisWidgetConnectionUtils.cpp
    kis_spacing_selection_widget.cpp
    kis_stacked_widget.cpp

)

ki18n_wrap_ui( kritawidgets_LIB_SRCS
    KoConfigAuthorPage.ui
    KoFillConfigWidget.ui
    KoStrokeConfigWidget.ui
    wdgstopgradienteditor.ui
    wdg_file_name_requester.ui
    WdgDlgInternalColorSelector.ui
    WdgPaletteListWidget.ui
)

add_library(kritawidgetsdebugobjects OBJECT
    WidgetsDebug.cpp
)
target_compile_definitions(kritawidgetsdebugobjects PRIVATE kritawidgets_EXPORTS)
target_include_directories(kritawidgetsdebugobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritawidgetsdebugobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritawidgetsdebugobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

kis_add_library(kritawidgets SHARED
    ${kritawidgets_LIB_SRCS}
    $<TARGET_OBJECTS:kritawidgetsdebugobjects>
    $<TARGET_OBJECTS:kritawidgetslevelssliderobjects>
    $<TARGET_OBJECTS:kritawidgetstitledtabobjects>
    $<TARGET_OBJECTS:kritawidgetswarningblockobjects>
    $<TARGET_OBJECTS:kritawidgetsstretchediconbuttonobjects>
    $<TARGET_OBJECTS:kritawidgetsanglegaugeobjects>
    $<TARGET_OBJECTS:kritawidgetscolorlabelbuttonobjects>
    $<TARGET_OBJECTS:kritawidgetscolorlabelselectorobjects>
    $<TARGET_OBJECTS:kritawidgetszoomhandlerobjects>
)
generate_export_header(kritawidgets BASE_NAME kritawidgets)
target_link_libraries(kritawidgets
    PUBLIC
    kritaglobal
    kritaflake
    kritapigment
    kritawidgetutils
    kritaresources
    kritaresourceui
    KF${KF_MAJOR}::CoreAddons
    KF${KF_MAJOR}::ConfigGui
    KF${KF_MAJOR}::GuiAddons
    KF${KF_MAJOR}::WidgetsAddons
    KF${KF_MAJOR}::ConfigCore
    KF${KF_MAJOR}::Completion
)

set_target_properties(kritawidgets PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritawidgets  ${INSTALL_TARGETS_DEFAULT_ARGS})
