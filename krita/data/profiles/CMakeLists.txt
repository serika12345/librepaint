########### install files ###############
add_subdirectory(elles-icc-profiles)
add_subdirectory(ycbcr-icc-profiles)

# These three legacy profiles do not have a sufficiently precise, consistent
# file-to-license record for the restricted iOS asset set.  The iOS build keeps
# the explicitly licensed Elle Stone and YCbCr profiles above instead.
if(NOT IOS)
    install(FILES
        README
        scRGB.icm
        cmyk.icm
        krita25_lcms-builtin-sRGB_g100-truegamma.icc
        DESTINATION ${KDE_INSTALL_DATADIR}/color/icc/krita)
endif()
