kis_add_test(
    VirtualChannelInfoContractTest.cpp
    LINK_LIBRARIES
        kritafiltersvirtualchannelinfoobjects
        kritapigmentstandardidsobjects
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::I18n
        Boost::boost
)
target_include_directories(VirtualChannelInfoContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
)
target_compile_definitions(VirtualChannelInfoContractTest PRIVATE
    kritaglobal_EXPORTS
    kritapigment_EXPORTS
)
