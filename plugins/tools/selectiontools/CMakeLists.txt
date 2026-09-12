set(kritaselectiontools_SOURCES
    selection_tools.cc
    kis_tool_select_rectangular.cc
    kis_tool_select_polygonal.cc
    kis_tool_select_elliptical.cc
    kis_tool_select_contiguous.cc
    kis_tool_select_outline.cc
    kis_tool_select_path.cc
    kis_tool_select_similar.cc
    KisMagneticWorker.cc
    KisToolSelectMagnetic.cc
)

qt_add_resources(kritaselectiontools_SOURCES selectiontools.qrc)
krita_ios_add_static_resource(selectiontools)

kis_add_library(kritaselectiontools MODULE ${kritaselectiontools_SOURCES})

generate_export_header(kritaselectiontools BASE_NAME kritaselectiontools)

target_link_libraries(kritaselectiontools
    kritaapplicationui
    kritaflake
    kritatools
    kritabasicflakes
    kritaimage
    kritapainting
)

install(TARGETS kritaselectiontools  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

kis_add_test(
    KisMagneticWorkerSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisMagneticWorkerSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisMagneticWorkerSchemaContractTest PRIVATE
    kritaselectiontools_EXPORTS
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

install( FILES
        KisToolSelectPolygonal.action
        KisToolSelectElliptical.action
        KisToolSelectSimilar.action
        KisToolSelectContiguous.action
        KisToolSelectRectangular.action
        KisToolSelectOutline.action
        KisToolSelectPath.action
        KisToolSelectMagnetic.action
    DESTINATION  ${KDE_INSTALL_DATADIR}/krita/actions
)
