project(kritapigment)

ko_compile_for_all_implementations_no_scalar(__per_arch_factory_objs compositeops/KoOptimizedCompositeOpFactoryPerArch.cpp)
ko_compile_for_all_implementations(__per_arch_alpha_applicator_factory_objs KoAlphaMaskApplicatorFactoryImpl.cpp)
ko_compile_for_all_implementations(__per_arch_rgb_scaler_factory_objs KoOptimizedPixelDataScalerU8ToU16FactoryImpl.cpp)

message("Following objects are generated from the per-arch lib")
foreach(_obj IN LISTS __per_arch_factory_objs __per_arch_alpha_applicator_factory_objs __per_arch_rgb_scaler_factory_objs)
    message("    * ${_obj}")
endforeach()

add_subdirectory(tests)
add_subdirectory(benchmarks)

add_library(kritapigmentstandardidsobjects OBJECT
    KoColorModelStandardIds.cpp
)
target_compile_definitions(kritapigmentstandardidsobjects PRIVATE kritapigment_EXPORTS)
target_include_directories(kritapigmentstandardidsobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapigmentstandardidsobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        KF${KF_MAJOR}::I18n
        Boost::boost
)
set_target_properties(kritapigmentstandardidsobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapigmentcolorimetryobjects OBJECT
    KoColorimetryUtils.cpp
)
target_compile_definitions(kritapigmentcolorimetryobjects PRIVATE kritapigment_EXPORTS)
target_include_directories(kritapigmentcolorimetryobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapigmentcolorimetryobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
)
set_target_properties(kritapigmentcolorimetryobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapigmentcmykmathsobjects OBJECT
    KoCmykColorSpaceMaths.cpp
)
target_compile_definitions(kritapigmentcmykmathsobjects PRIVATE kritapigment_EXPORTS)
target_include_directories(kritapigmentcmykmathsobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapigmentcmykmathsobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritapigmentcmykmathsobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapigmentmathsobjects OBJECT
    KoColorSpaceMaths.cpp
)
target_compile_definitions(kritapigmentmathsobjects PRIVATE kritapigment_EXPORTS)
target_include_directories(kritapigmentmathsobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapigmentmathsobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritapigmentmathsobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapigmentlabmathsobjects OBJECT
    KoLabColorSpaceMaths.cpp
)
target_compile_definitions(kritapigmentlabmathsobjects PRIVATE kritapigment_EXPORTS)
target_include_directories(kritapigmentlabmathsobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapigmentlabmathsobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritapigmentlabmathsobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritapigment_SRCS
    DebugPigment.cpp
    KoBasicHistogramProducers.cpp
    KoAlphaMaskApplicatorBase.cpp
    KoOptimizedPixelDataScalerU8ToU16Base.cpp
    KoOptimizedPixelDataScalerU8ToU16Factory.cpp
    KoColor.cpp
    KoColorDisplayRendererInterface.cpp
    KoColorConversionAlphaTransformation.cpp
    KoColorConversionCache.cpp
    KoColorConversions.cpp
    KoColorConversionSystem.cpp
    KoColorConversionTransformation.cpp
    KoColorProofingConversionTransformation.cpp
    KoColorConversionTransformationFactory.cpp
    KoColorProfile.cpp
    KoColorSpace.cpp
    KoColorSpaceEngine.cpp
    KoColorSpaceFactory.cpp
    KoColorProfileQuery.h
    KoColorSpaceRegistry.cpp
    KoColorProfileStorage.cpp
    KoColorTransformation.cpp
    KoColorTransformationFactory.cpp
    KoColorTransformationFactoryRegistry.cpp
    KoCompositeColorTransformation.cpp
    KoCompositeOp.cpp
    KoCompositeOpRegistry.cpp
    KoCopyColorConversionTransformation.cpp
    KoFallBackColorTransformation.cpp
    KoHistogramProducer.cpp
    KoMultipleColorConversionTransformation.cpp
    colorspaces/KoLabColorSpace.cpp
    colorspaces/KoRgbU16ColorSpace.cpp
    colorspaces/KoRgbU8ColorSpace.cpp
    colorspaces/KoSimpleColorSpaceFactory.cpp
    colorspaces/KoSimpleColorSpaceEngine.cpp
    compositeops/KoOptimizedCompositeOpFactory.cpp
    compositeops/KoOptimizedCompositeOpFactoryPerArch_Scalar.cpp
    compositeops/KoAlphaDarkenParamsWrapper.cpp
    compositeops/KoColorSpaceBlendingPolicy.cpp
    ${__per_arch_factory_objs}
    ${__per_arch_alpha_applicator_factory_objs}
    ${__per_arch_rgb_scaler_factory_objs}
    KoAlphaMaskApplicatorFactory.cpp
    colorprofiles/KoDummyColorProfile.cpp
    resources/KoAbstractGradient.cpp
    resources/KoColorSet.cpp
    resources/KisSwatch.cpp
    resources/KisSwatchGroup.cpp
    resources/KoPattern.cpp
    resources/KoStopGradient.cpp
    resources/KoSegmentGradient.cpp
    resources/KoCachedGradient.h
    resources/KisGradientConversion.cpp
    resources/KisUniqueColorSet.cpp
)

