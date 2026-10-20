kis_add_test(TestChannelDocker.cpp
    ../ChannelDockerWidget.cpp
    ../channelmodel.cpp
    ../KisChannelsThumbnailsStrokeStrategy.cpp
    TEST_NAME TestChannelDocker
    TEST_NAME_VAR channel_docker_test
    LINK_LIBRARIES kritaapplicationui kritainputui kritadocumentpainting kritatestsdk
)
target_include_directories(TestChannelDocker PRIVATE ..)
add_dependencies(TestChannelDocker kritaroundmarkerpaintop kritalcmsengine)
qt6_add_resources(channel_docker_window_resources
    ../../../../krita/krita.qrc
    ../../../../krita/pics/Breeze-dark/breeze-dark-icons.qrc
    ../../../../krita/pics/Breeze-light/breeze-light-icons.qrc)
target_sources(TestChannelDocker PRIVATE ${channel_docker_window_resources})
set_tests_properties(${channel_docker_test} PROPERTIES RUN_SERIAL TRUE)
if(APPLE AND NOT IOS)
    set_tests_properties(${channel_docker_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
elseif(UNIX AND NOT ANDROID AND NOT IOS)
    set_tests_properties(${channel_docker_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
endif()
