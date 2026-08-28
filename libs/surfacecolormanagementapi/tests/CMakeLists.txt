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
