set(kritaqmlwidgets_SRCS
    KisQQuickWidget.cpp
    KisQQuickPopupWidget.cpp
    kisquickwidgets.qrc

)

kis_add_library(kritaqmlwidgets SHARED ${kritaqmlwidgets_SRCS})
generate_export_header(kritaqmlwidgets)

target_link_libraries(kritaqmlwidgets
    PUBLIC
        Qt::Quick
        Qt::QuickWidgets
    PRIVATE
        kritaglobal
        kritaresources
        KF${KF_MAJOR}::I18n
        Qt::Qml
        Qt::QuickControls2
)

set_target_properties(kritaqmlwidgets PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION}
    SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritaqmlwidgets ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
