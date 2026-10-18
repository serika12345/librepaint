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

add_dependencies(TestDocument
    kritapngimport kritapngexport kritalcmsengine
    kritapatterngenerator kritacolorgenerator)
add_dependencies(TestNode kritakraimport kritakraexport kritapngexport kritalcmsengine)
add_dependencies(TestNode kritaroundmarkerpaintop)
target_link_libraries(TestDocument kritaimpexkra)

qt6_add_resources(public_node_window_resources
    ../../../krita/krita.qrc
    ../../../krita/pics/Breeze-dark/breeze-dark-icons.qrc
    ../../../krita/pics/Breeze-light/breeze-light-icons.qrc)
target_sources(TestNode PRIVATE ${public_node_window_resources})
target_link_libraries(TestNode kritainputui kritadocumentpainting)
set_tests_properties(libs-libkis-TestNode PROPERTIES RUN_SERIAL TRUE)
if(APPLE AND NOT IOS)
    set_tests_properties(libs-libkis-TestNode PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
elseif(UNIX AND NOT ANDROID AND NOT IOS)
    set_tests_properties(libs-libkis-TestNode PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
elseif(WIN32)
    set_tests_properties(libs-libkis-TestNode PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows")
endif()
