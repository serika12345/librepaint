add_library(kritatoolsspeedsmootherobjects OBJECT
    kis_speed_smoother.cpp
)
target_compile_definitions(kritatoolsspeedsmootherobjects PRIVATE kritatools_EXPORTS)
target_include_directories(kritatoolsspeedsmootherobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritatoolsspeedsmootherobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Boost::boost
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritatoolsspeedsmootherobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritatools_LIB_SRCS
    KisSelectionToolFactoryBase.cpp
    KisStabilizerDelayedPaintHelper.cpp
    KisToolPaintFactoryBase.cpp
    KisToolChangesTracker.cpp
    KisToolChangesTrackerData.cpp
    kis_delegated_tool_policies.cpp
    kis_outline_interaction.cpp
    kis_painting_information_builder.cpp
    kis_polyline_interaction.cpp
    kis_rectangle_interaction.cpp
    kis_selection_modifier_mapping.cpp
    kis_smoothing_options.cpp
    kis_stabilized_events_sampler.cpp
    kis_tool.cc
    kis_tool_paint_interaction.cpp
    kis_tool_utils.cpp
)

kis_add_library(kritatools SHARED
    ${kritatools_LIB_SRCS}
    $<TARGET_OBJECTS:kritatoolsspeedsmootherobjects>
)
generate_export_header(kritatools BASE_NAME kritatools)

target_link_libraries(kritatools
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        kritacanvas
        kritaflake
        kritaglobal
        kritaimage
        kritapainting
        kritapigment
        kritaresources
        kritawidgetutils
    PRIVATE
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
)

target_include_directories(kritatools
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

set_target_properties(kritatools
    PROPERTIES VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritatools ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
add_subdirectory(ui)
