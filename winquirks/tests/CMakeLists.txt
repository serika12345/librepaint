cmake_minimum_required(VERSION 3.19)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(WinQuirksContracts LANGUAGES CXX)
    include(CTest)
endif()

if(NOT MSVC)
    message(FATAL_ERROR "WinQuirksUnistdContractTest requires an MSVC-compatible compiler")
endif()

add_executable(
    WinQuirksUnistdContractTest
    WinQuirksUnistdContractTest.cpp
)
target_compile_features(WinQuirksUnistdContractTest PRIVATE cxx_std_17)
target_include_directories(
    WinQuirksUnistdContractTest
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/..
)

add_test(
    NAME winquirks-WinQuirksUnistdContractTest
    COMMAND WinQuirksUnistdContractTest
)
set_tests_properties(
    winquirks-WinQuirksUnistdContractTest
    PROPERTIES LABELS "windows;contract"
)
