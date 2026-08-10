add_subdirectory( dtd )
add_subdirectory( data )
# Install the application icons following the freedesktop icon theme spec
add_subdirectory( "pics/branding/${BRANDING}" )

if (ANDROID)
    if (QT_MAJOR_VERSION STREQUAL "5")
        set (QtAndroidExtras_INCLUDE_DIRS ${Qt5AndroidExtras_INCLUDE_DIRS})
        include_directories (${QtAndroidExtras_INCLUDE_DIRS})
    endif()
endif()

set(krita_SRCS main.cc)
set(KRITA_MACOS_BUNDLE_ICON_FILE "LibrePaint")

if (IOS)
    set(KRITA_IOS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/pics/branding/${BRANDING}/1024-apps-krita.png")
    set_source_files_properties(${KRITA_IOS_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    list(APPEND krita_SRCS
        ${KRITA_IOS_ICON}
        KisIOSLifecycleHandler.mm
        KisIOSMemoryWarningHandler.mm
        KisIOSPencilInteraction.mm
    )
endif()

# Set the application icon on the application. The iOS asset catalog is added
# separately once the port has a final icon set.
if (IOS)
    set(ICON_SRCS "")
elseif (NOT APPLE)
	file(GLOB ICON_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/pics/branding/${BRANDING}/*-apps-krita.png")
else()
    if(NOT KRITA_MACOS_USE_XCODE_TOOLS)
        set(KRITA_MACOS_ICON "${CMAKE_SOURCE_DIR}/packaging/macos/LibrePaint.icns")
        set_source_files_properties(${KRITA_MACOS_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        list(APPEND krita_SRCS ${KRITA_MACOS_ICON})
        set(KRITA_MACOS_BUNDLE_ICON_FILE "LibrePaint.icns")
    # Xcode below 26 cannot work with .icon format
    elseif(XCODE_VERSION VERSION_GREATER_EQUAL "26.0")
        find_program(XCRUN_EXECUTABLE NAMES xcrun)
        set(KRITA_ASSETS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/pics/branding/${BRANDING}/LibrePaint.icon")
        cmake_path(SET ASSETS_PATH "${CMAKE_CURRENT_BINARY_DIR}/assets")

        set(ASSETS_FILES "${ASSETS_PATH}/Assets.car" "${ASSETS_PATH}/LibrePaint.icns")
        add_custom_command(
          OUTPUT ${ASSETS_FILES}
          COMMAND ${CMAKE_COMMAND} ARGS -E make_directory "${ASSETS_PATH}"
          COMMAND ${XCRUN_EXECUTABLE} ARGS actool "${KRITA_ASSETS_ICON}"
            --compile ${ASSETS_PATH}
            --platform macosx --minimum-deployment-target ${MACOS_MINIMUM_VERSION}
            --app-icon LibrePaint --output-partial-info-plist "${CMAKE_CURRENT_BINARY_DIR}/librepaint_icon.plist"
            --output-format human-readable-text
            --standalone-icon-behavior all
          DEPENDS
            "${KRITA_ASSETS_ICON}"
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    else()
        set(ICON_SRCS
             "${CMAKE_CURRENT_SOURCE_DIR}/pics/branding/${BRANDING}/16-apps-krita.png"
             "${CMAKE_CURRENT_SOURCE_DIR}/pics/branding/${BRANDING}/32-apps-krita.png"
             "${CMAKE_CURRENT_SOURCE_DIR}/pics/branding/${BRANDING}/48-apps-krita.png"
             "${CMAKE_CURRENT_SOURCE_DIR}/pics/branding/${BRANDING}/128-apps-krita.png"
             "${CMAKE_CURRENT_SOURCE_DIR}/pics/branding/${BRANDING}/256-apps-krita.png"
             "${CMAKE_CURRENT_SOURCE_DIR}/pics/branding/${BRANDING}/512-apps-krita.png"
             "${CMAKE_CURRENT_SOURCE_DIR}/pics/branding/${BRANDING}/1024-apps-krita.png"
        )
    endif() # XCode version
endif()

if (WIN32)
    # Private headers are needed for main.cc to enable WinTab support
    set_source_files_properties(
        main.cc
        PROPERTIES
        INCLUDE_DIRECTORIES "${Qt${QT_MAJOR_VERSION}Gui_PRIVATE_INCLUDE_DIRS}"
        )
endif()

if (WIN32)
    set(krita_windows_stub_ICONS_SRCS "")
    ecm_add_app_icon(krita_windows_stub_ICONS_SRCS ICONS ${ICON_SRCS} OUTFILE_BASENAME krita)

    # This is used to generate an ICO file for the file icon, We don't actually
    # want to embed this icon into an EXE.
	file(GLOB krafile_ICON_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/pics/mimetypes/*-mimetypes-application-x-krita.png")
    set(krafile_dummy_ICONS_SRCS "")
    ecm_add_app_icon(krafile_dummy_ICONS_SRCS ICONS ${krafile_ICON_SRCS} OUTFILE_BASENAME kritafile)
    add_library(krafile_dummy_obj OBJECT ${krafile_dummy_ICONS_SRCS})

    # These icons are used to replace those in the shell extension package.
    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/krita.ico
            ${CMAKE_CURRENT_BINARY_DIR}/kritafile.ico
        DESTINATION ${CMAKE_INSTALL_PREFIX}/installer
    )
elseif(APPLE AND NOT IOS)
    if(NOT KRITA_MACOS_USE_XCODE_TOOLS)
      # The prebuilt ICNS file is already part of krita_SRCS.
    elseif(XCODE_VERSION VERSION_GREATER_EQUAL "26.0")
      set_source_files_properties(${ASSETS_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    else()
      ecm_add_app_icon(krita_SRCS OUTFILE_BASENAME LibrePaint ICONS ${ICON_SRCS})
    endif()
elseif(NOT IOS)
    ecm_add_app_icon(krita_SRCS ICONS ${ICON_SRCS})
endif()

# Install the mimetype icons
ecm_install_icons(ICONS
    "${CMAKE_CURRENT_SOURCE_DIR}/pics/mimetypes/16-mimetypes-application-x-krita.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/pics/mimetypes/22-mimetypes-application-x-krita.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/pics/mimetypes/32-mimetypes-application-x-krita.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/pics/mimetypes/48-mimetypes-application-x-krita.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/pics/mimetypes/64-mimetypes-application-x-krita.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/pics/mimetypes/128-mimetypes-application-x-krita.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/pics/mimetypes/256-mimetypes-application-x-krita.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/pics/mimetypes/512-mimetypes-application-x-krita.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/pics/mimetypes/1024-mimetypes-application-x-krita.png"
    DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)

# Android has a different splash screen.
if(ANDROID)
    set(krita_splash_qrc splash-android.qrc)
else()
    set(krita_splash_qrc splash.qrc)
endif()

# Application resources shared by the supported platform builds.
set(krita_QRCS
    ${CMAKE_SOURCE_DIR}/krita/krita.qrc
    ${CMAKE_SOURCE_DIR}/krita/pics/Breeze-dark/breeze-dark-icons.qrc
    ${CMAKE_SOURCE_DIR}/krita/pics/Breeze-light/breeze-light-icons.qrc
    ${CMAKE_SOURCE_DIR}/krita/pics/layerbox/layerbox-icons.qrc
    ${CMAKE_SOURCE_DIR}/krita/pics/layers/layers-icons.qrc
    ${CMAKE_SOURCE_DIR}/krita/pics/misc-light/misc-light-icons.qrc
    ${CMAKE_SOURCE_DIR}/krita/pics/misc-dark/misc-dark-icons.qrc
    ${CMAKE_SOURCE_DIR}/krita/pics/paintops/paintops-icons.qrc
    ${CMAKE_SOURCE_DIR}/krita/pics/tools/SVG/16/tools-svg-16-icons.qrc
    ${CMAKE_SOURCE_DIR}/krita/pics/tool_transform/tool-transform-icons.qrc
    ${CMAKE_SOURCE_DIR}/krita/pics/svg/svg-icons.qrc
    ${CMAKE_SOURCE_DIR}/libs/flake/flake.qrc
    ${CMAKE_SOURCE_DIR}/libs/widgets/kritawidgets.qrc
    ${CMAKE_SOURCE_DIR}/pics/icons.qrc
    ${CMAKE_SOURCE_DIR}/krita/data/aboutdata/aboutdata.qrc
    ${CMAKE_SOURCE_DIR}/krita/data/shaders/shaders.qrc
    ${CMAKE_SOURCE_DIR}/krita/data/cursors/cursors.qrc
    ${CMAKE_SOURCE_DIR}/krita/data/gamutmasks/gamutmasks.qrc
    ${CMAKE_SOURCE_DIR}/plugins/paintops/defaultpresets/defaultpresets.qrc
    ${CMAKE_SOURCE_DIR}/krita/pics/branding/${BRANDING}/branding.qrc
    ${CMAKE_SOURCE_DIR}/krita/data/splash/${krita_splash_qrc}
    CACHE INTERNAL "krita_QRCS"
)

if (QT_MAJOR_VERSION STREQUAL "6")
    list(APPEND krita_QRCS ${CMAKE_SOURCE_DIR}/krita/data/mime-database/mime-database.qrc)
endif()

qt_add_resources(krita_SRCS ${krita_QRCS})

if (ANDROID AND QT_MAJOR_VERSION STREQUAL "5")
    kis_add_library(krita SHARED ${krita_SRCS})
    target_link_libraries(krita PRIVATE Qt5::AndroidExtras)
elseif (WIN32)
    # Build the main implementation into a DLL to be called by a stub EXE.
    kis_add_library(krita SHARED ${krita_SRCS})
    set_target_properties(krita PROPERTIES PREFIX "")
elseif(IOS)
    qt_add_executable(krita MACOSX_BUNDLE ${krita_SRCS})
else()
    add_executable(krita ${krita_SRCS})
endif()
target_link_libraries(krita
    PRIVATE
                      kritamultiarch
                      kritaui
                      Qt${QT_MAJOR_VERSION}::Core
                      Qt${QT_MAJOR_VERSION}::Gui
                      Qt${QT_MAJOR_VERSION}::Widgets
                      Qt${QT_MAJOR_VERSION}::Xml
                      Qt${QT_MAJOR_VERSION}::Network
                      Qt${QT_MAJOR_VERSION}::Svg)
if(IOS AND QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(krita PRIVATE
        Qt6::QSvgIconPlugin
        Qt6::QSvgIconPlugin_init
        Qt6::QSvgPlugin
        Qt6::QSvgPlugin_init
    )
endif()
if(IOS AND KRITA_IOS_BUILD_PLUGINS)
    krita_ios_target_static_plugins(krita)
endif()
if(IOS)
    krita_ios_target_static_resources(krita)
endif()
if(NOT IOS)
    target_link_libraries(krita PRIVATE Qt${QT_MAJOR_VERSION}::PrintSupport)
endif()
if(HAVE_KCRASH)
    target_link_libraries(krita PRIVATE KF${KF_MAJOR}::Crash)
endif()

if (APPLE AND NOT IOS)
    set(KRITA_MACOS_INSTALL_RPATH
        "@loader_path/../../../../lib"
        "@loader_path/../lib"
        "@loader_path/../Frameworks"
        "@executable_path/../../../../lib"
        "@executable_path/../lib"
        "@executable_path/../Frameworks"
    )
    set_target_properties(krita PROPERTIES
        OUTPUT_NAME "LibrePaint"
        MACOSX_BUNDLE TRUE
        INSTALL_RPATH "${KRITA_MACOS_INSTALL_RPATH}"
    )
    set_target_properties(krita PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.template)
    set_target_properties(krita PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "local.librepaint.macos")
    set_target_properties(krita PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "LibrePaint")
    set_target_properties(krita PROPERTIES MACOSX_BUNDLE_ICON_FILE "${KRITA_MACOS_BUNDLE_ICON_FILE}")
    set_target_properties(krita PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING ${LIBREPAINT_VERSION_STRING})
    set_target_properties(krita PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING ${LIBREPAINT_VERSION_STRING})
    set_target_properties(krita PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${LIBREPAINT_VERSION_STRING})
    set_target_properties(krita PROPERTIES MACOSX_BUNDLE_COPYRIGHT "Copyright 1999-2026 The Krita Developers; LibrePaint modifications copyright 2026 LibrePaint contributors")
endif ()

if(IOS)
    set_target_properties(krita PROPERTIES
        OUTPUT_NAME "LibrePaint"
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.ios.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER "local.librepaint.ipad"
        MACOSX_BUNDLE_BUNDLE_NAME "LibrePaint"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${LIBREPAINT_VERSION_STRING}
        MACOSX_BUNDLE_BUNDLE_VERSION ${LIBREPAINT_VERSION_STRING}
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "2"
    )
endif()

if (NOT ANDROID AND NOT IOS)
  set(KRITAVERSION_SRCS kritaversion.cpp)
  add_executable(krita_version ${KRITAVERSION_SRCS})
  target_link_libraries(krita_version
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        kritaversion
  )

  if(APPLE)
      set_target_properties(krita_version PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks;@executable_path/../lib;@executable_path/../Frameworks")
      set_property(TARGET krita_version PROPERTY MACOSX_BUNDLE OFF)
  endif ()

  install(TARGETS krita_version ${INSTALL_TARGETS_DEFAULT_ARGS})
endif()


if(APPLE AND NOT IOS)
    set(KRITA_ASSET_KRZ_ICNS "krita-krz.icns" "krita-kra.icns")
    add_custom_command(
      OUTPUT ${KRITA_ASSET_KRZ_ICNS}
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy
      "./pics/mimetypes/krz/krita-krz.icns"
      "./pics/mimetypes/krita-kra.icns"
      "${CMAKE_CURRENT_BINARY_DIR}"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set_source_files_properties(${KRITA_ASSET_KRZ_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    # make krita depend on assets icons
    if(KRITA_MACOS_USE_XCODE_TOOLS AND XCODE_VERSION VERSION_GREATER_EQUAL "26.0")
      target_sources(krita PRIVATE ${ASSETS_FILES})
    endif()
    target_sources(krita PRIVATE ${KRITA_ASSET_KRZ_ICNS})

    install(TARGETS krita ${INSTALL_TARGETS_DEFAULT_ARGS} BUNDLE DESTINATION bin)
elseif(IOS)
    install(TARGETS krita ${INSTALL_TARGETS_DEFAULT_ARGS} BUNDLE DESTINATION bin)
else()
    install(TARGETS krita ${INSTALL_TARGETS_DEFAULT_ARGS})
endif()
install(FILES org.kde.krita.desktop DESTINATION ${KDE_INSTALL_APPDIR})

set(actions_files krita.action kritamenu.action)
if (ANDROID)
    # In-app-purchase-related actions are only relevant on Android. They
    # wouldn't show up in the menu anyway because the application doesn't
    # instantiate them, but they'd still appear in the keyboard shortcuts.
    list(APPEND actions_files kritamenu_android.action)
endif()
install(FILES ${actions_files} DESTINATION ${KDE_INSTALL_DATADIR}/krita/actions)

install(FILES  org.kde.krita.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )

install(DIRECTORY DESTINATION ${KDE_INSTALL_DATADIR}/krita/shortcuts)

if (WIN32)
    configure_file(versioninfo.rc.in ${CMAKE_CURRENT_BINARY_DIR}/versioninfo.rc)
    if (QT_MAJOR_VERSION STREQUAL "6")
        configure_file(krita6.exe.manifest.in ${CMAKE_CURRENT_BINARY_DIR}/krita.exe.manifest)
    else()
        configure_file(krita.exe.manifest.in ${CMAKE_CURRENT_BINARY_DIR}/krita.exe.manifest)
    endif()
    if (MINGW)
        # CMake does not know how to embed manifest on mingw, so we create our own RC file to be included
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/krita.exe.manifest.rc "1 24 \"krita.exe.manifest\"\n")
        set(KRITA_MANIFEST_SRC ${CMAKE_CURRENT_BINARY_DIR}/krita.exe.manifest.rc)
    else()
        set(KRITA_MANIFEST_SRC
            ${CMAKE_CURRENT_BINARY_DIR}/krita.exe.manifest)
    endif()
    add_executable(krita_windows_stub_exe
        windows_stub_main.cpp
        ${krita_windows_stub_ICONS_SRCS}
        ${CMAKE_CURRENT_BINARY_DIR}/versioninfo.rc
        ${KRITA_MANIFEST_SRC}
    )
    target_link_libraries(krita_windows_stub_exe PRIVATE krita)
    if (QT_MAJOR_VERSION STREQUAL "6")
        target_link_libraries(krita_windows_stub_exe PRIVATE Qt6::EntryPointPrivate)
    else()
        target_link_libraries(krita_windows_stub_exe PRIVATE Qt5::WinMain)
    endif()
    set_target_properties(krita_windows_stub_exe
        PROPERTIES
            OUTPUT_NAME "krita"
    )
    if (MSVC)
        target_link_options(krita_windows_stub_exe PRIVATE "/DEBUG:NONE")
    endif()
    install(TARGETS krita_windows_stub_exe ${INSTALL_TARGETS_DEFAULT_ARGS})

    add_executable(krita_windows_stub_com
        windows_stub_main.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/versioninfo.rc
        ${KRITA_MANIFEST_SRC}
    )
    target_link_libraries(krita_windows_stub_com PRIVATE krita)
    set_target_properties(krita_windows_stub_com
        PROPERTIES
            WIN32_EXECUTABLE false
            OUTPUT_NAME "krita"
            SUFFIX ".com"
    )
    if (MSVC)
        target_link_options(krita_windows_stub_com PRIVATE "/DEBUG:NONE")
        # If MSVC uses incremental linking, it will race the database
        # for krita.exe with the one for krita.com.
        add_dependencies(krita_windows_stub_com krita_windows_stub_exe)
    endif()
    install(TARGETS krita_windows_stub_com ${INSTALL_TARGETS_DEFAULT_ARGS})
endif (WIN32)
