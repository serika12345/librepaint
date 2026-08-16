include_directories(${QUAZIP_INCLUDE_DIRS})

set(kritaresourcestorage_LIB_SRCS
    KoDirectoryStore.cpp
    KoStoreDevice.cpp
    KoStore.cpp
    KoXmlNS.cpp
    KoXmlWriter.cpp
    KoQuaZipStore.cpp
    StoreDebug.cpp
)

kis_add_library(kritaresourcestorage SHARED ${kritaresourcestorage_LIB_SRCS})
generate_export_header(kritaresourcestorage BASE_NAME kritaresourcestorage)

target_link_libraries(kritaresourcestorage
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
    PRIVATE
        KF${KF_MAJOR}::ConfigCore
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Gui
        ${QUAZIP_LIBRARIES}
)

target_include_directories(kritaresourcestorage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

set_target_properties(kritaresourcestorage PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)
install(TARGETS kritaresourcestorage ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
