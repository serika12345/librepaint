add_subdirectory( tests )

include(CheckFunctionExists)
check_function_exists(backtrace HAVE_BACKTRACE)
configure_file(config-debug.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-debug.h)

option(HAVE_MEMORY_LEAK_TRACKER "Enable memory leak tracker (always disabled in release build)" OFF)
option(HAVE_BACKTRACE_SUPPORT "Enable recording of backtrace in memory leak tracker" OFF)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-memory-leak-tracker.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-memory-leak-tracker.h) ### WRONG PLACE???

add_library(kritaglobalfileutilsobjects OBJECT
    KisFileUtils.cpp
)
target_compile_definitions(kritaglobalfileutilsobjects PRIVATE kritaglobal_EXPORTS)
target_include_directories(kritaglobalfileutilsobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaglobalfileutilsobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaglobalfileutilsobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaglobalcumulativeundodataobjects OBJECT
    KisCumulativeUndoData.cpp
)
target_compile_definitions(kritaglobalcumulativeundodataobjects PRIVATE kritaglobal_EXPORTS)
target_include_directories(kritaglobalcumulativeundodataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaglobalcumulativeundodataobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        KF${KF_MAJOR}::ConfigCore
)
set_target_properties(kritaglobalcumulativeundodataobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaglobaltemporaryfileconfigurationobjects OBJECT
    KisTemporaryFileConfiguration.cpp
)
target_compile_definitions(kritaglobaltemporaryfileconfigurationobjects PRIVATE kritaglobal_EXPORTS)
target_include_directories(kritaglobaltemporaryfileconfigurationobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaglobaltemporaryfileconfigurationobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        KF${KF_MAJOR}::ConfigCore
)
if(APPLE AND NOT IOS)
    target_link_libraries(kritaglobaltemporaryfileconfigurationobjects PRIVATE kritamacosutils)
endif()
set_target_properties(kritaglobaltemporaryfileconfigurationobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaglobalqpainterstatesaverobjects OBJECT
    KisQPainterStateSaver.cpp
)
target_compile_definitions(kritaglobalqpainterstatesaverobjects PRIVATE kritaglobal_EXPORTS)
target_include_directories(kritaglobalqpainterstatesaverobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaglobalqpainterstatesaverobjects PUBLIC Qt${QT_MAJOR_VERSION}::Gui)
set_target_properties(kritaglobalqpainterstatesaverobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaglobaldeletelaterwrapperobjects OBJECT
    KisDeleteLaterWrapper.cpp
)
target_compile_definitions(kritaglobaldeletelaterwrapperobjects PRIVATE kritaglobal_EXPORTS)
target_include_directories(kritaglobaldeletelaterwrapperobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaglobaldeletelaterwrapperobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritaglobaldeletelaterwrapperobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaglobalbackupobjects OBJECT
    KisBackup.cpp
)
target_compile_definitions(kritaglobalbackupobjects PRIVATE kritaglobal_EXPORTS)
target_include_directories(kritaglobalbackupobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaglobalbackupobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaglobalbackupobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaglobalcursoroverridelockobjects OBJECT
    KisCursorOverrideLock.cpp
)
target_compile_definitions(kritaglobalcursoroverridelockobjects PRIVATE kritaglobal_EXPORTS)
target_include_directories(kritaglobalcursoroverridelockobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaglobalcursoroverridelockobjects PUBLIC Qt${QT_MAJOR_VERSION}::Gui)
set_target_properties(kritaglobalcursoroverridelockobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaglobalfilteredrollingmeanobjects OBJECT
    KisFilteredRollingMean.cpp
)
target_compile_definitions(kritaglobalfilteredrollingmeanobjects PRIVATE kritaglobal_EXPORTS)
target_include_directories(kritaglobalfilteredrollingmeanobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaglobalfilteredrollingmeanobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Boost::boost
)
set_target_properties(kritaglobalfilteredrollingmeanobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaglobalsharedobjects OBJECT
    kis_shared.cpp
)
target_compile_definitions(kritaglobalsharedobjects PRIVATE kritaglobal_EXPORTS)
target_include_directories(kritaglobalsharedobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaglobalsharedobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaglobalsharedobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritaglobal_LIB_SRCS
    kis_assert.cpp
    kis_debug.cpp
    kis_algebra_2d.cpp
    kis_memory_leak_tracker.cpp
    kis_dom_utils.cpp
    kis_painting_tweaks.cpp
    KisHandlePainterHelper.cpp
    KisHandleStyle.cpp
    kis_signal_compressor.cpp
    kis_signal_compressor_with_param.cpp
    kis_thread_safe_signal_compressor.cpp
    kis_acyclic_signal_connector.cpp
    kis_latency_tracker.cpp
    KisRollingMeanAccumulatorWrapper.cpp
    KisRollingSumAccumulatorWrapper.cpp
    kis_config_notifier.cpp
    KisUsageLogger.cpp
    KisSignalMapper.cpp
    KisRegion.cpp
    KoID.cpp
    KoUnit.cpp
    KisBezierUtils.cpp
    KisBezierPatch.cpp
    KisBezierMesh.cpp
    KisRectsGrid.cpp
    KisSynchronizedConnection.cpp
    KisSampleRectIterator.cpp
    kis_random_source.cpp
    KisNodeAdditionFlags.h
    KisMessageBoxWrapper.cpp
    KisTransformComponents.cpp
)

