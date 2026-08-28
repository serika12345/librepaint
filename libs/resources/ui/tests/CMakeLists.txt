kis_add_test(
    TestResourceUiContract.cpp
    LINK_LIBRARIES
        kritaresourceuidescriptorobjects
        kritaresourceitemchooserconstructionobjects
        kritaresourcestypesobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
target_include_directories(TestResourceUiContract PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
)
target_compile_definitions(TestResourceUiContract PRIVATE
    kritaresourceui_EXPORTS
    kritaresources_EXPORTS
    kritawidgetutils_EXPORTS
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

kis_add_test(
    KisResourceItemViewContractTest.cpp
    LINK_LIBRARIES
        kritaresourceitemviewobjects
        kritakineticscrollerobjects
        kritaicontooltipobjects
        kritaitemtooltipobjects
        kritacheckerboardpainterobjects
        kritaresourcethumbnailcacheobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisResourceItemViewContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisResourceItemViewContractTest PRIVATE
    kritaresourceui_EXPORTS
    kritawidgetutils_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    KisResourceItemListViewContractTest.cpp
    LINK_LIBRARIES
        kritaresourceitemlistviewobjects
        kritakineticscrollerobjects
        kritaicontooltipobjects
        kritaitemtooltipobjects
        kritacheckerboardpainterobjects
        kritaresourcethumbnailcacheobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisResourceItemListViewContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisResourceItemListViewContractTest PRIVATE
    kritaresourceui_EXPORTS
    kritawidgetutils_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    KisResourceItemListWidgetContractTest.cpp
    LINK_LIBRARIES
        kritaresourceitemlistwidgetobjects
        kritakineticscrollerobjects
        kritaicontooltipobjects
        kritaitemtooltipobjects
        kritacheckerboardpainterobjects
        kritaresourcethumbnailcacheobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisResourceItemListWidgetContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisResourceItemListWidgetContractTest PRIVATE
    kritaresourceui_EXPORTS
    kritawidgetutils_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    KisStorageChooserDelegateContractTest.cpp
    LINK_LIBRARIES
        kritastoragechooserdelegateobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisStorageChooserDelegateContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
)
target_compile_definitions(KisStorageChooserDelegateContractTest PRIVATE
    kritaresourceui_EXPORTS
    kritawidgetutils_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    KisStorageChooserWidgetContractTest.cpp
    LINK_LIBRARIES
        kritastoragechooserwidgetobjects
        kritastoragechooserdelegateobjects
        kritastoragefilterproxymodelobjects
        kritapopupbuttonobjects
        kritaresourcestypesobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisStorageChooserWidgetContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisStorageChooserWidgetContractTest PRIVATE
    kritaresourceui_EXPORTS
    kritawidgetutils_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    KisResourceUserOperationsContractTest.cpp
    LINK_LIBRARIES
        kritaresourceuseroperationsoverwriteobjects
        kritaresourceuseroperationsnameusageobjects
        kritaresourceuseroperationsimportobjects
        kritaresourceuseroperationsrenameobjects
        kritaresourceuseroperationsaddobjects
        kritaresourceuseroperationsupdateobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisResourceUserOperationsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisResourceUserOperationsContractTest PRIVATE
    kritaresourceui_EXPORTS
    kritawidgetutils_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    KisTagChooserWidgetContractTest.cpp
    LINK_LIBRARIES
        kritatagchooserwidgetobjects
        kritatagchooserwidgetaddobjects
        kritatagchooserwidgetselectionobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisTagChooserWidgetContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisTagChooserWidgetContractTest PRIVATE
    kritaresourceui_EXPORTS
    kritaresources_EXPORTS
)

kis_add_test(
    TagActionsContractTest.cpp
    LINK_LIBRARIES
        kritatagactionsobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
target_include_directories(TagActionsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
)
target_compile_definitions(TagActionsContractTest PRIVATE
    kritaresourceui_EXPORTS
    kritaresources_EXPORTS
)
