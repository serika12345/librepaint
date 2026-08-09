set(krita_workspace_files
    Animation.kws
    Big_Paint.kws
    Big_Paint_2.kws
    Big_Vector.kws
    Default.kws
    Small_Vector.kws
    VFX_Paint.kws
    Minimal.kws
)

if(NOT IOS)
    list(APPEND krita_workspace_files Storyboarding.kws)
endif()

install(
    FILES ${krita_workspace_files}
    DESTINATION ${KDE_INSTALL_DATADIR}/krita/workspaces
)
