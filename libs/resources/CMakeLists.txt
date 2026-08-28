include_directories(${QUAZIP_INCLUDE_DIRS})

add_subdirectory(tests)

add_library(kritaresourcestypesobjects OBJECT
    KisResourceTypes.cpp
    ResourceDebug.cpp
)
target_compile_definitions(kritaresourcestypesobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritaresourcestypesobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaresourcestypesobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourcestypesobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritacanvasresourcesinterfaceobjects OBJECT
    KoCanvasResourcesInterface.cpp
)
target_compile_definitions(kritacanvasresourcesinterfaceobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritacanvasresourcesinterfaceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritacanvasresourcesinterfaceobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritacanvasresourcesinterfaceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourcecacheinterfaceobjects OBJECT
    KoResourceCacheInterface.cpp
)
target_compile_definitions(kritaresourcecacheinterfaceobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritaresourcecacheinterfaceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
)
target_link_libraries(kritaresourcecacheinterfaceobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaresourcecacheinterfaceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaembeddedresourcevalueobjects OBJECT
    KoEmbeddedResource.cpp
    KoMD5Generator.cpp
    KoResourceSignature.cpp
)
target_compile_definitions(kritaembeddedresourcevalueobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritaembeddedresourcevalueobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaembeddedresourcevalueobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Boost::boost
)
set_target_properties(kritaembeddedresourcevalueobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourceloadvalueobjects OBJECT
    KoResource.cpp
    KoResourceLoadResult.cpp
)
target_compile_definitions(kritaresourceloadvalueobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritaresourceloadvalueobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaresourceloadvalueobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourceloadvalueobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourcesinterfaceobjects OBJECT
    KisResourcesInterface.cpp
)
target_compile_definitions(kritaresourcesinterfaceobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritaresourcesinterfaceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaresourcesinterfaceobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourcesinterfaceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritaresources_LIB_SRCS
    KisResourceCacheDb.cpp
    KisResourceLoader.cpp
    KisResourceLoaderRegistry.cpp
    KisResourceLocator.cpp
    KisResourceStorage.cpp
    KisResourceModel.cpp
    KisTagFilterResourceProxyModel.cpp
    KisResourceModelProvider.cpp
    KisResourceTypeModel.cpp
    KisStorageModel.cpp
    KisStorageFilterProxyModel.cpp
    KisResourceIterator.cpp
    KisResourceSearchBoxFilter.cpp
    KisResourceQueryMapper.cpp
    KisStoragePlugin.cpp
    KisBundleStorage.cpp
    KisFolderStorage.cpp
    KisMemoryStorage.cpp

    KisTag.cpp
    KisTagModel.cpp

    KisTagResourceModel.cpp

    KoResourceBundle.cpp
    KoResourceBundleManifest.cpp
    KoResourcePaths.cpp
    KisRequiredResourcesOperators.cpp
    KisLocalStrokeResources.cpp
    KisGlobalResourcesInterface.cpp
    KoResourceCacheStorage.cpp
    KoResourceCachePrefixedStorageWrapper.cpp

    KoLocalStrokeCanvasResources.cpp
    KisEmbeddedResourceStorageProxy.cpp

    KisResourceThumbnailCache.cpp
    KisResourceMetaDataModel.cpp
    KisSqlQueryLoader.cpp
    KisDatabaseTransactionLock.cpp
    KisTemporaryResourceStorageLock.cpp
)

qt_add_resources(kritaresources_LIB_SRCS sql.qrc)
krita_ios_add_static_resource(sql)

kis_add_library(kritaresources SHARED
    ${kritaresources_LIB_SRCS}
    $<TARGET_OBJECTS:kritaresourcestypesobjects>
    $<TARGET_OBJECTS:kritacanvasresourcesinterfaceobjects>
    $<TARGET_OBJECTS:kritaresourcecacheinterfaceobjects>
    $<TARGET_OBJECTS:kritaembeddedresourcevalueobjects>
    $<TARGET_OBJECTS:kritaresourceloadvalueobjects>
    $<TARGET_OBJECTS:kritaresourcesinterfaceobjects>
)

generate_export_header(kritaresources BASE_NAME kritaresources)

target_link_libraries(kritaresources
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Sql
        Boost::boost
        kritaxmlserialization
    PRIVATE
        kritaversion
        kritaglobal
        kritaplugin
        kritaresourcestorage
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::CoreAddons
        KF${KF_MAJOR}::I18n
        ${QUAZIP_LIBRARIES}
)

set_target_properties(kritaresources PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)
install(TARGETS kritaresources ${INSTALL_TARGETS_DEFAULT_ARGS} )

add_subdirectory(ui)
