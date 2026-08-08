cmake_minimum_required(VERSION 3.24)

project(KritaIOSFriBidiConsumer LANGUAGES C)

find_package(
    FriBidi ${KRITA_IOS_FRIBIDI_VERSION}
    EXACT MODULE REQUIRED
)

if(NOT TARGET FriBidi::FriBidi)
    message(FATAL_ERROR "FindFriBidi did not define its imported target")
endif()

if(NOT FriBidi_VERSION STREQUAL KRITA_IOS_FRIBIDI_VERSION)
    message(
        FATAL_ERROR
        "FriBidi version is '${FriBidi_VERSION}', expected '${KRITA_IOS_FRIBIDI_VERSION}'"
    )
endif()

get_target_property(fribidi_location FriBidi::FriBidi IMPORTED_LOCATION)
set(expected_location "${KRITA_IOS_FRIBIDI_PREFIX}/lib/libfribidi.a")
if(NOT fribidi_location STREQUAL expected_location)
    message(
        FATAL_ERROR
        "FriBidi target resolves to '${fribidi_location}', expected '${expected_location}'"
    )
endif()

get_target_property(fribidi_includes FriBidi::FriBidi INTERFACE_INCLUDE_DIRECTORIES)
set(expected_include "${KRITA_IOS_FRIBIDI_PREFIX}/include/fribidi")
if(NOT expected_include IN_LIST fribidi_includes)
    message(
        FATAL_ERROR
        "FriBidi includes are '${fribidi_includes}', expected '${expected_include}'"
    )
endif()

add_executable(krita-ios-fribidi-consumer main.c)
target_link_libraries(
    krita-ios-fribidi-consumer
    PRIVATE
        FriBidi::FriBidi
)

install(TARGETS krita-ios-fribidi-consumer BUNDLE DESTINATION bin)
