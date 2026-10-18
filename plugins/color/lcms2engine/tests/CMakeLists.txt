if(OpenEXR_FOUND)
    include_directories(SYSTEM ${OPENEXR_INCLUDE_DIRS})
endif()
include_directories( ${LCMS2_INCLUDE_DIR} )


kis_add_tests(
    TestKoLcmsColorProfile.cpp
    TestLcmsRGBP2020PQColorSpace.cpp
    TestProfileGeneration.cpp
    LINK_LIBRARIES kritalcmsengine_static kritawidgets kritapigment KF${KF_MAJOR}::I18n kritatestsdk ${LCMS2_LIBRARIES}
    )

kis_add_test(
    TestColorSpaceRegistry.cpp
    LINK_LIBRARIES kritawidgets kritapigment KF${KF_MAJOR}::I18n kritatestsdk ${LCMS2_LIBRARIES}
)
foreach(test_target IN ITEMS TestKoLcmsColorProfile TestColorSpaceRegistry TestLcmsRGBP2020PQColorSpace TestProfileGeneration)
    add_dependencies(${test_target} kritalcmsengine)
endforeach()

if (KRITA_USE_SURFACE_COLOR_MANAGEMENT_API)
    kis_add_test(
        TestIccFromColorimetryConversion.cpp
        LINK_LIBRARIES kritapigment kritatestsdk ${LCMS2_LIBRARIES} kritasurfacecolormanagementapi
    )
endif()
