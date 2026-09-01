if(OpenEXR_FOUND)
    include_directories(SYSTEM ${OPENEXR_INCLUDE_DIRS})
endif()
include_directories( ${LCMS2_INCLUDE_DIR} )

kis_add_test(
    LcmsColorProfileContainerSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(LcmsColorProfileContainerSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../colorprofiles
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
)
target_compile_definitions(LcmsColorProfileContainerSchemaContractTest PRIVATE
    kritalcmsengine_EXPORTS
    kritapigment_EXPORTS
)

kis_add_tests(
    TestKoLcmsColorProfile.cpp
    TestColorSpaceRegistry.cpp
    TestLcmsRGBP2020PQColorSpace.cpp
    TestProfileGeneration.cpp
    LINK_LIBRARIES kritalcmsengine_static kritawidgets kritapigment KF${KF_MAJOR}::I18n kritatestsdk ${LCMS2_LIBRARIES}
    )

if (KRITA_USE_SURFACE_COLOR_MANAGEMENT_API)
    kis_add_test(
        TestIccFromColorimetryConversion.cpp
        LINK_LIBRARIES kritapigment kritatestsdk ${LCMS2_LIBRARIES} kritasurfacecolormanagementapi
    )
endif()
