add_subdirectory(tests)

add_library(kritaassistantperspectivehelperobjects OBJECT
    PerspectiveBasedAssistantHelper.cpp
)
target_compile_definitions(kritaassistantperspectivehelperobjects PRIVATE KRITAASSISTANTTOOL_STATIC_DEFINE)
target_include_directories(kritaassistantperspectivehelperobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/ui>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/ui>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/application>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/application>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
)
target_link_libraries(kritaassistantperspectivehelperobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
)
set_target_properties(kritaassistantperspectivehelperobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritaassistanttool_SOURCES
    ConcentricEllipseAssistant.cc
    CurvilinearPerspectiveAssistant.cc
    Ellipse.cc
    EllipseAssistant.cc
    FisheyePointAssistant.cc
    InfiniteRulerAssistant.cc
    kis_assistant_tool.cc
    ParallelRulerAssistant.cc
    PerspectiveAssistant.cc
    Ruler.cc
    RulerAssistant.cc
    SplineAssistant.cc
    VanishingPointAssistant.cc
    EditAssistantsCommand.cpp
    TwoPointAssistant.cc
    PerspectiveEllipseAssistant.cc
)

ki18n_wrap_ui(kritaassistanttool_SOURCES AssistantsToolOptions.ui )

kis_add_library(kritaassistanttool_static STATIC
    ${kritaassistanttool_SOURCES}
    $<TARGET_OBJECTS:kritaassistantperspectivehelperobjects>
)

generate_export_header(kritaassistanttool_static BASE_NAME kritaassistanttool EXPORT_MACRO_NAME KRITAASSISTANTTOOL_EXPORT)

target_link_libraries(kritaassistanttool_static kritaapplicationui kritaflake)

kis_add_library(kritaassistanttool MODULE assistant_tool.cc)
target_link_libraries(kritaassistanttool kritaassistanttool_static)

install(TARGETS kritaassistanttool  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

install( FILES krita_tool_assistant.png dark_krita_tool_assistant.png light_krita_tool_assistant.png DESTINATION ${KDE_INSTALL_DATADIR}/krita/pics)
