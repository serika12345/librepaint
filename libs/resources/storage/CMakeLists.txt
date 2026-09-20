include_directories(${QUAZIP_INCLUDE_DIRS})

add_library(kritaresourcestoragelogobjects OBJECT
    StoreDebug.cpp
)
target_compile_definitions(kritaresourcestoragelogobjects PRIVATE kritaresourcestorage_EXPORTS)
target_include_directories(kritaresourcestoragelogobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaresourcestoragelogobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaresourcestoragelogobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourcestoragedeviceobjects OBJECT
    KoStoreDevice.cpp
)
target_compile_definitions(kritaresourcestoragedeviceobjects PRIVATE kritaresourcestorage_EXPORTS)
target_include_directories(kritaresourcestoragedeviceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaresourcestoragedeviceobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaresourcestoragedeviceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritaresourcestorage_LIB_SRCS
    KoDirectoryStore.cpp
    KoStoreDevice.h
    KoStore.cpp
    KoQuaZipStore.cpp
)

kis_add_library(kritaresourcestorage SHARED
    ${kritaresourcestorage_LIB_SRCS}
    $<TARGET_OBJECTS:kritaresourcestoragelogobjects>
    $<TARGET_OBJECTS:kritaresourcestoragedeviceobjects>
)
generate_export_header(kritaresourcestorage BASE_NAME kritaresourcestorage)

target_link_libraries(kritaresourcestorage
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
    PRIVATE
        KF${KF_MAJOR}::ConfigCore
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
