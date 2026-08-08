cmake_minimum_required(VERSION 3.24)

project(KritaIOSLibunibreakConsumer LANGUAGES C)

find_package(
    libunibreak ${KRITA_IOS_LIBUNIBREAK_VERSION}
    EXACT MODULE REQUIRED
)

if(NOT TARGET libunibreak::libunibreak)
    message(FATAL_ERROR "Findlibunibreak did not define its imported target")
endif()

get_target_property(libunibreak_location libunibreak::libunibreak IMPORTED_LOCATION)
set(expected_location "${KRITA_IOS_LIBUNIBREAK_PREFIX}/lib/libunibreak.a")
if(NOT libunibreak_location STREQUAL expected_location)
    message(
        FATAL_ERROR
        "libunibreak target resolves to '${libunibreak_location}', expected '${expected_location}'"
    )
endif()

add_executable(krita-ios-libunibreak-consumer main.c)
target_link_libraries(
    krita-ios-libunibreak-consumer
    PRIVATE
        libunibreak::libunibreak
)

install(TARGETS krita-ios-libunibreak-consumer BUNDLE DESTINATION bin)
