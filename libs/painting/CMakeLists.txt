add_subdirectory(undo)
add_subdirectory(metadata)

set(kritapainting_LIB_SRCS
    KisAsynchronousStrokeUpdateHelper.cpp
    KisColorSamplerStroke.cpp
    KisStrokeCompatibilityInfo.cpp
    KisStrokeSpeedMonitor.cpp
    kis_figure_painting_stroke.cpp
    kis_resources_snapshot.cpp
    kis_painting_utils.cpp

    strokes/freehand_stroke.cpp
    strokes/KisStrokeEfficiencyMeasurer.cpp
    strokes/kis_painter_based_stroke_strategy.cpp
    strokes/kis_filter_stroke_strategy.cpp
    strokes/kis_color_sampler_stroke_strategy.cpp
    strokes/KisFreehandStrokeInfo.cpp
    strokes/KisMaskedFreehandStrokePainter.cpp
    strokes/KisMaskingBrushRenderer.cpp
    strokes/KisMaskingBrushCompositeOpFactory.cpp
    strokes/move_stroke_strategy.cpp
    strokes/KisNodeSelectionRecipe.cpp
)

kis_add_library(kritapainting SHARED ${kritapainting_LIB_SRCS})
generate_export_header(kritapainting BASE_NAME kritapainting)

target_link_libraries(kritapainting
    PUBLIC
        kritaimage
        kritapaintingundo
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Boost::boost
)

target_include_directories(kritapainting
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/strokes>
)

set_target_properties(kritapainting PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION}
    SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritapainting ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
