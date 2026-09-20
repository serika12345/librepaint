if (UNIX AND NOT APPLE AND HAVE_DBUS)
    add_subdirectory(colord)
    set(kritacolor_LIB_SRCS kis_color_manager.h linux/kis_color_manager.cpp)
    set(kritacolor_EXTRA_LIBRARIES kritacolord)
    set_target_properties(kritacolord PROPERTIES
       VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION} )
elseif (MSVC)
    set(kritacolor_LIB_SRCS kis_color_manager.h dummy/kis_color_manager.cpp ${CMAKE_CURRENT_BINARY_DIR}/kritacolor_export.h)
    set(kritacolor_USES_DUMMY_BACKEND TRUE)
else ()
    set(kritacolor_LIB_SRCS kis_color_manager.h dummy/kis_color_manager.cpp)
    set(kritacolor_USES_DUMMY_BACKEND TRUE)
endif ()

kis_add_library(kritacolor SHARED ${kritacolor_LIB_SRCS})
generate_export_header(kritacolor BASE_NAME kritacolor)

target_link_libraries(kritacolor
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
    PRIVATE
        ${kritacolor_EXTRA_LIBRARIES}
)

set_target_properties(kritacolor PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritacolor  ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
