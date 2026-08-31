add_subdirectory( tests )

add_subdirectory( tiles3 )

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
    ${CMAKE_CURRENT_SOURCE_DIR}/brushengine
    ${CMAKE_CURRENT_SOURCE_DIR}/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/commands_new
    ${CMAKE_CURRENT_SOURCE_DIR}/filter
    ${CMAKE_CURRENT_SOURCE_DIR}/floodfill
    ${CMAKE_CURRENT_SOURCE_DIR}/generator
    ${CMAKE_CURRENT_SOURCE_DIR}/layerstyles
    ${CMAKE_CURRENT_SOURCE_DIR}/processing
)

if(FFTW3_FOUND)
  include_directories(${FFTW3_INCLUDE_DIR})
endif()

ko_compile_for_all_implementations_no_scalar(__per_arch_circle_mask_generator_objs kis_brush_mask_applicator_factories.cpp)
ko_compile_for_all_implementations_no_scalar(_per_arch_processor_objs kis_brush_mask_processor_factories.cpp)

message("Following objects are generated from the per-arch lib")
foreach(_obj IN LISTS __per_arch_circle_mask_generator_objs _per_arch_processor_objs)
  message("    * ${_obj}")
endforeach()

add_library(kritaimagefilterstrategyobjects OBJECT
   kis_filter_strategy.cc
)
target_compile_definitions(kritaimagefilterstrategyobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagefilterstrategyobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagefilterstrategyobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimagefilterstrategyobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagebatchnodeupdateobjects OBJECT
   KisBatchNodeUpdate.cpp
)
target_compile_definitions(kritaimagebatchnodeupdateobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagebatchnodeupdateobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagebatchnodeupdateobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
set_target_properties(kritaimagebatchnodeupdateobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageprocessinginformationobjects OBJECT
   kis_processing_information.cpp
)
target_compile_definitions(kritaimageprocessinginformationobjects PRIVATE
   kritaglobal_EXPORTS
   kritaimage_EXPORTS
)
target_include_directories(kritaimageprocessinginformationobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageprocessinginformationobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimageprocessinginformationobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagenodefilterinterfaceobjects OBJECT
   kis_node_filter_interface.cpp
)
target_compile_definitions(kritaimagenodefilterinterfaceobjects PRIVATE
   kritaglobal_EXPORTS
   kritaimage_EXPORTS
)
target_include_directories(kritaimagenodefilterinterfaceobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagenodefilterinterfaceobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagenodefilterinterfaceobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagegradientshapestrategyobjects OBJECT
   kis_gradient_shape_strategy.cpp
)
target_compile_definitions(kritaimagegradientshapestrategyobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagegradientshapestrategyobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaimagegradientshapestrategyobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagegradientshapestrategyobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagekeyframeobjects OBJECT
   kis_keyframe.cpp
)
target_compile_definitions(kritaimagekeyframeobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagekeyframeobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagekeyframeobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagekeyframeobjects PROPERTIES
   AUTOMOC ON
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageconfignotifierobjects OBJECT
   KisImageConfigNotifier.cpp
)
target_compile_definitions(kritaimageconfignotifierobjects PRIVATE
   kritaglobal_EXPORTS
   kritaimage_EXPORTS
)
target_include_directories(kritaimageconfignotifierobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageconfignotifierobjects
   PUBLIC
      kritaglobalsignalcompressorobjects
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
set_target_properties(kritaimageconfignotifierobjects PROPERTIES
   AUTOMOC ON
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagepaintoppresetupdateproxyobjects OBJECT
   brushengine/KisPaintOpPresetUpdateProxy.cpp
)
target_compile_definitions(kritaimagepaintoppresetupdateproxyobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagepaintoppresetupdateproxyobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/brushengine>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagepaintoppresetupdateproxyobjects
   PUBLIC
      kritaglobalsignalcompressorobjects
      Qt${QT_MAJOR_VERSION}::Core
)
set_target_properties(kritaimagepaintoppresetupdateproxyobjects PROPERTIES
   AUTOMOC ON
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageserializableconfigurationobjects OBJECT
   kis_serializable_configuration.cc
)
target_compile_definitions(kritaimageserializableconfigurationobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageserializableconfigurationobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageserializableconfigurationobjects
   PUBLIC
      kritaglobalsharedobjects
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritaimageserializableconfigurationobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagefiltercategoryidsobjects OBJECT
   filter/kis_filter_category_ids.cpp
)
target_compile_definitions(kritaimagefiltercategoryidsobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagefiltercategoryidsobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/filter>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagefiltercategoryidsobjects
   PUBLIC
      kritaglobalidobjects
      Qt${QT_MAJOR_VERSION}::Core
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimagefiltercategoryidsobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageperstrokerandomsourceobjects OBJECT
   brushengine/KisPerStrokeRandomSource.cpp
)
target_compile_definitions(kritaimageperstrokerandomsourceobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageperstrokerandomsourceobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/brushengine>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageperstrokerandomsourceobjects
   PUBLIC
      kritaglobalsharedobjects
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
set_target_properties(kritaimageperstrokerandomsourceobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagestrokerandomsourceobjects OBJECT
   brushengine/kis_stroke_random_source.cpp
)
target_compile_definitions(kritaimagestrokerandomsourceobjects PRIVATE
   kritaglobal_EXPORTS
   kritaimage_EXPORTS
)
target_include_directories(kritaimagestrokerandomsourceobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/brushengine>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagestrokerandomsourceobjects
   PUBLIC
      kritaglobalrandomsourceobjects
      kritaimageperstrokerandomsourceobjects
      kritaglobalsharedobjects
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
set_target_properties(kritaimagestrokerandomsourceobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagestrokespeedmeasurerobjects OBJECT
   brushengine/KisStrokeSpeedMeasurer.cpp
)
target_compile_definitions(kritaimagestrokespeedmeasurerobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagestrokespeedmeasurerobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/brushengine>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagestrokespeedmeasurerobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
)
set_target_properties(kritaimagestrokespeedmeasurerobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagefillintervalmapobjects OBJECT
   floodfill/kis_fill_interval_map.cpp
)
target_compile_definitions(kritaimagefillintervalmapobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagefillintervalmapobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/floodfill>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagefillintervalmapobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
set_target_properties(kritaimagefillintervalmapobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagebusywaitbrokerobjects OBJECT
   KisBusyWaitBroker.cpp
)
target_compile_definitions(kritaimagebusywaitbrokerobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagebusywaitbrokerobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
   PRIVATE
      $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(kritaimagebusywaitbrokerobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Widgets
      Boost::boost
)
set_target_properties(kritaimagebusywaitbrokerobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagecropsavedextradataobjects OBJECT
   kis_crop_saved_extra_data.cpp
)
target_compile_definitions(kritaimagecropsavedextradataobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagecropsavedextradataobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaimagecropsavedextradataobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
set_target_properties(kritaimagecropsavedextradataobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagesafeblockingqueueconnectionproxyobjects OBJECT
   KisSafeBlockingQueueConnectionProxy.cpp
)
target_compile_definitions(kritaimagesafeblockingqueueconnectionproxyobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagesafeblockingqueueconnectionproxyobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagesafeblockingqueueconnectionproxyobjects
   PUBLIC
      kritaimagebusywaitbrokerobjects
      kritaglobalsignalcompressorobjects
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Widgets
      Boost::boost
)
set_target_properties(kritaimagesafeblockingqueueconnectionproxyobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagefastmathobjects OBJECT
   kis_fast_math.cpp
)
target_compile_definitions(kritaimagefastmathobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagefastmathobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaimagefastmathobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagefastmathobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageperspectivemathobjects OBJECT
   kis_perspective_math.cpp
)
target_compile_definitions(kritaimageperspectivemathobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageperspectivemathobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaimageperspectivemathobjects
   PUBLIC
      Eigen3::Eigen
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritaimageperspectivemathobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagesafetransformobjects OBJECT
   kis_safe_transform.cpp
)
target_compile_definitions(kritaimagesafetransformobjects PRIVATE
   kritaglobal_EXPORTS
   kritaimage_EXPORTS
)
target_include_directories(kritaimagesafetransformobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagesafetransformobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      Boost::boost
)
set_target_properties(kritaimagesafetransformobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageoptimizedbrushoutlineobjects OBJECT
   brushengine/KisOptimizedBrushOutline.cpp
)
target_compile_definitions(kritaimageoptimizedbrushoutlineobjects PRIVATE
   kritaglobal_EXPORTS
   kritaimage_EXPORTS
)
target_include_directories(kritaimageoptimizedbrushoutlineobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/brushengine>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageoptimizedbrushoutlineobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      Boost::boost
)
set_target_properties(kritaimageoptimizedbrushoutlineobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagebspline1dobjects OBJECT
   bsplines/kis_bspline_1d.cpp
)
target_compile_definitions(kritaimagebspline1dobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagebspline1dobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsplines>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagebspline1dobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
set_target_properties(kritaimagebspline1dobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageuniformbsplineengineobjects OBJECT
   3rdparty/einspline/bspline_create.cpp
   3rdparty/einspline/bspline_data.cpp
)
target_include_directories(kritaimageuniformbsplineengineobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/einspline>
)
set_target_properties(kritaimageuniformbsplineengineobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagenubspline2dobjects OBJECT
   bsplines/kis_nu_bspline_2d.cpp
)
target_compile_definitions(kritaimagenubspline2dobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagenubspline2dobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsplines>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagenubspline2dobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
set_target_properties(kritaimagenubspline2dobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagenonuniformbsplineengineobjects OBJECT
   3rdparty/einspline/nubasis.cpp
   3rdparty/einspline/nubspline_create.cpp
   3rdparty/einspline/nugrid.cpp
)
target_include_directories(kritaimagenonuniformbsplineengineobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/einspline>
)
set_target_properties(kritaimagenonuniformbsplineengineobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagebspline2dobjects OBJECT
   bsplines/kis_bspline_2d.cpp
)
target_compile_definitions(kritaimagebspline2dobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagebspline2dobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsplines>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagebspline2dobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
set_target_properties(kritaimagebspline2dobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagecachedgradientshapestrategyobjects OBJECT
   kis_cached_gradient_shape_strategy.cpp
)
target_compile_definitions(kritaimagecachedgradientshapestrategyobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagecachedgradientshapestrategyobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bsplines>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagecachedgradientshapestrategyobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      Boost::boost
)
set_target_properties(kritaimagecachedgradientshapestrategyobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagepolygonalgradientshapestrategyobjects OBJECT
   kis_polygonal_gradient_shape_strategy.cpp
)
target_compile_definitions(kritaimagepolygonalgradientshapestrategyobjects PRIVATE
   kritaglobal_EXPORTS
   kritaimage_EXPORTS
)
target_include_directories(kritaimagepolygonalgradientshapestrategyobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagepolygonalgradientshapestrategyobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      Boost::boost
)
if (GSL_FOUND)
   target_link_libraries(kritaimagepolygonalgradientshapestrategyobjects PUBLIC GSL::gsl)
endif()
set_target_properties(kritaimagepolygonalgradientshapestrategyobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagenameserverobjects OBJECT
   kis_name_server.cpp
)
target_compile_definitions(kritaimagenameserverobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagenameserverobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaimagenameserverobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagenameserverobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagenodegraphlistenerobjects OBJECT
   kis_node_graph_listener.cpp
)
target_compile_definitions(kritaimagenodegraphlistenerobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagenodegraphlistenerobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagenodegraphlistenerobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagenodegraphlistenerobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageinterfacesobjects OBJECT
   kis_image_interfaces.cpp
)
target_compile_definitions(kritaimageinterfacesobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageinterfacesobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageinterfacesobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimageinterfacesobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageprojectionupdatesfilterobjects OBJECT
   kis_projection_updates_filter.cpp
)
target_compile_definitions(kritaimageprojectionupdatesfilterobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageprojectionupdatesfilterobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageprojectionupdatesfilterobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimageprojectionupdatesfilterobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagedefaultboundsbaseobjects OBJECT
   kis_default_bounds_base.cpp
)
target_compile_definitions(kritaimagedefaultboundsbaseobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagedefaultboundsbaseobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagedefaultboundsbaseobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagedefaultboundsbaseobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagebaseaccessorobjects OBJECT
   kis_base_accessor.cpp
)
target_compile_definitions(kritaimagebaseaccessorobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagebaseaccessorobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagebaseaccessorobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagebaseaccessorobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagerandomaccessorobjects OBJECT
   kis_random_accessor_ng.cpp
)
target_compile_definitions(kritaimagerandomaccessorobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagerandomaccessorobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagerandomaccessorobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagerandomaccessorobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagerandomsubaccessorobjects OBJECT
   kis_random_sub_accessor.cpp
)
target_compile_definitions(kritaimagerandomsubaccessorobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagerandomsubaccessorobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagerandomsubaccessorobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagerandomsubaccessorobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagepaintoppropertiesbaseobjects OBJECT
   brushengine/KisPaintopPropertiesBase.cpp
)
target_compile_definitions(kritaimagepaintoppropertiesbaseobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagepaintoppropertiesbaseobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagepaintoppropertiesbaseobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagepaintoppropertiesbaseobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageselectioncomponentobjects OBJECT
   kis_selection_component.cc
)
target_compile_definitions(kritaimageselectioncomponentobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageselectioncomponentobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageselectioncomponentobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritaimageselectioncomponentobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageprojectionplaneobjects OBJECT
   kis_abstract_projection_plane.cpp
)
target_compile_definitions(kritaimageprojectionplaneobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageprojectionplaneobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/resources>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment/resources>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/psdutils>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/psdutils>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
)
target_link_libraries(kritaimageprojectionplaneobjects
   PUBLIC
      Imath::Imath
      KF${KF_MAJOR}::I18n
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritaimageprojectionplaneobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagetransformmaskparamsinterfaceobjects OBJECT
   kis_transform_mask_params_interface.cpp
)
target_compile_definitions(kritaimagetransformmaskparamsinterfaceobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagetransformmaskparamsinterfaceobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagetransformmaskparamsinterfaceobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagetransformmaskparamsinterfaceobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageoptimizedbytearrayobjects OBJECT
   KisOptimizedByteArray.cpp
)
target_compile_definitions(kritaimageoptimizedbytearrayobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageoptimizedbytearrayobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageoptimizedbytearrayobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimageoptimizedbytearrayobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagelevelscurveobjects OBJECT
   KisLevelsCurve.cpp
)
target_compile_definitions(kritaimagelevelscurveobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagelevelscurveobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagelevelscurveobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
   PRIVATE
      Qt${QT_MAJOR_VERSION}::Gui
      Qt${QT_MAJOR_VERSION}::Widgets
      Qt${QT_MAJOR_VERSION}::Xml
      KF${KF_MAJOR}::I18n
)
set_target_properties(kritaimagelevelscurveobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagecubiccurveobjects OBJECT
   kis_cubic_curve.cpp
)
target_compile_definitions(kritaimagecubiccurveobjects PRIVATE
   kritaglobal_EXPORTS
   kritaimage_EXPORTS
)
target_include_directories(kritaimagecubiccurveobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
   PRIVATE
      $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
      $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
      $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Xml,INTERFACE_INCLUDE_DIRECTORIES>
      $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(kritaimagecubiccurveobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
      Eigen3::Eigen
)
set_target_properties(kritaimagecubiccurveobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageconvolutionkernelobjects OBJECT
   kis_convolution_kernel.cc
)
target_compile_definitions(kritaimageconvolutionkernelobjects PRIVATE
   kritaglobal_EXPORTS
   kritaimage_EXPORTS
)
target_include_directories(kritaimageconvolutionkernelobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageconvolutionkernelobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      Boost::boost
      Eigen3::Eigen
)
set_target_properties(kritaimageconvolutionkernelobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagespatialcontainerobjects OBJECT
   KisSpatialContainer.cpp
)
target_compile_definitions(kritaimagespatialcontainerobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagespatialcontainerobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
   PRIVATE
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/resources>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment/resources>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaimagespatialcontainerobjects
   PUBLIC
      Boost::boost
      Eigen3::Eigen
      Imath::Imath
      KF${KF_MAJOR}::I18n
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritaimagespatialcontainerobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagestrokestrategyobjects OBJECT
   kis_stroke_strategy.cpp
   KisStrokesQueueMutatedJobInterface.cpp
)
target_compile_definitions(kritaimagestrokestrategyobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagestrokestrategyobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaimagestrokestrategyobjects
   PUBLIC
      KF${KF_MAJOR}::I18n
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
set_target_properties(kritaimagestrokestrategyobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagejobstrategyobjects OBJECT
   kis_stroke_job_strategy.cpp
)
target_compile_definitions(kritaimagejobstrategyobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagejobstrategyobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagejobstrategyobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagejobstrategyobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagestrokejobobjects OBJECT
   KisRunnableStrokeJobDataBase.cpp
   KisRunnableStrokeJobData.cpp
   KisRunnableStrokeJobsInterface.cpp
   KisFakeRunnableStrokeJobsExecutor.cpp
)
target_compile_definitions(kritaimagestrokejobobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagestrokejobobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
   PRIVATE
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagestrokejobobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagestrokejobobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagesimplestrokestrategyobjects OBJECT
   kis_simple_stroke_strategy.cpp
)
target_compile_definitions(kritaimagesimplestrokestrategyobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagesimplestrokestrategyobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaimagesimplestrokestrategyobjects
   PUBLIC
      KF${KF_MAJOR}::I18n
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
set_target_properties(kritaimagesimplestrokestrategyobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagebehaviorinterfaceobjects OBJECT
   KisCroppedOriginalLayerInterface.cpp
   KisDecoratedNodeInterface.cpp
   KisDelayedUpdateNodeInterface.cpp
   KisInterstrokeDataFactory.cpp
   KisTransactionWrapperFactory.cpp
   KisTransformMaskTestingInterface.cpp
)
target_compile_definitions(kritaimagebehaviorinterfaceobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagebehaviorinterfaceobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagebehaviorinterfaceobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagebehaviorinterfaceobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagepaintopsettingsidsobjects OBJECT
   brushengine/KisPaintopSettingsIds.cpp
)
target_compile_definitions(kritaimagepaintopsettingsidsobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagepaintopsettingsidsobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/brushengine>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
set_target_properties(kritaimagepaintopsettingsidsobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagegreencoordinatesobjects OBJECT
   kis_green_coordinates_math.cpp
)
target_compile_definitions(kritaimagegreencoordinatesobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagegreencoordinatesobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
   PRIVATE
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagegreencoordinatesobjects
   PUBLIC Qt${QT_MAJOR_VERSION}::Core
   PRIVATE Qt${QT_MAJOR_VERSION}::Gui Boost::boost
)
set_target_properties(kritaimagegreencoordinatesobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagerandomgenerator2dobjects OBJECT
   KisRandomGenerator2D.cpp
)
target_compile_definitions(kritaimagerandomgenerator2dobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagerandomgenerator2dobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaimagerandomgenerator2dobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagerandomgenerator2dobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagesignalsobjects OBJECT
   KisImageSignals.cpp
)
target_compile_definitions(kritaimagesignalsobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagesignalsobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagesignalsobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagesignalsobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagebackgroundframegenerationlockobjects OBJECT
   KisBlockBackgroundFrameGenerationLock.cpp
)
target_compile_definitions(kritaimagebackgroundframegenerationlockobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagebackgroundframegenerationlockobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagebackgroundframegenerationlockobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagebackgroundframegenerationlockobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageframegenerationlockobjects OBJECT
   KisLockFrameGenerationLock.cpp
)
target_compile_definitions(kritaimageframegenerationlockobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageframegenerationlockobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageframegenerationlockobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimageframegenerationlockobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagecompositeprogressproxyobjects OBJECT
   kis_composite_progress_proxy.cpp
)
target_compile_definitions(kritaimagecompositeprogressproxyobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagecompositeprogressproxyobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
)
target_link_libraries(kritaimagecompositeprogressproxyobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagecompositeprogressproxyobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagenodecommandobjects OBJECT
   commands/kis_node_command.cpp
)
target_compile_definitions(kritaimagenodecommandobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagenodecommandobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaimagenodecommandobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Widgets
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimagenodecommandobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagetransactionbasedcommandobjects OBJECT
   commands_new/kis_transaction_based_command.cpp
)
target_compile_definitions(kritaimagetransactionbasedcommandobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagetransactionbasedcommandobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands_new>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaimagetransactionbasedcommandobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Widgets
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimagetransactionbasedcommandobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageselectionmovecommand2objects OBJECT
   commands_new/kis_selection_move_command2.cpp
)
target_compile_definitions(kritaimageselectionmovecommand2objects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageselectionmovecommand2objects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands_new>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritaimageselectionmovecommand2objects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      Qt${QT_MAJOR_VERSION}::Widgets
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimageselectionmovecommand2objects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagedisabledirtyrequestscommandobjects OBJECT
   commands_new/KisDisableDirtyRequestsCommand.cpp
)
target_compile_definitions(kritaimagedisabledirtyrequestscommandobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagedisabledirtyrequestscommandobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands_new>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaimagedisabledirtyrequestscommandobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Widgets
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimagedisabledirtyrequestscommandobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageupdatecommandexobjects OBJECT
   commands_new/KisUpdateCommandEx.cpp
)
target_compile_definitions(kritaimageupdatecommandexobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageupdatecommandexobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands_new>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaimageupdatecommandexobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Widgets
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimageupdatecommandexobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageupdatecommandobjects OBJECT
   commands_new/kis_update_command.cpp
)
target_compile_definitions(kritaimageupdatecommandobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageupdatecommandobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands_new>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritaimageupdatecommandobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Widgets
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimageupdatecommandobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagelayercollapsecommandobjects OBJECT
   commands_new/KisLayerCollapseCommand.cpp
)
target_compile_definitions(kritaimagelayercollapsecommandobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagelayercollapsecommandobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands_new>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaimagelayercollapsecommandobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Widgets
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimagelayercollapsecommandobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagelayercollapsecommandnodeaccessobjects OBJECT
   commands_new/KisLayerCollapseCommandNodeAccess.cpp
)
target_compile_definitions(kritaimagelayercollapsecommandnodeaccessobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagelayercollapsecommandnodeaccessobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands_new>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritaimagelayercollapsecommandnodeaccessobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimagelayercollapsecommandnodeaccessobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagenoderenamecommandobjects OBJECT
   commands/KisNodeRenameCommand.cpp
   commands_new/KisAsynchronouslyMergeableCommandInterface.cpp
)
target_compile_definitions(kritaimagenoderenamecommandobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagenoderenamecommandobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaimagenoderenamecommandobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Widgets
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimagenoderenamecommandobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagenoderenamecommandnodeaccessobjects OBJECT
   commands/KisNodeRenameCommandNodeAccess.cpp
)
target_compile_definitions(kritaimagenoderenamecommandnodeaccessobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagenoderenamecommandnodeaccessobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritaimagenoderenamecommandnodeaccessobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimagenoderenamecommandnodeaccessobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagenodeuuidinfoobjects OBJECT
   kis_node_uuid_info.cpp
)
target_compile_definitions(kritaimagenodeuuidinfoobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagenodeuuidinfoobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagenodeuuidinfoobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
set_target_properties(kritaimagenodeuuidinfoobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagenodeuuidinfonodeaccessobjects OBJECT
   KisNodeUuidInfoNodeAccess.cpp
)
target_compile_definitions(kritaimagenodeuuidinfonodeaccessobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagenodeuuidinfonodeaccessobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritaimagenodeuuidinfonodeaccessobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimagenodeuuidinfonodeaccessobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageupdateoutlinejobobjects OBJECT
   kis_update_outline_job.cpp
)
target_compile_definitions(kritaimageupdateoutlinejobobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageupdateoutlinejobobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageupdateoutlinejobobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      Boost::boost
)
set_target_properties(kritaimageupdateoutlinejobobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageupdateoutlinejobselectionaccessobjects OBJECT
   KisUpdateOutlineJobSelectionAccess.cpp
)
target_compile_definitions(kritaimageupdateoutlinejobselectionaccessobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageupdateoutlinejobselectionaccessobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritaimageupdateoutlinejobselectionaccessobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimageupdateoutlinejobselectionaccessobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageupdateselectionjobobjects OBJECT
   kis_update_selection_job.cpp
)
target_compile_definitions(kritaimageupdateselectionjobobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageupdateselectionjobobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageupdateselectionjobobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
set_target_properties(kritaimageupdateselectionjobobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageupdateselectionjobselectionaccessobjects OBJECT
   KisUpdateSelectionJobSelectionAccess.cpp
)
target_compile_definitions(kritaimageupdateselectionjobselectionaccessobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageupdateselectionjobselectionaccessobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritaimageupdateselectionjobselectionaccessobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimageupdateselectionjobselectionaccessobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageconvexhullpointsobjects OBJECT
   kis_convex_hull_points.cpp
)
target_compile_definitions(kritaimageconvexhullpointsobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageconvexhullpointsobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageconvexhullpointsobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      Boost::boost
)
set_target_properties(kritaimageconvexhullpointsobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageconvexhulldeviceobjects OBJECT
   kis_convex_hull_device.cpp
)
target_compile_definitions(kritaimageconvexhulldeviceobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageconvexhulldeviceobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageconvexhulldeviceobjects
   PUBLIC
      kritaimageconvexhullpointsobjects
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritaimageconvexhulldeviceobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageconvexhulldevicereaderobjects OBJECT
   kis_convex_hull.cpp
)
target_compile_definitions(kritaimageconvexhulldevicereaderobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageconvexhulldevicereaderobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageconvexhulldevicereaderobjects
   PUBLIC
      kritapigment
      kritaglobal
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritaimageconvexhulldevicereaderobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagecompressionobjects OBJECT
   tiles3/swap/kis_abstract_compression.cpp
   tiles3/swap/kis_lzf_compression.cpp
)
target_compile_definitions(kritaimagecompressionobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagecompressionobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaimagecompressionobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagecompressionobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageabstracttilecompressorobjects OBJECT
   tiles3/swap/kis_abstract_tile_compressor.cpp
)
target_compile_definitions(kritaimageabstracttilecompressorobjects PRIVATE
   kritaglobal_EXPORTS
   kritaimage_EXPORTS
)
target_include_directories(kritaimageabstracttilecompressorobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tiles3>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimageabstracttilecompressorobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
if (TARGET Qt6::Core5Compat)
   target_link_libraries(kritaimageabstracttilecompressorobjects PUBLIC Qt6::Core5Compat)
endif()
set_target_properties(kritaimageabstracttilecompressorobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagelodcapablelayeroffsetobjects OBJECT
   kis_lod_capable_layer_offset.cpp
)
target_compile_definitions(kritaimagelodcapablelayeroffsetobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagelodcapablelayeroffsetobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagelodcapablelayeroffsetobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritaimagelodcapablelayeroffsetobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagetiledextentmanagerobjects OBJECT
   tiles3/KisTiledExtentManager.cpp
)
target_compile_definitions(kritaimagetiledextentmanagerobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagetiledextentmanagerobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tiles3>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagetiledextentmanagerobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Boost::boost
)
if (TARGET Qt6::Core5Compat)
   target_link_libraries(kritaimagetiledextentmanagerobjects PUBLIC Qt6::Core5Compat)
endif()
set_target_properties(kritaimagetiledextentmanagerobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimagememorywindowobjects OBJECT
   tiles3/swap/kis_memory_window.cpp
)
target_compile_definitions(kritaimagememorywindowobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagememorywindowobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritaimagememorywindowobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
)
if (TARGET Qt6::Core5Compat)
   target_link_libraries(kritaimagememorywindowobjects PUBLIC Qt6::Core5Compat)
endif()
set_target_properties(kritaimagememorywindowobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageproofingconfigurationobjects OBJECT
   KisProofingConfiguration.cpp
)
target_compile_definitions(kritaimageproofingconfigurationobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageproofingconfigurationobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
      $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
      $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(kritaimageproofingconfigurationobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Gui
      Boost::boost
)
set_target_properties(kritaimageproofingconfigurationobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

add_library(kritaimageanimsettingcommandobjects OBJECT
   commands_new/KisImageAnimSettingCommand.cpp
)
target_compile_definitions(kritaimageanimsettingcommandobjects PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimageanimsettingcommandobjects
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands_new>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
      $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaimageanimsettingcommandobjects
   PUBLIC
      Qt${QT_MAJOR_VERSION}::Core
      Qt${QT_MAJOR_VERSION}::Widgets
      Qt${QT_MAJOR_VERSION}::Xml
      KF${KF_MAJOR}::I18n
      Boost::boost
)
set_target_properties(kritaimageanimsettingcommandobjects PROPERTIES
   AUTOMOC OFF
   POSITION_INDEPENDENT_CODE ON
)

set(kritaimage_LIB_SRCS
   tiles3/kis_tile.cc
   tiles3/kis_tile_data.cc
   tiles3/kis_tile_data_store.cc
   tiles3/kis_tile_data_pooler.cc
   tiles3/kis_tiled_data_manager.cc
   tiles3/kis_memento_manager.cc
   tiles3/kis_hline_iterator.cpp
   tiles3/kis_vline_iterator.cpp
   tiles3/kis_random_accessor.cc
   tiles3/swap/kis_legacy_tile_compressor.cpp
   tiles3/swap/kis_tile_compressor_2.cpp
   tiles3/swap/kis_chunk_allocator.cpp
   tiles3/swap/kis_swapped_data_store.cpp
   tiles3/swap/kis_tile_data_swapper.cpp
   kis_distance_information.cpp
   kis_painter.cc
   kis_painter_blt_multi_fixed.cpp
   kis_marker_painter.cpp
   KisOverlayPaintDeviceWrapper.cpp
   kis_progress_updater.cpp
   KisAnimatedOpacityProperty.cpp
   brushengine/kis_paint_information.cc
   brushengine/kis_paintop.cc
   brushengine/kis_paintop_factory.cpp
   brushengine/kis_paintop_preset.cpp
   brushengine/kis_paintop_registry.cc
   brushengine/kis_paintop_settings.cpp
   brushengine/kis_paintop_utils.cpp
   brushengine/kis_no_size_paintop_settings.cpp
   brushengine/kis_locked_properties.cc
   brushengine/kis_locked_properties_proxy.cpp
   brushengine/kis_locked_properties_server.cpp
   brushengine/kis_paintop_config_widget.cpp
   brushengine/kis_uniform_paintop_property.cpp
   brushengine/kis_callback_based_paintop_property.cpp
   brushengine/kis_combo_based_paintop_property.cpp
   brushengine/kis_slider_based_paintop_property.cpp
   brushengine/kis_standard_uniform_properties_factory.cpp
   brushengine/kis_paintop_lod_limitations.cpp
   commands/kis_deselect_global_selection_command.cpp
   commands/KisDeselectActiveSelectionCommand.cpp
   commands/kis_image_change_layers_command.cpp
   commands/kis_image_change_visibility_command.cpp
   commands/kis_image_command.cpp
   commands/kis_image_layer_add_command.cpp
   commands/kis_image_layer_move_command.cpp
   commands/kis_image_layer_remove_command.cpp
   commands/kis_image_layer_remove_command_impl.cpp
   commands/kis_node_commands_adapter.cpp
   commands/kis_node_group_operations.cpp
   commands/kis_node_operation_batch.cpp
   commands/kis_node_compositeop_command.cpp
   commands/kis_node_opacity_command.cpp
   commands/kis_node_property_list_command.cpp
   commands/kis_reselect_global_selection_command.cpp
   commands/KisReselectActiveSelectionCommand.cpp
   commands/kis_set_global_selection_command.cpp
   commands/KisChangeDeselectedMaskCommand.cpp
   commands/KisNotifySelectionChangedCommand.cpp
   commands/KisBatchUpdateLayerModificationCommand.cpp
   commands_new/kis_saved_commands.cpp
   commands_new/kis_processing_command.cpp
   commands_new/kis_image_resize_command.cpp
   commands_new/kis_image_set_resolution_command.cpp
   commands_new/kis_node_move_command2.cpp
   commands_new/kis_set_layer_style_command.cpp
   commands_new/kis_switch_current_time_command.cpp
   commands_new/kis_change_projection_color_command.cpp
   commands_new/kis_activate_selection_mask_command.cpp
   commands_new/KisHoldUIUpdatesCommand.cpp
   commands_new/KisChangeChannelFlagsCommand.cpp
   commands_new/KisChangeChannelLockFlagsCommand.cpp
   commands_new/KisMergeLabeledLayersCommand.cpp
   commands_new/KisSimpleModifyTransformMaskCommand.cpp
   commands_new/KisResetGroupLayerCacheCommand.cpp
   commands_new/KisLazyCreateTransformMaskKeyframesCommand.cpp
   commands_new/KisChangeCloneLayersCommand.cpp
   processing/kis_do_nothing_processing_visitor.cpp
   processing/kis_simple_processing_visitor.cpp
   processing/kis_convert_color_space_processing_visitor.cpp
   processing/kis_assign_profile_processing_visitor.cpp
   processing/kis_crop_processing_visitor.cpp
   processing/kis_crop_selections_processing_visitor.cpp
   processing/kis_transform_processing_visitor.cpp
   processing/kis_mirror_processing_visitor.cpp
   processing/KisSelectionBasedProcessingHelper.cpp
   filter/kis_filter.cc
   filter/kis_filter_configuration.cc
   filter/kis_color_transformation_configuration.cc
   filter/kis_filter_registry.cc
   filter/kis_color_transformation_filter.cc
   generator/kis_generator.cpp
   generator/kis_generator_layer.cpp
   generator/kis_generator_registry.cpp
   generator/kis_generator_stroke_strategy.cpp
   floodfill/kis_scanline_fill.cpp
   floodfill/kis_gap_map.cpp
   lazybrush/kis_min_cut_worker.cpp
   lazybrush/kis_lazy_fill_tools.cpp
   lazybrush/kis_multiway_cut.cpp
   lazybrush/KisWatershedWorker.cpp
   lazybrush/kis_colorize_mask.cpp
   lazybrush/kis_colorize_stroke_strategy.cpp
   KisFrameChangeUpdateRecipe.cpp
   KisAnimAutoKey.cpp
   kis_adjustment_layer.cc
   kis_selection_based_layer.cpp
   kis_base_node.cpp
   kis_base_processor.cpp
   kis_bookmarked_configuration_manager.cc
   kis_clone_layer.cpp
   kis_config_widget.cpp
   kis_convolution_painter.cc
   kis_gaussian_kernel.cpp
   kis_edge_detection_kernel.cpp
   KisAutoLevels.cpp
   kis_default_bounds.cpp
   kis_default_bounds_node_wrapper.cpp
   kis_effect_mask.cc
   kis_fill_painter.cc
   kis_filter_mask.cpp
   kis_transform_mask.cpp
   kis_recalculate_transform_mask_job.cpp
   kis_transform_mask_params_factory_registry.cpp
   kis_gradient_painter.cc
   kis_iterator_ng.cpp
   kis_base_rects_walker.cpp
   kis_async_merger.cpp
   kis_merge_walker.cc
   kis_updater_context.cpp
   kis_update_job_item.cpp
   kis_stroke_strategy_undo_command_based.cpp
   KisRunnableBasedStrokeStrategy.cpp
   kis_stroke.cpp
   kis_strokes_queue.cpp
   kis_simple_update_queue.cpp
   kis_update_scheduler.cpp
   kis_queues_progress_updater.cpp
   kis_sync_lod_cache_stroke_strategy.cpp
   kis_update_time_monitor.cpp
   kis_group_layer.cc
   kis_external_layer_iface.cc
   kis_count_visitor.cpp
   kis_histogram.cc
   kis_image_animation_interface.cpp
   kis_time_span.cpp
   kis_image.cc
   kis_image_signal_router.cpp
   kis_image_config.cpp
   KisImageResolutionProxy.cpp
   kis_suspend_projection_updates_stroke_strategy.cpp
   kis_regenerate_frame_stroke_strategy.cpp
   kis_switch_time_stroke_strategy.cpp
   kis_layer.cc
   kis_indirect_painting_support.cpp
   kis_layer_projection_plane.cpp
   kis_layer_utils.cpp
   kis_mask_projection_plane.cpp
   kis_projection_leaf.cpp
   KisSafeNodeProjectionStore.cpp
   kis_mask.cc
   kis_base_mask_generator.cpp
   kis_rect_mask_generator.cpp
   kis_circle_mask_generator.cpp
   kis_gauss_circle_mask_generator.cpp
   kis_gauss_rect_mask_generator.cpp
   ${__per_arch_circle_mask_generator_objs}
   ${_per_arch_processor_objs}
   kis_brush_mask_applicator_factories_Scalar.cpp
   kis_curve_circle_mask_generator.cpp
   kis_curve_rect_mask_generator.cpp
   kis_math_toolbox.cpp
   kis_memory_statistics_server.cpp
   kis_node.cpp
   kis_node_facade.cpp
   kis_node_progress_proxy.cpp
   kis_busy_progress_indicator.cpp
   kis_node_visitor.cpp
   kis_paint_device.cc
   kis_paint_device_debug_utils.cpp
   kis_fixed_paint_device.cpp
   kis_paint_layer.cc
   kis_pixel_selection.cpp
   kis_properties_configuration.cc
   kis_wrapped_random_accessor.cpp
   kis_selection.cc
   KisSelectionUpdateCompressor.cpp
   kis_selection_mask.cpp
   kis_transaction_data.cpp
   KisInterstrokeDataTransactionWrapperFactory.cpp
   KisInterstrokeData.cpp
   kis_transform_worker.cc
   kis_perspectivetransform_worker.cpp
   kis_warptransform_worker.cc
   kis_cage_transform_worker.cpp
   kis_liquify_transform_worker.cpp
   kis_transparency_mask.cc
   kis_undo_adapter.cpp
   kis_macro_based_undo_store.cpp
   kis_surrogate_undo_adapter.cpp
   kis_legacy_undo_adapter.cpp
   kis_post_execution_undo_adapter.cpp
   kis_processing_visitor.cpp
   kis_processing_applicator.cpp
   krita_utils.cpp
   kis_outline_generator.cpp
   kis_layer_composition.cpp
   kis_selection_filters.cpp
   KisProofingConfiguration.h
   KisRecycleProjectionsJob.cpp

   kis_keyframe_channel.cpp
   kis_keyframe_commands.cpp
   kis_scalar_keyframe_channel.cpp
   kis_raster_keyframe_channel.cpp
   kis_onion_skin_compositor.cpp
   kis_onion_skin_cache.cpp
   kis_idle_watcher.cpp

   kis_layer_properties_icons.cpp

   layerstyles/kis_multiple_projection.cpp
   layerstyles/kis_layer_style_filter.cpp
   layerstyles/kis_layer_style_filter_environment.cpp
   layerstyles/kis_layer_style_filter_projection_plane.cpp
   layerstyles/kis_layer_style_projection_plane.cpp
   layerstyles/KisStrokeLayerStyleFilterProjectionPlane.cpp
   layerstyles/kis_ls_drop_shadow_filter.cpp
   layerstyles/kis_ls_satin_filter.cpp
   layerstyles/kis_ls_stroke_filter.cpp
   layerstyles/kis_ls_bevel_emboss_filter.cpp
   layerstyles/kis_ls_overlay_filter.cpp
   layerstyles/kis_ls_utils.cpp
   layerstyles/gimp_bump_map.cpp
   layerstyles/KisLayerStyleKnockoutBlower.cpp

   kis_node_query_path.cc

   KisTranslateLayerNamesVisitor.cpp

   kis_asl_layer_style_serializer.cpp
   KisAslStorage.cpp
   kis_psd_layer_style.cpp
   KisBezierGradientMesh.cpp
   KisBezierTransformMesh.cpp

   KisEncloseAndFillPainter.cpp
)

set(einspline_SRCS
   3rdparty/einspline/multi_bspline_create.cpp
)

kis_add_library(kritaimage SHARED
   ${kritaimage_LIB_SRCS}
   ${einspline_SRCS}
   $<TARGET_OBJECTS:kritaimagefilterstrategyobjects>
   $<TARGET_OBJECTS:kritaimageanimsettingcommandobjects>
   $<TARGET_OBJECTS:kritaimagebatchnodeupdateobjects>
   $<TARGET_OBJECTS:kritaimageprocessinginformationobjects>
   $<TARGET_OBJECTS:kritaimagenodefilterinterfaceobjects>
   $<TARGET_OBJECTS:kritaimagegradientshapestrategyobjects>
   $<TARGET_OBJECTS:kritaimagecachedgradientshapestrategyobjects>
   $<TARGET_OBJECTS:kritaimagepolygonalgradientshapestrategyobjects>
   $<TARGET_OBJECTS:kritaimagekeyframeobjects>
   $<TARGET_OBJECTS:kritaimageconfignotifierobjects>
   $<TARGET_OBJECTS:kritaimagepaintoppresetupdateproxyobjects>
   $<TARGET_OBJECTS:kritaimageserializableconfigurationobjects>
   $<TARGET_OBJECTS:kritaimagefiltercategoryidsobjects>
   $<TARGET_OBJECTS:kritaimageperstrokerandomsourceobjects>
   $<TARGET_OBJECTS:kritaimagestrokerandomsourceobjects>
   $<TARGET_OBJECTS:kritaimagestrokespeedmeasurerobjects>
   $<TARGET_OBJECTS:kritaimagefillintervalmapobjects>
   $<TARGET_OBJECTS:kritaimagebusywaitbrokerobjects>
   $<TARGET_OBJECTS:kritaimagecropsavedextradataobjects>
   $<TARGET_OBJECTS:kritaimagesafeblockingqueueconnectionproxyobjects>
   $<TARGET_OBJECTS:kritaimagefastmathobjects>
   $<TARGET_OBJECTS:kritaimageperspectivemathobjects>
   $<TARGET_OBJECTS:kritaimagesafetransformobjects>
   $<TARGET_OBJECTS:kritaimageoptimizedbrushoutlineobjects>
   $<TARGET_OBJECTS:kritaimagebspline1dobjects>
   $<TARGET_OBJECTS:kritaimageuniformbsplineengineobjects>
   $<TARGET_OBJECTS:kritaimagenubspline2dobjects>
   $<TARGET_OBJECTS:kritaimagenonuniformbsplineengineobjects>
   $<TARGET_OBJECTS:kritaimagebspline2dobjects>
   $<TARGET_OBJECTS:kritaimagenameserverobjects>
   $<TARGET_OBJECTS:kritaimagenodegraphlistenerobjects>
   $<TARGET_OBJECTS:kritaimageinterfacesobjects>
   $<TARGET_OBJECTS:kritaimageprojectionupdatesfilterobjects>
   $<TARGET_OBJECTS:kritaimagedefaultboundsbaseobjects>
   $<TARGET_OBJECTS:kritaimagebaseaccessorobjects>
   $<TARGET_OBJECTS:kritaimagerandomaccessorobjects>
   $<TARGET_OBJECTS:kritaimagerandomsubaccessorobjects>
   $<TARGET_OBJECTS:kritaimagepaintoppropertiesbaseobjects>
   $<TARGET_OBJECTS:kritaimageselectioncomponentobjects>
   $<TARGET_OBJECTS:kritaimageprojectionplaneobjects>
   $<TARGET_OBJECTS:kritaimagetransformmaskparamsinterfaceobjects>
   $<TARGET_OBJECTS:kritaimageoptimizedbytearrayobjects>
   $<TARGET_OBJECTS:kritaimagelevelscurveobjects>
   $<TARGET_OBJECTS:kritaimagecubiccurveobjects>
   $<TARGET_OBJECTS:kritaimageconvolutionkernelobjects>
   $<TARGET_OBJECTS:kritaimagespatialcontainerobjects>
   $<TARGET_OBJECTS:kritaimagestrokestrategyobjects>
   $<TARGET_OBJECTS:kritaimagejobstrategyobjects>
   $<TARGET_OBJECTS:kritaimagestrokejobobjects>
   $<TARGET_OBJECTS:kritaimagesimplestrokestrategyobjects>
   $<TARGET_OBJECTS:kritaimagebehaviorinterfaceobjects>
   $<TARGET_OBJECTS:kritaimagepaintopsettingsidsobjects>
   $<TARGET_OBJECTS:kritaimagegreencoordinatesobjects>
   $<TARGET_OBJECTS:kritaimagerandomgenerator2dobjects>
   $<TARGET_OBJECTS:kritaimagesignalsobjects>
   $<TARGET_OBJECTS:kritaimagebackgroundframegenerationlockobjects>
   $<TARGET_OBJECTS:kritaimageframegenerationlockobjects>
   $<TARGET_OBJECTS:kritaimagecompositeprogressproxyobjects>
   $<TARGET_OBJECTS:kritaimagenodecommandobjects>
   $<TARGET_OBJECTS:kritaimagetransactionbasedcommandobjects>
   $<TARGET_OBJECTS:kritaimageselectionmovecommand2objects>
   $<TARGET_OBJECTS:kritaimagedisabledirtyrequestscommandobjects>
   $<TARGET_OBJECTS:kritaimageupdatecommandexobjects>
   $<TARGET_OBJECTS:kritaimageupdatecommandobjects>
   $<TARGET_OBJECTS:kritaimagelayercollapsecommandobjects>
   $<TARGET_OBJECTS:kritaimagelayercollapsecommandnodeaccessobjects>
   $<TARGET_OBJECTS:kritaimagenoderenamecommandobjects>
   $<TARGET_OBJECTS:kritaimagenoderenamecommandnodeaccessobjects>
   $<TARGET_OBJECTS:kritaimagenodeuuidinfoobjects>
   $<TARGET_OBJECTS:kritaimagenodeuuidinfonodeaccessobjects>
   $<TARGET_OBJECTS:kritaimageupdateoutlinejobobjects>
   $<TARGET_OBJECTS:kritaimageupdateoutlinejobselectionaccessobjects>
   $<TARGET_OBJECTS:kritaimageupdateselectionjobobjects>
   $<TARGET_OBJECTS:kritaimageupdateselectionjobselectionaccessobjects>
   $<TARGET_OBJECTS:kritaimageconvexhullpointsobjects>
   $<TARGET_OBJECTS:kritaimageconvexhulldeviceobjects>
   $<TARGET_OBJECTS:kritaimageconvexhulldevicereaderobjects>
   $<TARGET_OBJECTS:kritaimagecompressionobjects>
   $<TARGET_OBJECTS:kritaimageabstracttilecompressorobjects>
   $<TARGET_OBJECTS:kritaimagelodcapablelayeroffsetobjects>
   $<TARGET_OBJECTS:kritaimagetiledextentmanagerobjects>
   $<TARGET_OBJECTS:kritaimagememorywindowobjects>
   $<TARGET_OBJECTS:kritaimageproofingconfigurationobjects>
)

generate_export_header(kritaimage BASE_NAME kritaimage)

target_link_libraries(kritaimage 
    PUBLIC
        kritaversion 
        kritawidgets 
        kritaglobal 
        kritapsdutils 
         
        kritapigment 
        kritapaintingundo
        kritawidgetutils 
        kritapaintingmetadata
        kritaresources

        Eigen3::Eigen
        Boost::boost
    PRIVATE
        kritaresourcestorage
)

if(HAVE_CXX_ATOMICS_WITH_LIB OR HAVE_CXX_ATOMICS64_WITH_LIB)
   target_link_libraries(kritaimage PUBLIC atomic)
endif()

if(OpenEXR_FOUND)
  target_link_libraries(kritaimage PUBLIC ${LINK_OPENEXR_LIB})
endif()

target_link_libraries(kritaimage PRIVATE ${FFTW3_LIBRARIES})

if(APPLE AND NOT IOS)
    target_link_libraries(kritaimage PRIVATE kritamacosutils)
endif()

target_link_libraries(kritaimage PUBLIC kritamultiarch)

if (NOT GSL_FOUND)
  message(WARNING "No GNU Scientific Library was found; shaped gradients might be non-normalized. Please install GSL.")
else ()
  target_link_libraries(kritaimage PRIVATE GSL::gsl)
endif ()

target_include_directories(kritaimage
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/brushengine>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/filter>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generator>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/layerstyles>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/processing>
)

set_target_properties(kritaimage PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)
install(TARGETS kritaimage  ${INSTALL_TARGETS_DEFAULT_ARGS})
