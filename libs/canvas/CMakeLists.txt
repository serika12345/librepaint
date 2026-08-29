add_library(kritacanvasinputactiongroupobjects OBJECT
    KisInputActionGroup.cpp
)
target_compile_definitions(kritacanvasinputactiongroupobjects PRIVATE kritacanvas_EXPORTS)
target_include_directories(kritacanvasinputactiongroupobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritacanvasinputactiongroupobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritacanvasinputactiongroupobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritacanvastiledatapoolobjects OBJECT
    tiles/kis_tile_data_pool.h
    tiles/kis_tile_data_pool.cpp
)
target_compile_definitions(kritacanvastiledatapoolobjects PRIVATE kritacanvas_EXPORTS)
target_include_directories(kritacanvastiledatapoolobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritacanvastiledatapoolobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Boost::boost
)
set_target_properties(kritacanvastiledatapoolobjects PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

set(kritacanvas_LIB_SRCS
    animation/kis_animation_frame_cache_index.h
    animation/kis_animation_frame_cache_index.cpp
    animation/kis_frame_data_serializer.h
    animation/kis_frame_data_serializer.cpp
    animation/kis_frame_cache_store.h
    animation/kis_frame_cache_store.cpp
    tiles/kis_tile_data_buffer.h
    color/KisOcioConfiguration.h
    color/KisSurfaceColorSpaceWrapper.h
    color/kis_display_color_filter.h
    color/kis_display_color_transform.h
    color/kis_display_color_transform.cpp
    kis_coordinates_converter.cpp
    kis_image_patch.h
    kis_image_patch.cpp
    kis_prescaled_projection.h
    kis_prescaled_projection.cpp
    kis_projection_backend.h
    kis_projection_backend.cpp
    kis_projection_pixel_filter.h
    kis_projection_update_info.h
    kis_projection_update_info.cpp
    kis_update_info.h
    kis_update_info.cpp
    KisToolCanvas.h
    KisCanvasState.cpp
)

kis_add_library(kritacanvas SHARED
    ${kritacanvas_LIB_SRCS}
    $<TARGET_OBJECTS:kritacanvasinputactiongroupobjects>
    $<TARGET_OBJECTS:kritacanvastiledatapoolobjects>
)
generate_export_header(kritacanvas BASE_NAME kritacanvas)

target_link_libraries(kritacanvas
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        kritaflake
        kritacolor
        kritaimage
)

target_include_directories(kritacanvas
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>
)

set_target_properties(kritacanvas
    PROPERTIES VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritacanvas ${INSTALL_TARGETS_DEFAULT_ARGS})

# Workspace presentation resources follow the canvas and docker state they
# persist, and publish their established symbols from the application UI.
kis_add_library(kritaworkspacepresentation OBJECT
    workspace/kis_workspace_resource.cpp
)
target_compile_definitions(kritaworkspacepresentation PRIVATE kritaapplicationui_EXPORTS)
target_include_directories(kritaworkspacepresentation
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/ui
        ${CMAKE_BINARY_DIR}/libs/application
)
target_link_libraries(kritaworkspacepresentation
    PRIVATE
        kritaimage
        kritaresources
)

add_subdirectory(tests)
