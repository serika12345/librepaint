set(kritapaintingundo_LIB_SRCS
    kundo2stack.cpp
    kundo2group.cpp
    kundo2magicstring.cpp
    kundo2commandextradata.cpp
    kis_undo_store.cpp
    kis_undo_stores.cpp
    kis_command_utils.cpp
    KisCumulativeUndoData.cpp
)

kis_add_library(kritapaintingundo SHARED ${kritapaintingundo_LIB_SRCS})
generate_export_header(kritapaintingundo BASE_NAME kritapaintingundo)

target_link_libraries(kritapaintingundo
    PUBLIC
        kritawidgetutils
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ConfigGui
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Boost::boost
)

target_include_directories(kritapaintingundo
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

set_target_properties(kritapaintingundo PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION}
    SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritapaintingundo ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
