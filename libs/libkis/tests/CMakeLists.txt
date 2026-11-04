kis_add_tests(
    #TestKrita.cpp
    TestChannel.cpp
    TestDocument.cpp
    TestNode.cpp
    TestVectorLayer.cpp
    TestShape.cpp
    TestFilter.cpp
    TestManagedColor.cpp
    TestNotifier.cpp
    TestAngleSelector.cpp
    LINK_LIBRARIES kritalibkis kritatestsdk
    )

target_sources(TestNode PRIVATE TestNodeHierarchy.cpp TestNodeClipboard.cpp TestNodeLayerProperties.cpp TestNodeDocumentSession.cpp TestNodeFigureGroups.cpp TestNodeMaskCrop.cpp)
target_sources(TestDocument PRIVATE TestDocumentU16Blend.cpp TestDocumentKraPreview.cpp TestDocumentJpegExport.cpp TestDocumentLayerMetadata.cpp TestDocumentPngImport.cpp TestDocumentJpegImport.cpp TestDocumentLayerState.cpp TestDocumentDefaultBrush.cpp)

add_dependencies(TestDocument
    kritakraimport kritakraexport kritapngimport kritapngexport kritajpegimport kritajpegexport kritalcmsengine
    kritaexif kritaiptc kritaxmp
    kritapatterngenerator kritacolorgenerator)
add_dependencies(TestDocument kritadefaultpaintops kritaroundmarkerpaintop)
add_dependencies(TestNode kritakraimport kritakraexport kritapngimport kritapngexport kritalcmsengine)
add_dependencies(TestNode kritaroundmarkerpaintop)
add_dependencies(TestNode kritadefaulttools)
add_dependencies(TestNode kritatoolcrop)
add_dependencies(TestNode kritalayerdocker kritacolorsfilters)
target_link_libraries(TestDocument kritaimpexkra kritadocumentpainting)

kis_add_test(TestGpuEngineDefault.cpp
    TEST_NAME_VAR gpu_engine_default_test
    TEST_ARGS engineSelection ordinaryDocumentsUseSelectedEngine excludedColorDocumentsUseCpuEngine
    LINK_LIBRARIES kritalibkis kritatestsdk)
add_dependencies(TestGpuEngineDefault kritalcmsengine
    kritakraimport kritakraexport kritapngimport kritapngexport kritajpegimport kritajpegexport)
set_tests_properties(${gpu_engine_default_test} PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_test(NAME libs-libkis-no-adapter-TestGpuEngineDefault
        COMMAND TestGpuEngineDefault ordinaryDocumentsUseSelectedEngine)
    set_tests_properties(libs-libkis-no-adapter-TestGpuEngineDefault PROPERTIES
        TIMEOUT 120 RUN_SERIAL TRUE
        ENVIRONMENT "LIBREPAINT_TEST_GPU_UNAVAILABLE=1;VK_DRIVER_FILES=/nonexistent/librepaint-vulkan-driver.json;VK_ICD_FILENAMES=/nonexistent/librepaint-vulkan-driver.json")
endif()

kis_add_test(TestGpuNativeLayerTransfer.cpp
    TEST_NAME_VAR gpu_native_layer_transfer_test
    LINK_LIBRARIES kritalibkis kritatestsdk)
add_dependencies(TestGpuNativeLayerTransfer kritalcmsengine kritakraimport kritakraexport)
set_tests_properties(${gpu_native_layer_transfer_test} PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)

qt6_add_resources(public_node_window_resources
    ../../../krita/krita.qrc
    ../../../krita/pics/Breeze-dark/breeze-dark-icons.qrc
    ../../../krita/pics/Breeze-light/breeze-light-icons.qrc)
target_sources(TestNode PRIVATE ${public_node_window_resources})
target_link_libraries(TestNode kritainputui kritadocumentpainting)
set_tests_properties(libs-libkis-TestNode libs-libkis-TestDocument PROPERTIES RUN_SERIAL TRUE)
if(APPLE AND NOT IOS)
    set_tests_properties(libs-libkis-TestNode libs-libkis-TestDocument PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
elseif(UNIX AND NOT ANDROID AND NOT IOS)
    set_tests_properties(libs-libkis-TestNode libs-libkis-TestDocument PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
elseif(WIN32)
    set_tests_properties(libs-libkis-TestNode libs-libkis-TestDocument PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows")
endif()
