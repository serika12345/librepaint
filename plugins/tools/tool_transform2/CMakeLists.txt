add_subdirectory(tests)

set(kritatooltransform_static_SRCS
    tool_transform_args.cc
    kis_transform_mask_adapter.cpp
    KisAnimatedTransformMaskParamsHolder.cpp
    kis_tool_transform.cc
    kis_tool_transform_config_widget.cpp
    kis_transform_strategy_base.cpp
    kis_warp_transform_strategy.cpp
    kis_cage_transform_strategy.cpp
    kis_simplified_action_policy_strategy.cpp
    kis_liquify_transform_strategy.cpp
    kis_liquify_paint_helper.cpp
    kis_liquify_paintop.cpp
    kis_mesh_transform_strategy.cpp
    kis_free_transform_strategy.cpp
    kis_free_transform_strategy_gsl_helpers.cpp
    kis_perspective_transform_strategy.cpp
    kis_transform_utils.cpp
    strokes/transform_stroke_strategy.cpp
    strokes/inplace_transform_stroke_strategy.cpp
    )

add_library(kritatooltransformliquifypropertiesobjects OBJECT
    kis_liquify_properties.cpp
)
target_compile_definitions(kritatooltransformliquifypropertiesobjects PRIVATE kritatooltransform_EXPORTS)
target_include_directories(kritatooltransformliquifypropertiesobjects
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
)
target_link_libraries(kritatooltransformliquifypropertiesobjects
    PUBLIC
        kritaglobaldomutilsobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
        Boost::boost
)
set_target_properties(kritatooltransformliquifypropertiesobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

list(APPEND kritatooltransform_static_SRCS
    $<TARGET_OBJECTS:kritatooltransformliquifypropertiesobjects>
)

ki18n_wrap_ui(kritatooltransform_static_SRCS wdg_tool_transform.ui)

kis_add_library(kritatooltransform_static STATIC ${kritatooltransform_static_SRCS})
generate_export_header(kritatooltransform_static BASE_NAME kritatooltransform)

set(kritatooltransform_SOURCES
  tool_transform.cc
)

qt_add_resources(kritatooltransform_SOURCES tool_transform.qrc)
krita_ios_add_static_resource(tool_transform)
kis_add_library(kritatooltransform MODULE ${kritatooltransform_SOURCES})

target_link_libraries(kritatooltransform_static
  PUBLIC
    kritaapplicationui
    kritapainting
    kritatools
)

if (NOT GSL_FOUND)
  message(WARNING "No GNU Scientific Library was found; the Transform Tool cannot scale images with handles. Please install GSL.")
else ()
  target_link_libraries(kritatooltransform_static
    PUBLIC
      GSL::gsl
  )
endif ()

target_link_libraries(kritatooltransform PRIVATE kritatooltransform_static)

install(TARGETS kritatooltransform  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})


install( FILES
    KisToolTransform.action
DESTINATION  ${KDE_INSTALL_DATADIR}/krita/actions)
