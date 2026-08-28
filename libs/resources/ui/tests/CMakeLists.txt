kis_add_tests(
    TestResourceUiContract.cpp
    LINK_LIBRARIES kritaresourceui Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    ResourceListViewModesContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(ResourceListViewModesContractTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KisResourceUiDescriptorContractTest.cpp
    LINK_LIBRARIES
        kritaresourceuidescriptorobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisResourceUiDescriptorContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisResourceUiDescriptorContractTest PRIVATE kritaresourceui_EXPORTS)

kis_add_test(
    KisTagLabelContractTest.cpp
    LINK_LIBRARIES
        kritataglabelobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisTagLabelContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisTagLabelContractTest PRIVATE kritaresourceui_EXPORTS)

kis_add_test(
    KisIconToolTipContractTest.cpp
    LINK_LIBRARIES
        kritaicontooltipobjects
        kritaitemtooltipobjects
        kritacheckerboardpainterobjects
        kritaresourcethumbnailcacheobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisIconToolTipContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisIconToolTipContractTest PRIVATE
    kritaresourceui_EXPORTS
    kritawidgetutils_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    KisResourceThumbnailPainterContractTest.cpp
    LINK_LIBRARIES
        kritaresourcethumbnailpainterobjects
        kritacheckerboardpainterobjects
        kritaresourcethumbnailcacheobjects
        kritaresourcestypesobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisResourceThumbnailPainterContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisResourceThumbnailPainterContractTest PRIVATE
    kritaresourceui_EXPORTS
    kritawidgetutils_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    KisResourceItemDelegateContractTest.cpp
    LINK_LIBRARIES
        kritaresourceitemdelegateobjects
        kritaresourcethumbnailpainterobjects
        kritacheckerboardpainterobjects
        kritaresourcethumbnailcacheobjects
        kritaresourcestypesobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisResourceItemDelegateContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisResourceItemDelegateContractTest PRIVATE
    kritaresourceui_EXPORTS
    kritawidgetutils_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    KisResourceItemChooserSyncContractTest.cpp
    LINK_LIBRARIES
        kritaresourceitemchoosersyncobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisResourceItemChooserSyncContractTest PRIVATE
    kritaresourceui_EXPORTS
)
