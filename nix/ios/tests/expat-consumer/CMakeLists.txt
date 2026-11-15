cmake_minimum_required(VERSION 3.20)

project(KritaIOSExpatConsumer LANGUAGES C)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
find_package(
    expat ${KRITA_IOS_EXPAT_VERSION} EXACT CONFIG REQUIRED
    COMPONENTS char context_bytes dtd ns
)

foreach(feature IN ITEMS char context_bytes dtd ns)
    if(NOT expat_${feature}_FOUND)
        message(FATAL_ERROR "Required Expat feature is disabled: ${feature}")
    endif()
endforeach()

foreach(feature IN ITEMS attr_info large_size min_size ushort wchar_t)
    if(expat_${feature}_FOUND)
        message(FATAL_ERROR "Unexpected Expat feature is enabled: ${feature}")
    endif()
endforeach()

if(NOT TARGET expat::expat)
    message(FATAL_ERROR "Expat CMake package did not export expat::expat")
endif()
get_target_property(expat_target_type expat::expat TYPE)
if(NOT expat_target_type STREQUAL "STATIC_LIBRARY")
    message(FATAL_ERROR "expat::expat is ${expat_target_type}, expected STATIC_LIBRARY")
endif()

add_executable(krita-ios-expat-consumer main.c)
target_compile_features(krita-ios-expat-consumer PRIVATE c_std_99)
target_link_libraries(krita-ios-expat-consumer PRIVATE expat::expat)
