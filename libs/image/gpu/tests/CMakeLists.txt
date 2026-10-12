kis_add_test(KisGpuOperationDocumentTest.cpp
    LINK_LIBRARIES kritaimagegpuoperations Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisGpuOperationDocumentTest PRIVATE kritaimage_EXPORTS)
