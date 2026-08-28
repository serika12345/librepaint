kis_add_tests(
    TestResourceCacheDb.cpp
    TestResourceLoaderRegistry.cpp
    TestResourceLocator.cpp
    TestResourceStorage.cpp
    TestFolderStorage.cpp
    TestMemoryStorage.cpp
    TestTag.cpp
    TestBundleStorage.cpp
    TestResourceModel.cpp
    TestTagFilterResourceProxyModel.cpp
    TestTagModel.cpp
    TestResourceTypeModel.cpp
    TestStorageModel.cpp
    TestStorageFilterProxyModel.cpp
    TestTagResourceModel.cpp
    LINK_LIBRARIES kritaglobal kritapigment kritaplugin kritaresources kritawidgets kritaversion KF${KF_MAJOR}::ConfigCore Qt${QT_MAJOR_VERSION}::Sql kritatestsdk
    )

kis_add_test(
    TestResourceSearchBoxFilter.cpp
    LINK_LIBRARIES
        kritaresourcesearchboxfilterobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(TestResourceSearchBoxFilter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(TestResourceSearchBoxFilter PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KisDirtyStateSaverTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisDirtyStateSaverTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KoCanvasResourceIdsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoCanvasResourceIdsContractTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisResourceTypesContractTest.cpp
    LINK_LIBRARIES
        kritaresourcestypesobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisResourceTypesContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KisResourceModelEnumContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::I18n
        Boost::boost
)
target_include_directories(KisResourceModelEnumContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisResourceModelEnumContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KoCanvasResourcesInterfaceContractTest.cpp
    LINK_LIBRARIES
        kritacanvasresourcesinterfaceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoCanvasResourcesInterfaceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoCanvasResourcesInterfaceContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KoLocalStrokeCanvasResourcesContractTest.cpp
    LINK_LIBRARIES
        kritalocalstrokecanvasresourcesobjects
        kritacanvasresourcesinterfaceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoLocalStrokeCanvasResourcesContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoLocalStrokeCanvasResourcesContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KisResourceMetaDataModelContractTest.cpp
    LINK_LIBRARIES
        kritaresourcemetadatamodelobjects
        Qt${QT_MAJOR_VERSION}::Sql
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisResourceMetaDataModelContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisResourceMetaDataModelContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KisDatabaseTransactionLockContractTest.cpp
    LINK_LIBRARIES
        kritadatabasetransactionlockobjects
        Qt${QT_MAJOR_VERSION}::Sql
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisDatabaseTransactionLockContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisDatabaseTransactionLockContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KisTemporaryResourceStorageLockContractTest.cpp
    LINK_LIBRARIES
        kritatemporaryresourcestoragelockobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisTemporaryResourceStorageLockContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisTemporaryResourceStorageLockContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KoResourceCacheInterfaceContractTest.cpp
    LINK_LIBRARIES
        kritaresourcecacheinterfaceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoResourceCacheInterfaceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoResourceCacheInterfaceContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KoResourceCachePrefixedStorageWrapperContractTest.cpp
    LINK_LIBRARIES
        kritaresourcecacheprefixedwrapperobjects
        kritaresourcecacheinterfaceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoResourceCachePrefixedStorageWrapperContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoResourceCachePrefixedStorageWrapperContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KoResourceCacheStorageContractTest.cpp
    LINK_LIBRARIES
        kritaresourcecachestorageobjects
        kritaresourcecacheinterfaceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoResourceCacheStorageContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoResourceCacheStorageContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KoEmbeddedResourceValueContractTest.cpp
    LINK_LIBRARIES
        kritaembeddedresourcevalueobjects
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KoEmbeddedResourceValueContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoEmbeddedResourceValueContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KoResourceLoadResultContractTest.cpp
    LINK_LIBRARIES
        kritaresourceloadvalueobjects
        kritaembeddedresourcevalueobjects
        kritaresourcestypesobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KoResourceLoadResultContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoResourceLoadResultContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KoResourceContractTest.cpp
    LINK_LIBRARIES
        kritaresourceloadvalueobjects
        kritaembeddedresourcevalueobjects
        kritaresourcestypesobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KoResourceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoResourceContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KisResourcesInterfaceContractTest.cpp
    LINK_LIBRARIES
        kritaresourcesinterfaceobjects
        kritaresourceloadvalueobjects
        kritaembeddedresourcevalueobjects
        kritaresourcestypesobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisResourcesInterfaceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisResourcesInterfaceContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KisLocalStrokeResourcesContractTest.cpp
    LINK_LIBRARIES
        kritalocalstrokeresourcesobjects
        kritaresourcesinterfaceobjects
        kritaresourceloadvalueobjects
        kritaembeddedresourcevalueobjects
        kritaresourcestypesobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisLocalStrokeResourcesContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisLocalStrokeResourcesContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KisGlobalResourcesInterfaceContractTest.cpp
    LINK_LIBRARIES
        kritaglobalresourcesinstanceobjects
        kritaresourcesinterfaceobjects
        kritaresourceloadvalueobjects
        kritaembeddedresourcevalueobjects
        kritaresourcestypesobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisGlobalResourcesInterfaceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisGlobalResourcesInterfaceContractTest PRIVATE kritaresources_EXPORTS)
