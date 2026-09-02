kis_add_tests(
    TestToolCoreContract.cpp
    LINK_LIBRARIES kritatools Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    DeselectShapesActivationPolicyContractTest.cpp
    LINK_LIBRARIES
        kritatoolsdelegatedtoolpolicyobjects
        kritaflake
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(DeselectShapesActivationPolicyContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/flake
    ${CMAKE_BINARY_DIR}/libs/flake
    ${CMAKE_SOURCE_DIR}/libs/flake/tests
)
target_compile_definitions(DeselectShapesActivationPolicyContractTest PRIVATE
    kritaflake_EXPORTS
    kritatools_EXPORTS
)

kis_add_test(
    KisToolActionContractTest.cpp
    LINK_LIBRARIES
        kritatoolactionmappingobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisToolActionContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_SOURCE_DIR}/libs/tools
    ${CMAKE_BINARY_DIR}/libs/tools
    ${CMAKE_SOURCE_DIR}/libs/flake
    ${CMAKE_BINARY_DIR}/libs/flake
    ${CMAKE_SOURCE_DIR}/libs/flake/resources
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/compositeops
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
)
target_compile_definitions(KisToolActionContractTest PRIVATE
    kritaflake_EXPORTS
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapigment_EXPORTS
    kritaresources_EXPORTS
    kritatools_EXPORTS
)

kis_add_test(
    KisToolSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisToolSchemaContractTest PRIVATE
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_SOURCE_DIR}/libs/tools
    ${CMAKE_BINARY_DIR}/libs/tools
    ${CMAKE_SOURCE_DIR}/libs/flake
    ${CMAKE_BINARY_DIR}/libs/flake
    ${CMAKE_SOURCE_DIR}/libs/flake/resources
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/compositeops
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
)
target_compile_definitions(KisToolSchemaContractTest PRIVATE
    kritaflake_EXPORTS
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapigment_EXPORTS
    kritaresources_EXPORTS
    kritatools_EXPORTS
)

kis_add_test(
    KisSpeedSmootherContractTest.cpp
    LINK_LIBRARIES
        kritatoolsspeedsmootherobjects
        kritaglobalfilteredrollingmeanobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisSpeedSmootherContractTest PRIVATE kritatools_EXPORTS)

kis_add_test(
    KisToolChangesTrackerDataContractTest.cpp
    LINK_LIBRARIES
        kritatoolschangestrackerdataobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisToolChangesTrackerDataContractTest PRIVATE kritatools_EXPORTS)

kis_add_test(
    KisToolChangesTrackerContractTest.cpp
    LINK_LIBRARIES
        kritatoolschangestrackerobjects
        kritatoolschangestrackerdataobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisToolChangesTrackerContractTest PRIVATE kritatools_EXPORTS)

kis_add_test(
    KisRectangleInteractionContractTest.cpp
    LINK_LIBRARIES
        kritatoolsrectangleinteractionobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisRectangleInteractionContractTest PRIVATE kritatools_EXPORTS)

kis_add_test(
    KisSmoothingOptionsRemainderContractTest.cpp
    LINK_LIBRARIES
        kritatoolssmoothingoptionsobjects
        kritaglobalsignalcompressorobjects
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::ConfigCore
)
target_compile_definitions(KisSmoothingOptionsRemainderContractTest PRIVATE
    kritaglobal_EXPORTS
    kritatools_EXPORTS
)

kis_add_tests(
    kis_stabilized_events_sampler_test.cpp
    LINK_LIBRARIES kritatools kritatestsdk
)
