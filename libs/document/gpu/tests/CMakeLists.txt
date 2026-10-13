kis_add_test(KisGpuPaintDocumentTest.cpp TEST_NAME_VAR gpu_paint_document_test
    LINK_LIBRARIES kritadocumentgpupaint kritaimagegpupaint kritaimagegputiles kritaimagegpuoperations
        kritaimpexkra Qt6::GuiPrivate Qt6::Test Qt6::Xml ${QUAZIP_LIBRARIES})
target_compile_definitions(KisGpuPaintDocumentTest PRIVATE
    kritadocumentpainting_EXPORTS kritaimage_EXPORTS kritaimpex_EXPORTS)
if(WIN32)
    target_link_libraries(KisGpuPaintDocumentTest kritaimage)
else()
    target_link_libraries(KisGpuPaintDocumentTest kritaimagecompressionobjects)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_tests_properties(${gpu_paint_document_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_tests_properties(${gpu_paint_document_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
endif()
