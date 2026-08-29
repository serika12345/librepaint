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

add_library(kritatagobjects OBJECT
    KisTag.cpp
)
target_compile_definitions(kritatagobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritatagobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritatagobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritatagobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourcebundlemanifestobjects OBJECT
    KoResourceBundleManifest.cpp
)
target_compile_definitions(kritaresourcebundlemanifestobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritaresourcebundlemanifestobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaresourcebundlemanifestobjects
    PUBLIC
        kritaxmlserialization
        kritaglobaldebugobjects
        kritaresourcestypesobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritaresourcebundlemanifestobjects PROPERTIES
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

add_library(kritalocalstrokecanvasresourcesobjects OBJECT
    KoLocalStrokeCanvasResources.cpp
)
target_compile_definitions(kritalocalstrokecanvasresourcesobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritalocalstrokecanvasresourcesobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritalocalstrokecanvasresourcesobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritalocalstrokecanvasresourcesobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourcemetadatamodelobjects OBJECT
    KisResourceMetaDataModel.cpp
)
target_compile_definitions(kritaresourcemetadatamodelobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritaresourcemetadatamodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaresourcemetadatamodelobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Sql
)
set_target_properties(kritaresourcemetadatamodelobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritadatabasetransactionlockobjects OBJECT
    KisDatabaseTransactionLock.cpp
)
target_compile_definitions(kritadatabasetransactionlockobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritadatabasetransactionlockobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritadatabasetransactionlockobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Sql
)
set_target_properties(kritadatabasetransactionlockobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritasqlqueryloaderobjects OBJECT
    KisSqlQueryLoader.cpp
)
target_compile_definitions(kritasqlqueryloaderobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritasqlqueryloaderobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritasqlqueryloaderobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Sql
)
set_target_properties(kritasqlqueryloaderobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritatemporaryresourcestoragelockobjects OBJECT
    KisTemporaryResourceStorageLock.cpp
)
target_compile_definitions(kritatemporaryresourcestoragelockobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritatemporaryresourcestoragelockobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritatemporaryresourcestoragelockobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritatemporaryresourcestoragelockobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritatemporaryresourcestoragelocatorobjects OBJECT
    KisTemporaryResourceStorageLocator.cpp
)
target_compile_definitions(kritatemporaryresourcestoragelocatorobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritatemporaryresourcestoragelocatorobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritatemporaryresourcestoragelocatorobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritatemporaryresourcestoragelocatorobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourcesearchboxfilterobjects OBJECT
    KisResourceSearchBoxFilter.cpp
)
target_compile_definitions(kritaresourcesearchboxfilterobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritaresourcesearchboxfilterobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaresourcesearchboxfilterobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaresourcesearchboxfilterobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourcethumbnailcacheobjects OBJECT
    KisResourceThumbnailCache.cpp
)
target_compile_definitions(kritaresourcethumbnailcacheobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritaresourcethumbnailcacheobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaresourcethumbnailcacheobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaresourcethumbnailcacheobjects PROPERTIES
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

add_library(kritaresourcecacheprefixedwrapperobjects OBJECT
    KoResourceCachePrefixedStorageWrapper.cpp
)
target_compile_definitions(kritaresourcecacheprefixedwrapperobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritaresourcecacheprefixedwrapperobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaresourcecacheprefixedwrapperobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaresourcecacheprefixedwrapperobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaresourcecachestorageobjects OBJECT
    KoResourceCacheStorage.cpp
)
target_compile_definitions(kritaresourcecachestorageobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritaresourcecachestorageobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaresourcecachestorageobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaresourcecachestorageobjects PROPERTIES
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

add_library(kritastoragepluginobjects OBJECT
    KisStoragePlugin.cpp
)
target_compile_definitions(kritastoragepluginobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritastoragepluginobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/koplugin>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/koplugin>
)
target_link_libraries(kritastoragepluginobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritastoragepluginobjects PROPERTIES
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

add_library(kritalocalstrokeresourcesobjects OBJECT
    KisLocalStrokeResources.cpp
)
target_compile_definitions(kritalocalstrokeresourcesobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritalocalstrokeresourcesobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritalocalstrokeresourcesobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritalocalstrokeresourcesobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaglobalresourcesinstanceobjects OBJECT
    KisGlobalResourcesInterface.cpp
)
target_compile_definitions(kritaglobalresourcesinstanceobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritaglobalresourcesinstanceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaglobalresourcesinstanceobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaglobalresourcesinstanceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaglobalresourcesmodelsourceobjects OBJECT
    KisGlobalResourcesInterfaceSource.cpp
)
target_compile_definitions(kritaglobalresourcesmodelsourceobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritaglobalresourcesmodelsourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaglobalresourcesmodelsourceobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritaglobalresourcesmodelsourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritastoragefilterproxymodelobjects OBJECT
    KisStorageFilterProxyModel.cpp
)
target_compile_definitions(kritastoragefilterproxymodelobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritastoragefilterproxymodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritastoragefilterproxymodelobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritastoragefilterproxymodelobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritastoragefilterproxymodelsourceobjects OBJECT
    KisStorageFilterProxyModelSource.cpp
)
target_compile_definitions(kritastoragefilterproxymodelsourceobjects PRIVATE kritaresources_EXPORTS)
target_include_directories(kritastoragefilterproxymodelsourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritastoragefilterproxymodelsourceobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritastoragefilterproxymodelsourceobjects PROPERTIES
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
    KisResourceIterator.cpp
    KisResourceQueryMapper.cpp
    KisBundleStorage.cpp
    KisFolderStorage.cpp
    KisMemoryStorage.cpp

    KisTagModel.cpp

    KisTagResourceModel.cpp

    KoResourceBundle.cpp
    KoResourcePaths.cpp
    KisRequiredResourcesOperators.cpp

    KisEmbeddedResourceStorageProxy.cpp
)

qt_add_resources(kritaresources_LIB_SRCS sql.qrc)
krita_ios_add_static_resource(sql)

kis_add_library(kritaresources SHARED
    ${kritaresources_LIB_SRCS}
    $<TARGET_OBJECTS:kritaresourcestypesobjects>
    $<TARGET_OBJECTS:kritatagobjects>
    $<TARGET_OBJECTS:kritaresourcebundlemanifestobjects>
    $<TARGET_OBJECTS:kritacanvasresourcesinterfaceobjects>
    $<TARGET_OBJECTS:kritalocalstrokecanvasresourcesobjects>
    $<TARGET_OBJECTS:kritaresourcemetadatamodelobjects>
    $<TARGET_OBJECTS:kritadatabasetransactionlockobjects>
    $<TARGET_OBJECTS:kritasqlqueryloaderobjects>
    $<TARGET_OBJECTS:kritatemporaryresourcestoragelockobjects>
    $<TARGET_OBJECTS:kritatemporaryresourcestoragelocatorobjects>
    $<TARGET_OBJECTS:kritaresourcesearchboxfilterobjects>
    $<TARGET_OBJECTS:kritaresourcethumbnailcacheobjects>
    $<TARGET_OBJECTS:kritaresourcecacheinterfaceobjects>
    $<TARGET_OBJECTS:kritaresourcecacheprefixedwrapperobjects>
    $<TARGET_OBJECTS:kritaresourcecachestorageobjects>
    $<TARGET_OBJECTS:kritaembeddedresourcevalueobjects>
    $<TARGET_OBJECTS:kritaresourceloadvalueobjects>
    $<TARGET_OBJECTS:kritastoragepluginobjects>
    $<TARGET_OBJECTS:kritaresourcesinterfaceobjects>
    $<TARGET_OBJECTS:kritalocalstrokeresourcesobjects>
    $<TARGET_OBJECTS:kritaglobalresourcesinstanceobjects>
    $<TARGET_OBJECTS:kritaglobalresourcesmodelsourceobjects>
    $<TARGET_OBJECTS:kritastoragefilterproxymodelobjects>
    $<TARGET_OBJECTS:kritastoragefilterproxymodelsourceobjects>
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