add_library(kritaalphacolorspaceobjects OBJECT
    colorspaces/KoAlphaColorSpace.cpp
)
target_compile_definitions(kritaalphacolorspaceobjects PRIVATE kritapigment_EXPORTS)
target_include_directories(kritaalphacolorspaceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/colorspaces>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/compositeops>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaalphacolorspaceobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritaalphacolorspaceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC OR (WIN32 AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel"))
  # avoid "cannot open file 'LIBC.lib'" error
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:LIBC.LIB")
endif()

kis_add_library(kritapigment SHARED
    ${kritapigment_SRCS}
    $<TARGET_OBJECTS:kritapigmentmathsobjects>
    $<TARGET_OBJECTS:kritapigmentcmykmathsobjects>
    $<TARGET_OBJECTS:kritapigmentlabmathsobjects>
    $<TARGET_OBJECTS:kritapigmentstandardidsobjects>
    $<TARGET_OBJECTS:kritapigmentcolorimetryobjects>
    $<TARGET_OBJECTS:kritaalphacolorspaceobjects>
)

target_sources(kritapigment
  PRIVATE
    KoColorProfileQuery.h
)

generate_export_header(kritapigment)

target_include_directories( kritapigment
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/resources>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/compositeops>
)

target_link_libraries( kritapigment
    PUBLIC
        kritaplugin
        kritaglobal
        kritaresources
        kritapaintingundo
        ${LINK_OPENEXR_LIB}
        kritamultiarch
        KF${KF_MAJOR}::I18n 
        KF${KF_MAJOR}::ConfigCore
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
    PRIVATE
        kritaresourcestorage
        kritaxmlserialization
)

set_target_properties(kritapigment PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritapigment  ${INSTALL_TARGETS_DEFAULT_ARGS})

if(BUILD_TESTING)
    file(GLOB_RECURSE kritapigment_header_self_containment_headers
        CONFIGURE_DEPENDS
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "*.h"
    )
    list(FILTER kritapigment_header_self_containment_headers EXCLUDE REGEX
        "(^|/)[^/]+_p\\.h$"
    )
    list(FILTER kritapigment_header_self_containment_headers EXCLUDE REGEX
        "(^|/)(3rdparty|benchmarks|tests)/"
    )

    # KoStreamedMath.h selects this version-specific implementation for
    # xsimd < 10. The current dependency profile validates its xsimd >= 10 path.
    list(REMOVE_ITEM kritapigment_header_self_containment_headers
        "compositeops/KoRgbaInterleavers.h"
    )

    set(kritapigment_header_self_containment_sources)
    foreach(header IN LISTS kritapigment_header_self_containment_headers)
        string(REPLACE "/" "__" source_name "${header}")
        set(source_path
            "${CMAKE_CURRENT_BINARY_DIR}/header_self_containment/${source_name}.cpp"
        )
        file(GENERATE OUTPUT "${source_path}" CONTENT "#include \"${header}\"\n")
        list(APPEND kritapigment_header_self_containment_sources "${source_path}")
    endforeach()

    add_library(kritapigment_header_self_containment OBJECT
        ${kritapigment_header_self_containment_sources}
    )
    target_link_libraries(kritapigment_header_self_containment PRIVATE
        kritapigment
    )
    set_target_properties(kritapigment_header_self_containment PROPERTIES
        AUTOMOC OFF
        UNITY_BUILD OFF
    )
endif()
