kis_add_test(KisGpuPaintDocumentTest.cpp TEST_NAME_VAR gpu_paint_document_test
    LINK_LIBRARIES kritadocumentpainting Qt6::Test Qt6::Gui Qt6::Xml ${QUAZIP_LIBRARIES})
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_tests_properties(${gpu_paint_document_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_tests_properties(${gpu_paint_document_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
endif()