if (ANDROID)
    list(APPEND kritaglobal_LIB_SRCS
        KisAndroidCrashHandler.cpp
        KisAndroidExitInfo.cpp
        KisAndroidLogHandler.cpp
        KisAndroidUtils.cpp
    )
    include_directories(${unwindstack_INCLUDE_DIR})
    if (QT_MAJOR_VERSION STREQUAL "5")
        include_directories(${Qt5AndroidExtras_INCLUDE_DIRS})
    endif()
endif()

kis_add_library(kritaglobal SHARED
    ${kritaglobal_LIB_SRCS}
    $<TARGET_OBJECTS:kritaglobalfileutilsobjects>
    $<TARGET_OBJECTS:kritaglobalcumulativeundodataobjects>
    $<TARGET_OBJECTS:kritaglobaltemporaryfileconfigurationobjects>
    $<TARGET_OBJECTS:kritaglobalqpainterstatesaverobjects>
    $<TARGET_OBJECTS:kritaglobaldeletelaterwrapperobjects>
    $<TARGET_OBJECTS:kritaglobalbackupobjects>
    $<TARGET_OBJECTS:kritaglobalcursoroverridelockobjects>
    $<TARGET_OBJECTS:kritaglobalfilteredrollingmeanobjects>
    $<TARGET_OBJECTS:kritaglobalsharedobjects>
)

target_sources(kritaglobal
  PRIVATE
    KisMessageBoxWrapper.h
    KisMessageBoxWrapper.cpp
)
generate_export_header(kritaglobal BASE_NAME kritaglobal)

target_link_libraries(kritaglobal 
    PUBLIC
        kritaversion
        Qt${QT_MAJOR_VERSION}::Core 
        Qt${QT_MAJOR_VERSION}::Gui 
        Qt${QT_MAJOR_VERSION}::Widgets 
        Qt${QT_MAJOR_VERSION}::Xml 
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ConfigCore
        Boost::boost
        zug
        lager
    PRIVATE
        Eigen3::Eigen
)

if (QT_MAJOR_VERSION STREQUAL "6")
  target_link_libraries(kritaglobal PUBLIC Qt6::Core5Compat Qt6::OpenGL)
endif()

if (GSL_FOUND)
  target_link_libraries(kritaglobal PRIVATE GSL::gsl)
endif ()

if (ANDROID)
    if (QT_MAJOR_VERSION STREQUAL "6")
        target_link_libraries(kritaglobal PRIVATE ${unwindstack_LIBRARIES} log)
    else()
        target_link_libraries(kritaglobal PRIVATE ${unwindstack_LIBRARIES} Qt5::AndroidExtras log)
    endif()
endif()

if(APPLE AND NOT IOS)
    target_link_libraries(kritaglobal PRIVATE
        kritamacosutils
    )
endif ()

set_target_properties(kritaglobal PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritaglobal  ${INSTALL_TARGETS_DEFAULT_ARGS})
