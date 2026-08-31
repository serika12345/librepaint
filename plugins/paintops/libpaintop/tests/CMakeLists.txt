########### next target ###############


kis_add_tests(KisCurveOptionDataTest.cpp
    KisCurveOptionModelTest.cpp
    LINK_LIBRARIES kritaimage kritalibpaintop kritatestsdk)

kis_add_tests(kis_linked_pattern_manager_test.cpp
    LINK_LIBRARIES kritaimage kritalibpaintop kritatestsdk)

kis_add_test(
    KisPaintOpOptionUtilsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisPaintOpOptionUtilsContractTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisOptionTupleContractTest.cpp
    LINK_LIBRARIES
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisOptionTupleContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
)

kis_add_test(
    KisPrefixedOptionDataWrapperContractTest.cpp
    LINK_LIBRARIES
        Eigen3::Eigen
        Imath::ImathConfig
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisPrefixedOptionDataWrapperContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
)
target_compile_definitions(KisPrefixedOptionDataWrapperContractTest PRIVATE
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisAirbrushOptionDataContractTest.cpp
    LINK_LIBRARIES
        kritapaintopairbrushoptiondataobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisAirbrushOptionDataContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
    kritalibpaintop_EXPORTS
)

kis_add_test(
    KisColorOptionDataContractTest.cpp
    LINK_LIBRARIES
        kritapaintopcoloroptiondataobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisColorOptionDataContractTest PRIVATE
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisColorSourceOptionDataContractTest.cpp
    LINK_LIBRARIES
        kritapaintopcolorsourceoptiondataobjects
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisColorSourceOptionDataContractTest PRIVATE
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisCurveOptionDataCommonContractTest.cpp
    LINK_LIBRARIES
        kritapaintopcurveoptiondatacommonobjects
        kritapaintopsensorpackinterfaceobjects
        kritaglobalidobjects
        Boost::boost
        Eigen3::Eigen
        Imath::ImathConfig
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisCurveOptionDataCommonContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisPaintingModeOptionDataContractTest.cpp
    LINK_LIBRARIES
        kritapaintoppaintingmodeoptiondataobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisPaintingModeOptionDataContractTest PRIVATE
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisPrecisionOptionContractTest.cpp
    LINK_LIBRARIES
        kritapaintopprecisionoptionobjects
        Boost::boost
        Eigen3::Eigen
        Imath::ImathConfig
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisPrecisionOptionContractTest PRIVATE
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisFilterOptionDataContractTest.cpp
    LINK_LIBRARIES
        kritapaintopfilteroptiondataobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisFilterOptionDataContractTest PRIVATE
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisTextureOptionDataValueContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::I18n
)
target_include_directories(KisTextureOptionDataValueContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
)
target_compile_definitions(KisTextureOptionDataValueContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
    kritalibpaintop_EXPORTS
)

kis_add_test(
    KisTextureOptionDataIOContractTest.cpp
    LINK_LIBRARIES
        kritapaintoptextureoptionioobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisTextureOptionDataIOContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
    kritalibpaintop_EXPORTS
)

kis_add_test(
    KisTextureOptionLodContractTest.cpp
    LINK_LIBRARIES
        kritapaintoptexturelodobjects
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisTextureOptionLodContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapigment_EXPORTS
    kritalibpaintop_EXPORTS
)

kis_add_test(
    KisCurveControlStrategyInterfacesContractTest.cpp
    LINK_LIBRARIES
        kritapaintopcurvecontrolinterfaceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisCurveControlStrategyInterfacesContractTest PRIVATE
    kritalibpaintop_EXPORTS
)

kis_add_test(
    KisCurveRangeModelInterfaceContractTest.cpp
    LINK_LIBRARIES
        kritapaintopcurverangemodelinterfaceobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        lager
)
target_compile_definitions(KisCurveRangeModelInterfaceContractTest PRIVATE
    kritalibpaintop_EXPORTS
)

kis_add_test(
    KisColorOptionModelContractTest.cpp
    LINK_LIBRARIES
        kritapaintopcoloroptionmodelobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisColorOptionModelContractTest PRIVATE
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
    kritawidgets_EXPORTS
)

kis_add_test(
    KisSensorWithLengthModelContractTest.cpp
    LINK_LIBRARIES
        kritapaintopsensormodelobjects
        kritapaintopsensordataobjects
        kritaglobalidobjects
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        lager
)
target_compile_definitions(KisSensorWithLengthModelContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
)

kis_add_test(
    KisDrawingAngleSensorModelContractTest.cpp
    LINK_LIBRARIES
        kritapaintopsensormodelobjects
        kritapaintopsensordataobjects
        kritaglobalidobjects
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        lager
)
target_compile_definitions(KisDrawingAngleSensorModelContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
)

kis_add_test(
    KisMirrorOptionModelContractTest.cpp
    LINK_LIBRARIES
        kritapaintopmirroroptionmodelobjects
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisMirrorOptionModelContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisSensorDataContractTest.cpp
    LINK_LIBRARIES
        kritapaintopsensordataobjects
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisSensorDataContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
)

kis_add_test(
    KisSensorPackInterfaceContractTest.cpp
    LINK_LIBRARIES
        kritapaintopsensorpackinterfaceobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisSensorPackInterfaceContractTest PRIVATE
    kritalibpaintop_EXPORTS
)

kis_add_test(
    KisKritaSensorPackContractTest.cpp
    LINK_LIBRARIES
        kritapaintopkritasensorpackobjects
        kritapaintopsensordataobjects
        kritapaintopsensorpackinterfaceobjects
        kritaglobalidobjects
        Boost::boost
        Eigen3::Eigen
        Imath::ImathConfig
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisKritaSensorPackContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisDynamicSensorFactoryContractTest.cpp
    LINK_LIBRARIES
        kritapaintopdynamicsensorfactoryobjects
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisDynamicSensorFactoryContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
)

kis_add_test(
    KisSimpleDynamicSensorFactoryContractTest.cpp
    LINK_LIBRARIES
        kritapaintopdynamicsensorfactoryobjects
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisSimpleDynamicSensorFactoryContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
)
