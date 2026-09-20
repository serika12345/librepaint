set(kritasurfacecolormanagementapi_LIB_SRCS
    surfacecolormanagement/KisOutputColorInfoInterface.cpp
    surfacecolormanagement/KisSurfaceColorimetryIccUtils.cpp
    surfacecolormanagement/KisSurfaceColorManagerInterface.cpp
    surfacecolormanagement/KisSurfaceColorManagementInfo.cpp
)

kis_add_library(kritasurfacecolormanagementapi SHARED
    ${kritasurfacecolormanagementapi_LIB_SRCS}
    $<TARGET_OBJECTS:kritasurfacecolorimetryobjects>
)

target_link_libraries(kritasurfacecolormanagementapi
    PRIVATE
    kritaglobal
    kritapigment
    ${QT_QTCORE_LIBRARY})

set_target_properties(kritasurfacecolormanagementapi PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritasurfacecolormanagementapi  ${INSTALL_TARGETS_DEFAULT_ARGS})
