kis_add_test(KisGpuOperationDocumentTest.cpp
    LINK_LIBRARIES kritaimagegpuoperations Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisGpuOperationDocumentTest PRIVATE kritaimage_EXPORTS)

kis_add_test(KisGpuTileRendererTest.cpp
    LINK_LIBRARIES kritaimagegputiles kritaimagegpuoperations Qt6::GuiPrivate Qt6::Test
)
target_compile_definitions(KisGpuTileRendererTest PRIVATE kritaimage_EXPORTS)
