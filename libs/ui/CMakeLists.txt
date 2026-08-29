
set(kritauiselectionactionspanel_INCLUDE_DIRS
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/application>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/application>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/canvas>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/canvas>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/flake>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/flake>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/flake/resources>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image/brushengine>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/compositeops>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/resources>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/ui>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/ui>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/ui/canvas>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/ui/flake>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/ui/ora>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/ui/tool>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/ui/utils>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/ui/widgets>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/ui/widgets/gradient>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils/config>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils/xmlgui>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgets>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgets>
)

add_library(kritauiselectionactionspanelobjects OBJECT
    selection/kis_selection_actions_panel.cpp
)
target_compile_definitions(kritauiselectionactionspanelobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauiselectionactionspanelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauiselectionactionspanel_INCLUDE_DIRS}
)
target_link_libraries(kritauiselectionactionspanelobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauiselectionactionspanelobjects PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauiselectionactionspanelsourceobjects OBJECT
    selection/kis_selection_actions_panel_source.cpp
)
target_compile_definitions(kritauiselectionactionspanelsourceobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauiselectionactionspanelsourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauiselectionactionspanel_INCLUDE_DIRS}
)
target_link_libraries(kritauiselectionactionspanelsourceobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::ConfigGui
        KF${KF_MAJOR}::Completion
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::WidgetsAddons
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauiselectionactionspanelsourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritauimultinodeproperty_INCLUDE_DIRS
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/compositeops>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/resources>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/psdutils>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/psdutils>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/ui>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/ui>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils/config>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils/xmlgui>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgets>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgets>
)

add_library(kritauimultinodepropertyobjects OBJECT
    nodes/kis_multinode_property.cpp
)
target_compile_definitions(kritauimultinodepropertyobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauimultinodepropertyobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauimultinodeproperty_INCLUDE_DIRS}
)
target_link_libraries(kritauimultinodepropertyobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauimultinodepropertyobjects PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauimultinodepropertysourceobjects OBJECT
    nodes/kis_multinode_property_source.cpp
)
target_compile_definitions(kritauimultinodepropertysourceobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauimultinodepropertysourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauimultinodeproperty_INCLUDE_DIRS}
)
target_link_libraries(kritauimultinodepropertysourceobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauimultinodepropertysourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritauinodemanager_INCLUDE_DIRS
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image/brushengine>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image/filter>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/compositeops>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/resources>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/psdutils>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/psdutils>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/ui>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/ui>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils/config>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils/xmlgui>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgets>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgets>
)

