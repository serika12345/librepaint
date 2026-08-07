if(IOS)
    set (krita_BUNDLE_FILES
         README
         Krita_4_Default_Resources.bundle
    )
else()
    set (krita_BUNDLE_FILES
         README
         Krita_4_Default_Resources.bundle
         Krita_Artists_SeExpr_examples.bundle
         RGBA_brushes.bundle
    )

    if (NOT ANDROID)
        set (krita_BUNDLE_FILES
             ${krita_BUNDLE_FILES}
             Krita_3_Default_Resources.bundle
        )
    endif()
endif()

install( FILES
    ${krita_BUNDLE_FILES}
    DESTINATION  ${KDE_INSTALL_DATADIR}/krita/bundles)
