find_package(Vulkan REQUIRED)
find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)
find_program(SPIRV_VALIDATOR spirv-val REQUIRED)
set(poc_shaders)
foreach(stage vert frag)
    set(shader "${CMAKE_CURRENT_BINARY_DIR}/frame.${stage}.spv")
    add_custom_command(OUTPUT "${shader}"
        COMMAND "${GLSLANG_VALIDATOR}" -V --target-env vulkan1.0
            "${CMAKE_CURRENT_SOURCE_DIR}/frame.${stage}" -o "${shader}"
        COMMAND "${SPIRV_VALIDATOR}" --target-env vulkan1.0 "${shader}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/frame.${stage}" VERBATIM)
    list(APPEND poc_shaders "${shader}")
endforeach()

add_library(kritauivulkanpresentationobjects OBJECT KisVulkanFrameWindow.cpp)
set_target_properties(kritauivulkanpresentationobjects PROPERTIES
    AUTOMOC ON POSITION_INDEPENDENT_CODE ON)
target_include_directories(kritauivulkanpresentationobjects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kritauivulkanpresentationobjects PUBLIC Qt6::Gui Qt6::Widgets Vulkan::Headers)
qt_add_resources(kritauivulkanpresentationobjects vulkan_poc_shaders
    PREFIX "/librepaint/vulkan-poc" BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${poc_shaders} OUTPUT_TARGETS poc_resource_targets)
set(moltenvk_poc_resource_targets ${poc_resource_targets} PARENT_SCOPE)
get_target_property(poc_vulkan_headers Vulkan::Headers INTERFACE_INCLUDE_DIRECTORIES)
set(moltenvk_poc_vulkan_headers ${poc_vulkan_headers} PARENT_SCOPE)

if(BUILD_TESTING)
    add_executable(KisVulkanPresentationContractTest tests/KisVulkanPresentationContractTest.cpp)
    set_target_properties(KisVulkanPresentationContractTest PROPERTIES AUTOMOC ON)
    target_link_libraries(KisVulkanPresentationContractTest PRIVATE
        kritauivulkanpresentationobjects Qt6::Test)
    add_test(NAME KisVulkanPresentationContractTest COMMAND KisVulkanPresentationContractTest)
    set_tests_properties(KisVulkanPresentationContractTest PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=cocoa" LABELS "gpu;screen;moltenvk-poc" TIMEOUT 120)
endif()
