kis_add_tests(
    TestResourceCacheDb.cpp
    TestResourceLoaderRegistry.cpp
    TestResourceLocator.cpp
    TestResourceStorage.cpp
    TestFolderStorage.cpp
    TestMemoryStorage.cpp
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
    TestTag.cpp
    LINK_LIBRARIES
        kritatagobjects
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::I18n
)
target_include_directories(TestTag PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(TestTag PRIVATE kritaresources_EXPORTS)

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
    KisResourceThumbnailCacheContractTest.cpp
    LINK_LIBRARIES
        kritaresourcethumbnailcacheobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisResourceThumbnailCacheContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisResourceThumbnailCacheContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KisDirtyStateSaverTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisDirtyStateSaverTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisResourceTypesCompatibilityTest.cpp
    LINK_LIBRARIES
        kritaresourcestypesobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisResourceTypesCompatibilityTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KoResourceBundleManifestContractTest.cpp
    LINK_LIBRARIES
        kritaresourcebundlemanifestobjects
        kritaresourcestypesobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoResourceBundleManifestContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoResourceBundleManifestContractTest PRIVATE kritaresources_EXPORTS)

kis_add_test(
    KisResourceStorageTypeCompatibilityTest.cpp
    LINK_LIBRARIES
        kritaglobal
        kritaresources
        kritatestsdk
        Qt${QT_MAJOR_VERSION}::Sql
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::I18n
)
target_include_directories(KisResourceStorageTypeCompatibilityTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)







kis_add_test(
    KisTagPseudoUrlCompatibilityTest.cpp
    LINK_LIBRARIES
        kritaglobal
        kritaresources
        kritatestsdk
        Qt${QT_MAJOR_VERSION}::Sql
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::I18n
)
target_include_directories(KisTagPseudoUrlCompatibilityTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

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
    KisSqlQueryLoaderContractTest.cpp
    LINK_LIBRARIES
        kritasqlqueryloaderobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Sql
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisSqlQueryLoaderContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisSqlQueryLoaderContractTest PRIVATE kritaresources_EXPORTS)

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
        kritaresourcecachestorageobjects
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
    KisStorageFilterProxyModelContractTest.cpp
    LINK_LIBRARIES
        kritastoragefilterproxymodelobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
)
target_include_directories(KisStorageFilterProxyModelContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisStorageFilterProxyModelContractTest PRIVATE kritaresources_EXPORTS)
