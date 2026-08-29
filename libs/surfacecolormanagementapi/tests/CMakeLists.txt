kis_add_test(
    KisSurfaceColorimetryContractTest.cpp
    LINK_LIBRARIES
        kritasurfacecolorimetryobjects
        kritapigmentcolorimetryobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisSurfaceColorimetryContractTest PRIVATE
    kritapigment_EXPORTS
    kritasurfacecolormanagementapi_EXPORTS
)

kis_add_test(
    KisSurfaceColorimetryIccUtilsContractTest.cpp
    ../surfacecolormanagement/KisSurfaceColorimetryIccUtils.cpp
    TEST_NAME KisSurfaceColorimetryIccUtilsContractTest
    LINK_LIBRARIES
        kritapigmentcolorimetryobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisSurfaceColorimetryIccUtilsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
)
target_compile_definitions(KisSurfaceColorimetryIccUtilsContractTest PRIVATE
    kritapigment_EXPORTS
    kritasurfacecolormanagementapi_EXPORTS
)

kis_add_test(
    KisSurfaceColorManagementInfoContractTest.cpp
    ../surfacecolormanagement/KisSurfaceColorManagementInfo.cpp
    TEST_NAME KisSurfaceColorManagementInfoContractTest
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisSurfaceColorManagementInfoContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisSurfaceColorManagementInfoContractTest PRIVATE
    kritasurfacecolormanagementapi_EXPORTS
)

kis_add_test(
    SurfaceColorInterfacesContractTest.cpp
    ../surfacecolormanagement/KisOutputColorInfoInterface.cpp
    ../surfacecolormanagement/KisSurfaceColorManagerInterface.cpp
    TEST_NAME SurfaceColorInterfacesContractTest
    LINK_LIBRARIES
        kritapigmentcolorimetryobjects
        kritasurfacecolorimetryobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(SurfaceColorInterfacesContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
)
target_compile_definitions(SurfaceColorInterfacesContractTest PRIVATE
    kritapigment_EXPORTS
    kritasurfacecolormanagementapi_EXPORTS
)
