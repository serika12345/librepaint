add_library(kritasurfacecolorimetryobjects OBJECT
    ../surfacecolormanagement/KisSurfaceColorimetry.cpp
)
generate_export_header(kritasurfacecolorimetryobjects
    BASE_NAME kritasurfacecolormanagementapi
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/../kritasurfacecolormanagementapi_export.h
)
target_compile_definitions(kritasurfacecolorimetryobjects
    PRIVATE kritasurfacecolormanagementapi_EXPORTS
)
target_include_directories(kritasurfacecolorimetryobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritasurfacecolorimetryobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
)
set_target_properties(kritasurfacecolorimetryobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)
