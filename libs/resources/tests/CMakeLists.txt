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
    TestResourceSearchBoxFilter.cpp
    TestStorageFilterProxyModel.cpp
    TestTagResourceModel.cpp
    LINK_LIBRARIES kritaglobal kritapigment kritaplugin kritaresources kritawidgets kritaversion KF${KF_MAJOR}::ConfigCore Qt${QT_MAJOR_VERSION}::Sql kritatestsdk
    )

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
