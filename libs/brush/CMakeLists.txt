set(kritalibbrush_LIB_SRCS
    kis_predefined_brush_factory.cpp
    kis_auto_brush.cpp
    kis_boundary.cc
    kis_brush.cpp
    kis_scaling_size_brush.cpp
    kis_brush_registry.cpp
    KisBrushServerProvider.cpp
    kis_gbr_brush.cpp
    kis_abr_brush.cpp
    kis_abr_brush_collection.cpp
    kis_imagepipe_brush.cpp
    kis_png_brush.cpp
    kis_svg_brush.cpp
    kis_text_brush.cpp
    kis_auto_brush_factory.cpp
    kis_text_brush_factory.cpp
    KisAbrStorage.cpp
    KisColorfulBrush.cpp
    KisBrushTypeMetaDataFixup.cpp
    KisBrushModel.cpp
)

add_library(kritabrushpipeparasiteobjects OBJECT
    kis_pipebrush_parasite.cpp
)
target_compile_definitions(kritabrushpipeparasiteobjects PRIVATE kritalibbrush_EXPORTS)
target_include_directories(kritabrushpipeparasiteobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image/brushengine>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/psdutils>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/psdutils>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
)
target_link_libraries(kritabrushpipeparasiteobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::CoreAddons
        KF${KF_MAJOR}::I18n
        Imath::Imath
        Boost::boost
)
set_target_properties(kritabrushpipeparasiteobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

kis_add_library(kritalibbrush SHARED
    ${kritalibbrush_LIB_SRCS}
    $<TARGET_OBJECTS:kritabrushpipeparasiteobjects>
)
generate_export_header(kritalibbrush BASE_NAME kritabrush EXPORT_MACRO_NAME BRUSH_EXPORT)

add_library(kritabrushqimagepyramidobjects OBJECT
    kis_qimage_pyramid.cpp
)
target_compile_definitions(kritabrushqimagepyramidobjects PRIVATE kritalibbrush_EXPORTS)
target_include_directories(kritabrushqimagepyramidobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritabrushqimagepyramidobjects PUBLIC Qt${QT_MAJOR_VERSION}::Gui)
set_target_properties(kritabrushqimagepyramidobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)
target_sources(kritalibbrush PRIVATE
    $<TARGET_OBJECTS:kritabrushqimagepyramidobjects>
)

target_link_libraries(kritalibbrush kritaimage Qt${QT_MAJOR_VERSION}::Svg kritamultiarch lager)

set_target_properties(kritalibbrush PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritalibbrush  ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory( tests )