add_library(kritauinodemanagerimagestateobjects OBJECT
    nodes/KisNodeManagerImageState.cpp
)
target_compile_definitions(kritauinodemanagerimagestateobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagerimagestateobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagerimagestateobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagerimagestateobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagerimagestatesourceobjects OBJECT
    nodes/KisNodeManagerImageStateSource.cpp
)
target_compile_definitions(kritauinodemanagerimagestatesourceobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagerimagestatesourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagerimagestatesourceobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagerimagestatesourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagernodestateobjects OBJECT
    nodes/KisNodeManagerNodeState.cpp
)
target_compile_definitions(kritauinodemanagernodestateobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagernodestateobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagernodestateobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagernodestateobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagernodestatesourceobjects OBJECT
    nodes/KisNodeManagerNodeStateSource.cpp
)
target_compile_definitions(kritauinodemanagernodestatesourceobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagernodestatesourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagernodestatesourceobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagernodestatesourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagernodechangeobjects OBJECT
    nodes/KisNodeManagerNodeChange.cpp
)
target_compile_definitions(kritauinodemanagernodechangeobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagernodechangeobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagernodechangeobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagernodechangeobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagermodificationobjects OBJECT
    nodes/KisNodeManagerModification.cpp
)
target_compile_definitions(kritauinodemanagermodificationobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagermodificationobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagermodificationobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagermodificationobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanageractiveobjects OBJECT
    nodes/KisNodeManagerActive.cpp
)
target_compile_definitions(kritauinodemanageractiveobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanageractiveobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanageractiveobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanageractiveobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanageraccessorobjects OBJECT
    nodes/KisNodeManagerAccessors.cpp
)
target_compile_definitions(kritauinodemanageraccessorobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanageraccessorobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanageraccessorobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanageraccessorobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagerselectionstateobjects OBJECT
    nodes/KisNodeManagerSelectionState.cpp
)
target_compile_definitions(kritauinodemanagerselectionstateobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagerselectionstateobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagerselectionstateobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagerselectionstateobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagerpropertiesobjects OBJECT
    nodes/KisNodeManagerProperties.cpp
)
target_compile_definitions(kritauinodemanagerpropertiesobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagerpropertiesobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagerpropertiesobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagerpropertiesobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagerpropertydialogobjects OBJECT
    nodes/KisNodeManagerPropertyDialog.cpp
)
target_compile_definitions(kritauinodemanagerpropertydialogobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagerpropertydialogobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagerpropertydialogobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagerpropertydialogobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagernodeupdateobjects OBJECT
    nodes/KisNodeManagerNodeUpdate.cpp
)
target_compile_definitions(kritauinodemanagernodeupdateobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagernodeupdateobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagernodeupdateobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagernodeupdateobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanageravigationobjects OBJECT
    nodes/KisNodeManagerNavigation.cpp
)
target_compile_definitions(kritauinodemanageravigationobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanageravigationobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanageravigationobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanageravigationobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagerorderingobjects OBJECT
    nodes/KisNodeManagerOrdering.cpp
)
target_compile_definitions(kritauinodemanagerorderingobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagerorderingobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagerorderingobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagerorderingobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagerremovalobjects OBJECT
    nodes/KisNodeManagerRemoval.cpp
)
target_compile_definitions(kritauinodemanagerremovalobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagerremovalobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagerremovalobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagerremovalobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagertreeoperationobjects OBJECT
    nodes/KisNodeManagerTreeOperations.cpp
)
target_compile_definitions(kritauinodemanagertreeoperationobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagertreeoperationobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagertreeoperationobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagertreeoperationobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagerisolationobjects OBJECT
    nodes/KisNodeManagerIsolation.cpp
)
target_compile_definitions(kritauinodemanagerisolationobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagerisolationobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagerisolationobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagerisolationobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagerselectionobjects OBJECT
    nodes/KisNodeManagerSelection.cpp
)
target_compile_definitions(kritauinodemanagerselectionobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagerselectionobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagerselectionobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagerselectionobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagertoggleobjects OBJECT
    nodes/KisNodeManagerToggle.cpp
)
target_compile_definitions(kritauinodemanagertoggleobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagertoggleobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
)
target_link_libraries(kritauinodemanagertoggleobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagertoggleobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritauinodemanagermirrorobjects OBJECT
    nodes/KisNodeManagerMirror.cpp
)
target_compile_definitions(kritauinodemanagermirrorobjects PRIVATE
    kritaapplicationui_EXPORTS
)
target_include_directories(kritauinodemanagermirrorobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritauinodemanager_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/libs/painting/undo
        ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_link_libraries(kritauinodemanagermirrorobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritauinodemanagermirrorobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

if (ANDROID)
    # We explicitly set QT_OPENGL_ES_3 here because Qt for some reason does
    # not enable this for Android.
    # See also: https://bugreports.qt.io/browse/QTBUG-64703
    # This is not needed for ARM GNU/Linux environments.
    add_definitions(-DQT_OPENGL_ES_3)
    target_compile_definitions(kritaapplicationui PRIVATE QT_OPENGL_ES_3)
    if (QT_MAJOR_VERSION STREQUAL "5")
        include_directories (${Qt5AndroidExtras_INCLUDE_DIRS})
        target_include_directories(kritaapplicationui PRIVATE ${Qt5AndroidExtras_INCLUDE_DIRS})
    endif()
endif()

add_subdirectory( tests )

set(kritaui_LIB_SRCS
    $<TARGET_OBJECTS:kritauiselectionactionspanelobjects>
    $<TARGET_OBJECTS:kritauiselectionactionspanelsourceobjects>
    $<TARGET_OBJECTS:kritauimultinodepropertyobjects>
    $<TARGET_OBJECTS:kritauimultinodepropertysourceobjects>
    $<TARGET_OBJECTS:kritauinodemanagerimagestateobjects>
    $<TARGET_OBJECTS:kritauinodemanagerimagestatesourceobjects>
    $<TARGET_OBJECTS:kritauinodemanagernodestateobjects>
    $<TARGET_OBJECTS:kritauinodemanagernodestatesourceobjects>
    $<TARGET_OBJECTS:kritauinodemanagernodechangeobjects>
    $<TARGET_OBJECTS:kritauinodemanagermodificationobjects>
    $<TARGET_OBJECTS:kritauinodemanageractiveobjects>
    $<TARGET_OBJECTS:kritauinodemanageraccessorobjects>
    $<TARGET_OBJECTS:kritauinodemanagerselectionstateobjects>
    $<TARGET_OBJECTS:kritauinodemanagerpropertiesobjects>
    $<TARGET_OBJECTS:kritauinodemanagerpropertydialogobjects>
    $<TARGET_OBJECTS:kritauinodemanagernodeupdateobjects>
    $<TARGET_OBJECTS:kritauinodemanageravigationobjects>
    $<TARGET_OBJECTS:kritauinodemanagerorderingobjects>
    $<TARGET_OBJECTS:kritauinodemanagerremovalobjects>
    $<TARGET_OBJECTS:kritauinodemanagertreeoperationobjects>
    $<TARGET_OBJECTS:kritauinodemanagerisolationobjects>
    $<TARGET_OBJECTS:kritauinodemanagerselectionobjects>
    $<TARGET_OBJECTS:kritauinodemanagertoggleobjects>
    $<TARGET_OBJECTS:kritauinodemanagermirrorobjects>

    canvas/kis_canvas_widget_base.cpp
    canvas/kis_canvas2.cpp
    canvas/kis_canvas_tool_support.cpp
    canvas/kis_canvas_updates_compressor.cpp
    canvas/kis_canvas_controller.cpp
    canvas/kis_display_color_converter.cpp
    canvas/kis_display_filter.cpp
    canvas/kis_exposure_gamma_correction_interface.cpp
    canvas/kis_canvas_decoration.cc
    canvas/kis_grid_manager.cpp
    canvas/kis_grid_decoration.cpp
    canvas/kis_grid_config.cpp
    canvas/kis_qpainter_canvas.cpp
    canvas/kis_qpainter_projection_factory.cpp
    canvas/kis_image_pyramid.cpp
    canvas/kis_infinity_manager.cpp
    canvas/kis_change_guides_command.cpp
    canvas/kis_guides_decoration.cpp
    canvas/kis_guides_manager.cpp
    canvas/kis_guides_config.cpp
    canvas/kis_snap_line_strategy.cpp
    canvas/KisSnapPointStrategy.cpp
    canvas/KisSnapPixelStrategy.cpp
    canvas/KisMirrorAxisConfig.cpp
    canvas/KoCanvasControllerWidget.cpp
    canvas/KoCanvasControllerWidgetViewport_p.cpp

    dialogs/kis_about_application.cpp
    dialogs/kis_dlg_adj_layer_props.cc
    dialogs/kis_dlg_adjustment_layer.cc
    dialogs/kis_dlg_filter.cpp
    dialogs/kis_dlg_generator_layer.cpp
    dialogs/kis_dlg_file_layer.cpp
    dialogs/kis_dlg_filter.cpp
    dialogs/kis_dlg_stroke_selection_properties.cpp
    dialogs/kis_dlg_image_properties.cc
    dialogs/kis_dlg_layer_properties.cc
    dialogs/kis_dlg_preferences.cc
    dialogs/KisDlgPreferencesNotifications.cpp
    dialogs/kis_dlg_preferences_input.cpp
    dialogs/slider_and_spin_box_sync.cpp
    dialogs/kis_dlg_layer_style.cpp
    dialogs/kis_dlg_import_image_sequence.cpp
    dialogs/kis_delayed_save_dialog.cpp
    dialogs/KisSessionManagerDialog.cpp
    dialogs/KisNewWindowLayoutDialog.cpp
    dialogs/KisDlgChangeCloneSource.cpp
    dialogs/KisDlgSavePreset.cpp
    dialogs/kis_dlg_hlg_import.cpp
    dialogs/KisDlgConfigureCumulativeUndo.cpp
    dialogs/KisCumulativeUndoModel.cpp
    dialogs/KisColorSpaceConversionDialog.cpp
    dialogs/KisProofingConfigModel.cpp
    dialogs/KisFrameRateLimitModel.cpp
    dialogs/KisDlgCreateNewDocument.cpp

    flake/kis_node_dummies_graph.cpp
    flake/kis_dummies_facade_base.cpp
    flake/kis_dummies_facade.cpp
    flake/kis_node_shapes_graph.cpp
    flake/kis_node_shape.cpp
    flake/kis_shape_controller.cpp
    flake/kis_shape_layer.cc
    flake/kis_shape_layer_canvas.cpp
    flake/kis_shape_selection.cpp
    flake/kis_shape_selection_canvas.cpp
    flake/kis_shape_selection_model.cpp
    flake/kis_take_all_shapes_command.cpp

    tool/kis_bookmarked_configurations_editor.cc
    tool/kis_bookmarked_configurations_model.cc
    tool/kis_bookmarked_filter_configurations_model.cc
    animation/KisPlaybackEngine.cpp
    animation/KisPlaybackEngineQT.cpp
    resources/KisPresetShadowUpdater.cpp
    canvas/kis_canvas_resource_provider.cpp
    resources/kis_derived_resources.cpp
    resources/kis_control_frame.cpp
    canvas/KisDisplayConfig.cpp
    resources/kis_custom_pattern.cc
    document/kis_file_layer.cpp
    document/kis_change_file_layer_command.h
    document/kis_filter_manager.cc
    resources/kis_filters_model.cc
    document/kis_image_manager.cc
    canvas/kis_image_view_converter.cpp
    nodes/kis_layer_manager.cc
    nodes/kis_mask_manager.cc
    nodes/kis_node_manager.cpp
    nodes/kis_node_selection_adapter.cpp
    nodes/kis_node_insertion_adapter.cpp
    canvas/KisNodeDisplayModeAdapter.cpp
    nodes/kis_node_model.cpp
    nodes/kis_node_filter_proxy_model.cpp
    nodes/kis_model_index_converter_base.cpp
    nodes/kis_model_index_converter.cpp
    nodes/kis_model_index_converter_show_all.cpp
    canvas/KisMultiSurfaceStateManager.cpp
    canvas/KisLayerThumbnailCache.cpp
    canvas/kis_painting_assistant.cc
    canvas/kis_painting_assistants_decoration.cpp
    selection/kis_selection_actions_panel_handle.cpp
    selection/kis_selection_actions_panel_button.cpp
    canvas/KisDecorationsManager.cpp
    tool/kis_paintop_box.cc
    resources/kis_popup_palette.cpp
    flake/KisSelectedShapesProxy.cpp
    canvas/kis_selection_decoration.cc
    selection/kis_selection_manager.cc
    selection/KisSelectionActionsAdapter.cpp
    canvas/kis_statusbar.cc
    canvas/kis_zoom_manager.cc
    resources/kis_favorite_resource_manager.cpp
    canvas/kis_canvas_controls_manager.cpp
    theme/KisUiFont.cpp
    canvas/KisIdleTasksManager.cpp
    canvas/KisIdleTaskStrokeStrategy.cpp
    canvas/KisImageThumbnailStrokeStrategy.cpp
    document/KisTextPropertiesManager.cpp
    events/KisLongPressEventFilter.cpp

    opengl/kis_opengl.cpp
    opengl/kis_opengl_canvas2.cpp
    opengl/kis_opengl_canvas_debugger.cpp
    opengl/kis_opengl_image_textures.cpp
    opengl/kis_texture_tile.cpp
    opengl/kis_opengl_shader_loader.cpp
    opengl/kis_opengl_update_info.cpp
    opengl/KisOpenGLUpdateInfoBuilder.cpp
    opengl/KisOpenGLModeProber.cpp
    opengl/KisScreenInformationAdapter.cpp
    opengl/KisOpenGLBufferCircularStorage.cpp
    opengl/KisOpenGLSync.cpp
    opengl/KisOpenGLBufferCreationGuard.cpp
    opengl/KisOpenGLCanvasRenderer.cpp
    opengl/KisOpenGLContextSwitchLock.cpp
    canvas/kis_fps_decoration.cpp

    tool/kis_selection_tool_helper.cpp
    tool/kis_tool_freehand.cc
    tool/kis_painting_information_builder_adapters.cpp
    tool/kis_tool_freehand_helper.cpp
    tool/kis_scratch_pad.cpp
    tool/kis_tool_multihand_helper.cpp
    tool/kis_tool_paint.cc
    tool/kis_tool_shape.cc
    tool/kis_tool_ellipse_base.cpp
    tool/kis_tool_rectangle_base.cpp
    tool/kis_tool_polyline_base.cpp
    tool/KisToolOutlineBase.cpp
    tool/kis_tool_canvas_utils.cpp
    tool/KisAsyncColorSamplerHelper.cpp

    toolbox/KoToolBox.cpp
    toolbox/KoToolBoxButton.cpp
    toolbox/KoToolBoxDocker.cpp
    toolbox/KoToolBoxFactory.cpp

    widgets/kis_canvas_drop.cpp
    widgets/KisCompositeOpListConnectionHelper.cpp
    widgets/kis_cmb_contour.cpp
    widgets/kis_cmb_gradient.cpp
    widgets/kis_collapsible_button_group.cpp
    widgets/kis_cmb_idlist.cc
    widgets/kis_color_space_selector.cc
    widgets/kis_advanced_color_space_selector.cc
    widgets/kis_cie_tongue_widget.cpp
    widgets/kis_tone_curve_widget.cpp
    widgets/kis_transport_controls.cpp
    widgets/kis_utility_title_bar.cpp
    widgets/kis_utility_title_bar_p.h
    widgets/kis_curve_widget.cpp
    widgets/KisCurveWidgetConnectionHelper.cpp
    widgets/KisCurveWidgetControlsManager.cpp
    widgets/kis_custom_image_widget.cc
    widgets/kis_image_from_clipboard_widget.cpp
    widgets/kis_filter_selector_widget.cc
    widgets/kis_iconwidget.cc
    widgets/kis_mask_widgets.cpp
    widgets/kis_meta_data_merge_strategy_chooser_widget.cc
    widgets/kis_multi_bool_filter_widget.cc
    widgets/kis_multi_double_filter_widget.cc
    widgets/kis_multi_integer_filter_widget.cc
    widgets/kis_paintop_presets_editor.cpp
    widgets/kis_paintop_presets_chooser_popup.cpp
    widgets/kis_paintop_preset_icon_library.cpp
    widgets/kis_pattern_chooser.cc

    widgets/kis_preset_chooser.cpp
    widgets/kis_progress_widget.cpp
    widgets/kis_scratch_pad_event_filter.cpp
    widgets/KisSelectionPropertySlider.cpp
    widgets/kis_size_group.cpp
    widgets/kis_size_group_p.cpp
    widgets/kis_wdg_generator.cpp
    widgets/kis_workspace_chooser.cpp
    widgets/kis_widget_chooser.cpp
    widgets/kis_tool_button.cpp
    widgets/kis_floating_message.cpp
    widgets/kis_lod_availability_widget.cpp
    widgets/KisLodAvailabilityData.cpp
    widgets/KisLodAvailabilityModel.cpp
    widgets/kis_color_filter_combo.cpp
    widgets/kis_layer_filter_widget.cpp
    widgets/kis_elided_label.cpp
    widgets/kis_preset_live_preview_view.cpp
    widgets/KisScreenColorSampler.cpp
    widgets/KoDualColorButton.cpp
    widgets/KoForegroundColour.cpp
    widgets/KoBackgroundColour.cpp
    widgets/KoSwapBgFgColours.cpp
    widgets/KoResetBgFgColours.cpp
    widgets/KisLayerStyleAngleSelector.cpp
    widgets/KisMemoryReportButton.cpp
    widgets/KisDitherWidget.cpp
    widgets/KisWarningWidget.cpp
    widgets/KisAutoLevelsWidget.cpp
    widgets/KisHistogramPainter.cpp
    widgets/KisHistogramView.cpp
    widgets/kis_zoom_scrollbar.cpp
    widgets/KisDlgListPicker.cpp
    widgets/kis_round_hud_button.cpp
    widgets/KisDockerHud.cpp
    widgets/KisProofingOptionsWidget.cpp

    widgets/KisGrabKeyboardFocusRecoveryWorkaround.cpp

    # Gradient related widgets
    widgets/gradient/KisSegmentGradientEditor.cpp
    widgets/gradient/KisSegmentGradientSlider.cpp
    widgets/gradient/KisGradientChooser.cpp
    widgets/gradient/KisGenericGradientEditor.cpp

    dialogs/KisDlgPaletteEditor.cpp

    widgets/KisGamutMaskToolbar.cpp
	widgets/KisColorSamplerPreviewPreview.cpp

    utils/kis_document_aware_spin_box_unit_manager.cpp
    utils/KisSpinBoxSplineUnitConverter.cpp
    utils/KisDitherUtil.cpp
    utils/KisFileIconCreator.cpp
    utils/KisRecentDocumentsModelWrapper.cpp
    utils/KisRecentFileIconCache.cpp

    operations/kis_operation.cpp
    operations/kis_operation_configuration.cpp
    operations/kis_operation_registry.cpp
    operations/kis_operation_ui_factory.cpp
    operations/kis_operation_ui_widget.cpp
    operations/kis_filter_selection_operation.cpp
    actions/kis_selection_action_factories.cpp
    actions/KisPasteActionFactories.cpp
    actions/KisTransformToolActivationCommand.cpp
    actions/KisLayerSelectionAction.cpp
    actions/input/KisApplicationInputActions.cpp
    actions/input/KisCanvasOnlyAction.cpp
    actions/input/KisTouchGestureAction.cpp
    actions/input/kis_change_frame_action.cpp
    actions/input/kis_gamma_exposure_action.cpp
    actions/input/kis_pan_action.cpp
    actions/input/kis_rotate_canvas_action.cpp
    actions/input/kis_select_layer_action.cpp
    actions/input/kis_zoom_action.cpp
    actions/input/kis_zoom_and_rotate_action.cpp
    animation/KisFrameDisplayProxy.cpp
    events/kis_gui_context_command.cpp
    events/kis_gui_context_command_p.cpp
    processing/fill_processing_visitor.cpp
    processing/KisEncloseAndFillProcessingVisitor.cpp
    canvas/kis_mirror_axis.cpp
    canvas/KisCanvasImageState.cpp
    canvas/KisCanvasColorDrop.cpp

    imagesize/imagesize.cc
    imagesize/dlg_canvassize.cc
    imagesize/dlg_imagesize.cc
    imagesize/wdg_imagesize.cc
    imagesize/dlg_layersize.cc

    canvas/kcanvaspreview.cc
    canvas/kis_abstract_perspective_grid.cpp

    document/KisDocument.cpp
    document/KisDocumentImageState.cpp
    document/KisImageManagerDrop.cpp
    document/KisDocumentTemplate.cpp
    impex/KisImportExportColorSpaceDialog.cpp
    impex/KisImportExportResizeWidget.cpp
    document/KisCloneDocumentStroke.cpp
    nodes/kis_node_view_color_scheme.cpp
    events/kis_async_action_feedback.cpp
    actions/KisUndoActionsUpdateManager.cpp

    theme/thememanager.cpp

    canvas/kis_mirror_manager.cpp

    qtsingleapplication/qtlocalpeer.cpp
    qtsingleapplication/qtsingleapplication.cpp


    canvas/KisReferenceImagesDecoration.cpp
    canvas/KisReferenceImage.cpp
    flake/KisReferenceImagesLayer.cpp
    flake/KisReferenceImagesLayer.h
    events/KisMouseClickEater.cpp

    canvas/KisDecorationsWrapperLayer.cpp

    document/StoryboardItem.cpp

    nodes/KisNodeActivationActionCreatorVisitor.cpp
)

if (KSeExpr_FOUND)
    set(kritaui_LIB_SRCS ${kritaui_LIB_SRCS}
        widgets/kis_seexpr_script_chooser.cpp
    )
endif()

if (Mlt7_FOUND)
    set(kritaui_LIB_SRCS ${kritaui_LIB_SRCS}
        animation/KisPlaybackEngineMLT.cpp
        animation/KisMLTProducerKrita.cpp
    )
endif()

if(WIN32)
    set(kritaui_LIB_SRCS
        ${kritaui_LIB_SRCS}
        dialogs/KisDlgCustomTabletResolution.cpp
        )
elseif(ANDROID)
    if(KRITA_QT_HAS_ANDROID_QPLATFORMSCREEN_DENSITY_ADJUSTMENT)
        list(APPEND kritaui_LIB_SRCS
            canvas/KisAndroidScaling.cpp
        )
    endif()
endif()

    set(kritaui_LIB_SRCS
        ${kritaui_LIB_SRCS}
        animation/kis_animation_frame_cache.cpp
        animation/kis_animation_cache_populator.cpp
        animation/KisAsyncAnimationRendererBase.cpp
        animation/KisAsyncAnimationCacheRenderer.cpp
        animation/KisAsyncAnimationFramesSavingRenderer.cpp
        dialogs/KisAsyncAnimationRenderDialogBase.cpp
        dialogs/KisAsyncAnimationCacheRenderDialog.cpp
        dialogs/KisAsyncAnimationFramesSaveDialog.cpp
        canvas/KisCanvasAnimationState.cpp	
        animation/cache/KisFrameCacheSwapper.cpp
        animation/cache/KisAbstractFrameCacheSwapper.cpp
        animation/cache/KisInMemoryFrameCacheSwapper.cpp

    )

if(APPLE AND NOT IOS)
    set(kritaui_LIB_SRCS
        ${kritaui_LIB_SRCS}
        platform/osx.mm
    )
endif()

ki18n_wrap_ui(kritaui_UI_HEADERS
    widgets/KisDitherWidget.ui
    widgets/KisAutoLevelsWidget.ui

    forms/wdgaboutapplication.ui
    forms/wdgfullscreensettings.ui
    forms/wdgsegmentgradienteditor.ui
    forms/wdggeneralsettings.ui
    forms/wdgperformancesettings.ui
    forms/wdggenerators.ui
    forms/wdgbookmarkedconfigurationseditor.ui
    forms/wdgapplyprofile.ui
    forms/wdgcustompattern.ui
    forms/wdglayerproperties.ui
    forms/wdgcolorsettings.ui
    forms/wdgtabletsettings.ui
    forms/wdgcolorspaceselector.ui
    forms/wdgcolorspaceselectoradvanced.ui
    forms/wdgdisplaysettings.ui
    forms/wdgproofingoptions.ui
    forms/kis_matrix_widget.ui
    forms/wdggeometryoptions.ui
    forms/wdgnewimage.ui
    forms/wdgimageproperties.ui
    forms/wdgmaskfromselection.ui
    forms/wdgmasksource.ui
    forms/wdgfilterdialog.ui
    forms/wdgmetadatamergestrategychooser.ui
    forms/wdgpaintoppresets.ui
    forms/wdgpaintopsettings.ui
    forms/wdgdlggeneratorlayer.ui
    forms/wdgdlgfilelayer.ui
    forms/wdgfilterselector.ui
    forms/wdgfilternodecreation.ui
    forms/wdgnodequerypatheditor.ui
    forms/wdgpresetselectorstrip.ui
    forms/wdgsavebrushpreset.ui
    forms/wdgpreseticonlibrary.ui
    forms/wdgimportimagesequence.ui
    forms/wdgstrokeselectionproperties.ui
    forms/wdgsessionmanager.ui
    forms/wdgnewwindowlayout.ui
    forms/WdgDlgPaletteEditor.ui
    forms/wdgGamutMaskToolbar.ui
    forms/wdgchangeclonesource.ui
    forms/WdgPopupPaletteSettings.ui
    forms/wdgconvertcolorspace.ui


    widgets/KisDlgListPicker.ui
    dialogs/kis_delayed_save_dialog.ui
    dialogs/kis_dlg_hlg_import.ui

    layerstyles/wdgBevelAndEmboss.ui
    layerstyles/wdgblendingoptions.ui
    layerstyles/WdgColorOverlay.ui
    layerstyles/wdgContour.ui
    layerstyles/wdgdropshadow.ui
    layerstyles/WdgGradientOverlay.ui
    layerstyles/wdgInnerGlow.ui
    layerstyles/wdglayerstyles.ui
    layerstyles/WdgPatternOverlay.ui
    layerstyles/WdgSatin.ui
    layerstyles/WdgStroke.ui
    layerstyles/wdgstylesselector.ui
    layerstyles/wdgTexture.ui
    layerstyles/wdgKisLayerStyleAngleSelector.ui

    imagesize/wdg_imagesize.ui
    imagesize/wdg_layersize.ui
    imagesize/wdg_canvassize.ui
)

if(WIN32)
    ki18n_wrap_ui(kritaui_UI_HEADERS
        dialogs/KisDlgCustomTabletResolution.ui
    )
endif()

if (KRITA_USE_SURFACE_COLOR_MANAGEMENT_API)
    set(kritaui_LIB_SRCS
        ${kritaui_LIB_SRCS}

        canvas/KisSRGBSurfaceColorSpaceManager.cpp
        canvas/KisCanvasSurfaceColorSpaceManager.cpp
        canvas/KisRootSurfaceInfoProxy.cpp
    )
endif()

set_source_files_properties(
    ${kritaui_UI_HEADERS}
    TARGET_DIRECTORY kritaapplicationui
    PROPERTIES GENERATED TRUE
)
add_custom_target(
    kritaapplicationui_generated_ui_headers
    DEPENDS ${kritaui_UI_HEADERS}
)
add_dependencies(
    kritaapplicationui
    kritaapplicationui_generated_ui_headers
)

target_sources(
    kritaapplicationui
    PRIVATE
        ${kritaui_UI_HEADERS}
        ${kritaui_HEADERS_MOC}
        ${kritaui_LIB_SRCS}
)

if(IOS)
    # Qt 6 exposes the GLES backend through QT_CONFIG(opengles2), while this
    # renderer still selects its established GLES path with the legacy macro.
    set_source_files_properties(
        opengl/kis_opengl.cpp
        opengl/kis_opengl_image_textures.cpp
        opengl/kis_texture_tile.cpp
        TARGET_DIRECTORY kritaapplicationui
        PROPERTIES COMPILE_DEFINITIONS QT_OPENGL_ES_2
    )
endif()

target_sources(kritaapplicationui
  PRIVATE
    resources/kis_abstract_resources.h
    resources/kis_abstract_resources.cpp
)

target_link_libraries(kritaapplicationui
    PUBLIC
        KF${KF_MAJOR}::CoreAddons
        KF${KF_MAJOR}::Completion
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ItemViews

        Qt${QT_MAJOR_VERSION}::Network
        Qt${QT_MAJOR_VERSION}::Concurrent

        Eigen3::Eigen
        Boost::boost
        ${PNG_LIBRARIES}

        kritaversion
        kritaimpex
        kritawidgets
        kritawidgetutils
        kritaresources
        kritaresourcestorage
        kritatools
        kritatoolsui
        kritacanvas
        kritadocument
        kritainput
        kritaapplication
    PRIVATE
        kritadocumentfiles
        kritadocumentui
        kritaimpexui
        kritainputui
        kritaworkspacepresentation
        kritaresourceui
        kritaxmlserialization
)

if (KRITA_USE_SURFACE_COLOR_MANAGEMENT_API)
    target_link_libraries(kritaapplicationui
        PRIVATE
            kritasurfacecolormanagementapi
    )
endif()

if(WIN32)
    target_link_libraries(kritaapplicationui PRIVATE dxgi)

    # Private headers are needed for:
    #    * KisDlgCustomTabletResolution
    #    * KisScreenInformationAdapter
    #    * dialogs/kis_dlg_preferences.cc
    set_source_files_properties(
        dialogs/KisDlgCustomTabletResolution.cpp
        opengl/KisScreenInformationAdapter.cpp
        dialogs/kis_dlg_preferences.cc # for WinTab switching API
        TARGET_DIRECTORY kritaapplicationui
        PROPERTIES
        INCLUDE_DIRECTORIES "${Qt${QT_MAJOR_VERSION}Gui_PRIVATE_INCLUDE_DIRS}"
        )
elseif(ANDROID)
    # Private headers are required to access QPlatformScreen::densityAdjustment
    # and QPlatformScreen::setDensityAdjustment. These are patched into our Qt
    # to allow us to dynamically change the UI scale on Android.
    if(KRITA_QT_HAS_ANDROID_QPLATFORMSCREEN_DENSITY_ADJUSTMENT)
        set_source_files_properties(
            canvas/KisAndroidScaling.cpp
            TARGET_DIRECTORY kritaapplicationui
            PROPERTIES
            INCLUDE_DIRECTORIES "${Qt${QT_MAJOR_VERSION}Gui_PRIVATE_INCLUDE_DIRS}"
        )
    endif()
endif()

if (HAVE_X11)
    set_source_files_properties(
        opengl/KisOpenGLModeProber.cpp
        TARGET_DIRECTORY kritaapplicationui
        PROPERTIES
        INCLUDE_DIRECTORIES "${Qt${QT_MAJOR_VERSION}Gui_PRIVATE_INCLUDE_DIRS}"
        )
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
    target_link_libraries(kritaapplicationui PRIVATE GLESv3)
    target_link_libraries(kritaapplicationui PUBLIC Qt${QT_MAJOR_VERSION}::Gui)
    target_link_libraries(kritaapplicationui PRIVATE Qt${QT_MAJOR_VERSION}::AndroidExtras)
    if(TARGET LibAV)
        target_link_libraries(kritaapplicationui PRIVATE LibAV)
    endif()
endif()

if (HAIKU)
  target_link_libraries(kritaapplicationui PRIVATE network expat iconv intl)
endif()

if(APPLE AND NOT IOS)
    target_link_libraries(kritaapplicationui
        PUBLIC
        kritamacosutils

        PRIVATE
        ${FOUNDATION_LIBRARY}
        ${APPKIT_LIBRARY}
    )
endif ()

if(OpenEXR_FOUND)
  target_link_libraries(kritaapplicationui PUBLIC ${LINK_OPENEXR_LIB})
endif()

target_include_directories(kritaapplicationui
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/impex/ui>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/impex/animation>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/canvas>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/flake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ora>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tool>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/widgets>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/widgets/gradient>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/input/ui/wintab>
)

if (APPLE AND NOT IOS)
    install(FILES platform/osx.stylesheet DESTINATION ${KDE_INSTALL_DATADIR}/krita)
endif ()
