# Krita の Qt フォークの改変一覧（for-krita/6.11.3）

Issue #108 フェーズQ2 と Issue #68 の作業資料。Krita の Qt フォーク（`dimula73/qtbase` の `for-krita/6.11.3`、先頭 d918f368da）が Qt 正式版 6.11.3 に加えている 92 件の改変を、目的ごとにまとめ、LibrePaint の依存と判定の方法を記録する。判定が決まったら、この表の「判定」と DECISIONS.md を更新する。

「正式版」の列は、改変の件名で qt/qtbase の履歴を検索して見つかったコミットを示す（2026-11-08 時点）。件名が変わって取り込まれたものは見落としている可能性がある。

| まとまり | 件数 | 判定の方針（暫定） |
| --- | --- | --- |
| Windows の ANGLE と OpenGL ES の経路 | 18 | 保留：L1 の H7（ANGLE を使わない OpenGL）の結果で決める。ANGLE を残すなら必要、なくすなら不要 |
| 画面更新の間引き（フレームレート圧縮） | 10 | 代替の候補：LibrePaint は Linux と macOS でこれなしに `KisCanvas2` 側の間引きで動いている。L1 の H6 で Windows の遅れへの寄与を測り、1系統にまとめる |
| HDR と色変換（バックバッファー、QRhi） | 4 | 保留：Windows の HDR 表示を維持するかと合わせて決める。LibrePaint は `setTextureColorSpace`（小さな色選択器）で依存 |
| Windows のペン入力（WinTab、Windows Ink、タッチ） | 14 | 必要の可能性が高い：ペン入力に直結する。外すかどうかは、合成ペン入力と PenProbe の数値（報告レート、取りこぼし）で確かめる |
| Wayland のペン入力とウィンドウ | 10 | 要確認：LibrePaint の Linux 版はこれなしで動いている。正式版に取り込み済みのものは、取り込んだ版で解決 |
| キーボードとショートカット（配列、不均衡な押下と解放） | 12 | 代替の候補：LibrePaint は `KRITA_QT_HAS_UNBALANCED_KEY_PRESS_RELEASE_PATCH` と `KRITA_QT_HAS_ENTER_LEAVE_PATCH` がないとき回避コードで動く（Linux と macOS で既に使用） |
| 不具合とクラッシュの修正（ウィジェット、GUI、コア） | 13 | 要確認：1件ずつ、再現条件が LibrePaint で起きるかを確かめる。正式版に取り込み済みのものは解決済み |
| その他（構築、UI、プラットフォーム固有の調整） | 11 | 個別：`QMdiArea::AlwaysShowSubwindowNameInTitleBar` は LibrePaint が使う（代替が必要）。`QT_GLX_SWAP_INTERVAL_SYNC_WORKAROUND` は Linux の交換の止まりと関係し、Qt 6.11.3 の修正で不要の見込み |

## Windows の ANGLE と OpenGL ES の経路

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| 927cee712e | Fix difinition in QEGLPlatformContext | src/gui/opengl/platform | なし | 未判定 |
| 731ab8bba8 | Fix openGL 2.x support in QOpenGL2PaintEngine | src/opengl/qopenglpaintengine.cpp, src/opengl/qopenglpaintengine_p.h | なし | 未判定 |
| 8ec599f593 | [eglfs] Don't enable QOpenGLCompositor on platforms it is not used | src/opengl/CMakeLists.txt | なし | 未判定 |
| 50d61a44d5 | [xcb] Fix QPlatformIntegration::openGLModuleType() for XCB platform | src/gui/kernel/qplatformintegration.cpp, src/plugins/platforms/xcb | なし | 未判定 |
| f5a65b7761 | [win] Recover ANGLE support on Windows | cmake/3rdparty/extra-cmake-modules/find-modules, cmake/FindGLESv2.cmake, src/gui/CMakeLists.txt ほか | なし | 未判定 |
| 7ad4a8bc17 | QSurfaceFormat: implement color space selection for OpenGL ES | src/gui/kernel/qsurfaceformat.cpp, src/gui/kernel/qsurfaceformat.h, src/gui/opengl/platform ほか | なし | 未判定 |
| 95bb9c74f1 | Deprecate QT_FEATURE_opengl_dynamic | src/gui/CMakeLists.txt, src/gui/configure.cmake, src/gui/kernel/qopenglcontext.cpp ほか | なし | 未判定 |
| 747eca7b69 | Let hellogles3 to manually switch API on Windows | examples/opengl/hellogles3/CMakeLists.txt, examples/opengl/hellogles3/app.manifest, examples/opengl/hellogles3/glwindow.cpp ほか | なし | 未判定 |
| 72a68d87fd | Fix building on platforms without EGL | src/gui/CMakeLists.txt | なし | 未判定 |
| 02216d26ba | [win+angle] Fix flickering when resizing the Window | src/plugins/platforms/windows | なし | 未判定 |
| 1efb3f6434 | [win] Implement callback for ANGLE debugging information | src/plugins/platforms/windows | なし | 未判定 |
| 261c23275e | [win] Implement support for ANGLE's openGL backend | src/gui/kernel/qplatformopenglcontext.h, src/gui/rhi/qrhigles2.cpp, src/gui/rhi/qrhigles2_p.h ほか | なし | 未判定 |
| 14ebe92549 | Remove dynamicgl from mkspecs | mkspecs/features/win32/opengl.prf | なし | 未判定 |
| aced4e6119 | Fix crash on Linux when using wayland plugin | src/gui/opengl/platform | なし | 未判定 |
| 7f50d93a7f | Fix a crash when opening multiple windows in Compatibility openGL profile | src/opengl/qopenglpaintengine.cpp, src/opengl/qopenglpaintengine_p.h | なし | 未判定 |
| 1a543f2832 | Add workaround for flickering on resizing on ANGLE and Qt6 | src/plugins/platforms/windows | なし | 未判定 |
| 77d2b02daf | Fix flickering when opening dialogs on ANGLE and Qt6 | src/gui/painting/qbackingstoredefaultcompositor.cpp, src/gui/painting/qbackingstoredefaultcompositor_p.h, src/gui/painting/qplatformbackingstore.cpp ほか | なし | 未判定 |
| 70086804f9 | Fix EGL format being reset to 0 alpha instead of 2 | src/gui/opengl/platform | なし | 未判定 |

## 画面更新の間引き（フレームレート圧縮）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| e121cb08e5 | Pass proper dirty rect in paint event to QOpenGLWidget with partial updates | src/openglwidgets/qopenglwidget.cpp, src/widgets/kernel/qwidget.cpp, src/widgets/kernel/qwidget_p.h ほか | なし | 未判定 |
| a9b60ead54 | Implement framerate compression in QWidgetRepaintManager | src/corelib/CMakeLists.txt, src/corelib/kernel/qsignalcompressor.cpp, src/corelib/kernel/qsignalcompressor_p.h ほか | なし | 未判定 |
| 7ec0f9c727 | Replace QSignalCompressor with a specialized QFrameRateCompressor | src/corelib/CMakeLists.txt, src/corelib/kernel/qframeratecompressor.cpp, src/corelib/kernel/qframeratecompressor_p.h ほか | なし | 未判定 |
| fb923e4a97 | Add fast-transfer option for QImage when rendering QWidget | src/gui/painting/qbackingstoredefaultcompositor.cpp | なし | 未判定 |
| 6d39b2c286 | Compress frames in a more parallel way | src/gui/rhi/qrhi.cpp, src/gui/rhi/qrhi.h, src/gui/rhi/qrhi_p.h ほか | なし | 未判定 |
| dc3246d166 | Fix possible recursion issue in QFrameRateCompressor | src/corelib/kernel/qframeratecompressor.cpp, src/corelib/kernel/qframeratecompressor_p.h | なし | 未判定 |
| 6e540ae34f | Implement ability to switch FPS compression mode | src/widgets/kernel/qwidgetrepaintmanager.cpp | なし | 未判定 |
| c7d92e97b8 | Implement optional FPS stats printing | src/corelib/CMakeLists.txt, src/corelib/kernel/qmeasureavgportion_p.h, src/gui/rhi/qrhigles2.cpp | なし | 未判定 |
| 71fd8157aa | Add QT_FRAME_RATE_OVERRIDE switch | src/widgets/kernel/qwidgetrepaintmanager.cpp | なし | 未判定 |
| f1cf04e958 | Fix clipping when cutting QOpenGLWidget through the GUI texture | src/widgets/kernel/qwidget.cpp | なし | 未判定 |

## HDR と色変換（バックバッファー、QRhi）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| 752559eb9b | Implement color conversion for the backing store texture | src/gui/CMakeLists.txt, src/gui/painting/qbackingstoredefaultcompositor.cpp, src/gui/painting/qbackingstoredefaultcompositor_p.h ほか | なし | 未判定 |
| c55693d623 | Implement a manual test for checking is HDR features work | examples/opengl/CMakeLists.txt, examples/opengl/hdr-qopenglwidget/CMakeLists.txt, examples/opengl/hdr-qopenglwidget/KisGLImageF16.cpp ほか | なし | 未判定 |
| a1fa6f1629 | Create color conversion pipelines on-demand | src/gui/painting/qbackingstoredefaultcompositor.cpp, src/gui/painting/qbackingstoredefaultcompositor_p.h | なし | 未判定 |
| 5da2f94813 | Implement feature QRhi::DynamicBlendSwitch | src/gui/painting/qbackingstoredefaultcompositor.cpp, src/gui/painting/qbackingstoredefaultcompositor_p.h, src/gui/rhi/qrhi.h ほか | なし | 未判定 |

## Windows のペン入力（WinTab、Windows Ink、タッチ）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| c41d66281e | [win] Generate Enter/Leave events on touching non-client area of the window | src/plugins/platforms/windows | なし | 未判定 |
| df3a678309 | [win] Remove subscription on WM_MOUSELEAVE when a tablet enters the window | src/plugins/platforms/windows | なし | 未判定 |
| 7bb46828ae | Disable tablet relative mode in Qt | src/plugins/platforms/windows | なし | 未判定 |
| 051139ed72 | Fetch mapped screen size from the Wintab driver | src/plugins/platforms/windows | なし | 未判定 |
| 2357bbfe90 | Add a workaround for button mapping on Lenovo Yoga C940 | src/plugins/platforms/windows | なし | 未判定 |
| 33a49f25d1 | Add workaround for handling table press correctly in WinInk mode | src/gui/kernel/qguiapplication.cpp, src/gui/kernel/qwindowsysteminterface.cpp, src/plugins/platforms/windows | なし | 未判定 |
| 7ca0b0e289 | Revert "Query the mouse buttons initially in case the middle button was used" | src/plugins/platforms/windows | なし | 未判定 |
| f5a75c4071 | WinInk: Synthesize mouse events for full pen stroke | src/plugins/platforms/windows | なし | 未判定 |
| 41adadd8db | Synthesize Enter/LeaveEvent for accepted QTabletEvent | src/widgets/kernel/qwidgetwindow.cpp | なし | 未判定 |
| 0027358f20 | Poison Qt's headers with a mark about presence of Enter/Leave patch | src/gui/kernel/qevent.h | なし | 未判定 |
| 8f8eabfb72 | Fix duplicated syntetic mouse events on WinTab | src/plugins/platforms/windows | なし | 未判定 |
| c132390390 | Revert "Send LeaveEvent as reaction to WM_POINTERLEAVE to reset hover states" | src/plugins/platforms/windows | なし | 未判定 |
| a60ed01a25 | [win] Allow explicit accepting of touch events on Windows | src/gui/kernel/qguiapplication.cpp, src/plugins/platforms/windows | なし | 未判定 |
| d918f368da | Windows: implement touchpad gestures using Direct Manipulation | src/gui/configure.cmake, src/gui/kernel/qevent.cpp, src/plugins/platforms/direct2d ほか | なし | 未判定 |

## Wayland のペン入力とウィンドウ

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| ed56d12b50 | Fix unexpected window transparency after enabling CSD on Wayland | src/plugins/platforms/wayland | f1fbd15105（2026-02-27） | 未判定 |
| d2aa04db1a | Fix cursor update when tablet moves between native surfaces | src/plugins/platforms/wayland | なし | 未判定 |
| b069509b06 | Fix cursor update on tablet transition between alien widgets | src/plugins/platforms/wayland | なし | 未判定 |
| 483ceed535 | [wayland] Handle the case when a tablet has multiple tools | src/plugins/platforms/wayland | なし | 未判定 |
| 3b6b57d0b7 | [wayland] Emit native Enter/Leave events on tablet proximity events | src/plugins/platforms/wayland | なし | 未判定 |
| bc9fec0a20 | [wayland] Implement raise() and lower() methods for subsurfaces | src/plugins/platforms/wayland, tests/auto/widgets/kernel | なし | 未判定 |
| 5020e751e6 | [wayland] Make positioners reactive by default | src/plugins/platforms/wayland | なし | 未判定 |
| 435e604932 | wayland: keep a popup in place when it is resized | src/plugins/platforms/wayland, tests/auto/wayland/shared, tests/auto/wayland/xdgshell | c3cc50d89d（2026-09-09） | 未判定 |
| eb6579e6d1 | [wayland] Fix flickering when resizing popup windows | src/plugins/platforms/wayland | なし | 未判定 |
| 693de2c4fb | Wayland client: update cursor position after sending QTabletEvent | src/plugins/platforms/wayland | 4332e5e2c4（2026-07-23） | 未判定 |

## キーボードとショートカット（配列、不均衡な押下と解放）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| 82d2864a8a | Don't eat ShortcutOverride events when there is a partial-match | src/gui/kernel/qshortcutmap_p.h, src/gui/kernel/qwindowsysteminterface.cpp | なし | 未判定 |
| dc27f27d8e | Fix unbalanced KeyPress/Release events in children of QMdiArea | src/widgets/widgets/qmdiarea.cpp | なし | 未判定 |
| 18a07bf888 | Fix shortcuts assigned to special keys on non-latin layouts | src/plugins/platforms/windows | なし | 未判定 |
| 3fab97e30b | Fix multiple shortcuts match from different keyboard layouts | src/gui/kernel/qshortcutmap.cpp | なし | 未判定 |
| adf861033b | Fix fallback latin shortcuts not to be the top priority on Windows | src/plugins/platforms/windows | なし | 未判定 |
| 747423036c | [win] Fix unbalanced KeyPress/Release events when using | src/plugins/platforms/windows | なし | 未判定 |
| f8a61ebc5b | [win] Fin unbalanced shortcuts on non-latin layouts again | src/plugins/platforms/windows | なし | 未判定 |
| 9bfd8a120f | Make sure AltGr-based shortcuts are disallowed when textbox is in focus | src/widgets/widgets/qwidgetlinecontrol.cpp | なし | 未判定 |
| c8049a518f | Fix composing of dead keys with short form of system locale | src/plugins/platforminputcontexts/compose | なし | 未判定 |
| 07a57fa505 | Fix shortcuts with special keys on non-US layouts on Linux | src/gui/platform/unix | なし | 未判定 |
| 21d400db4f | Fix unbalanced KeyPress/Release on non-latin keyboard on Linux | src/plugins/platforms/xcb | なし | 未判定 |
| 35296972ce | Poison Qt's headers with a mark about presence of KeyPress/Release patches | src/gui/kernel/qevent.h | なし | 未判定 |

## 不具合とクラッシュの修正（ウィジェット、GUI、コア）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| 29d93a5d62 | Remove chunk size limits | src/gui/image/qpnghandler.cpp | なし | 未判定 |
| cf05dd04c8 | Check whether there is a QGestureRecognizer at all | src/widgets/kernel/qgesturerecognizer.cpp | なし | 未判定 |
| d2507d05fa | Check if we reached the end of Iterator | src/widgets/kernel/qgesturemanager.cpp | なし | 未判定 |
| 93bd31ff0f | Use DebugBreak instead of fast-fail exception to abort | src/corelib/global/qassert.cpp | なし | 未判定 |
| f1302dbbc8 | Do not crash if the foundry name is an empty space | src/gui/text/qfontdatabase.cpp | なし | 未判定 |
| 8dadf099b8 | Check for uninitialized QDockAreaLayoutInfo | src/widgets/widgets/qdockarealayout.cpp | なし | 未判定 |
| 2ea7817457 | Return 0 for Window if there is no widget | src/widgets/accessible/qaccessiblewidget.cpp | なし | 未判定 |
| bf2ee30931 | QMenu: make less sensitive to mouse jitter | src/widgets/widgets/qmenu.cpp | 4292ad0292（2020-08-19） | 未判定 |
| df46429bb3 | Fix slowdown caused by making window system events synchronous. | src/gui/kernel/qwindowsysteminterface.cpp, src/gui/kernel/qwindowsysteminterface.h | なし | 未判定 |
| 7c155ae659 | Implement better parsing of EDID model number | src/gui/util/qedidparser.cpp | なし | 未判定 |
| 83e4fd6099 | Sync buffers of the destination file after QFile::copy() | src/corelib/io/qfile.cpp | なし | 未判定 |
| d850335b2d | [win] Fix pasting copy-pasting QImages to Chrome | src/plugins/platforms/windows | なし | 未判定 |
| 24ea6b40c2 | Fix QHeaderView failing to init memory mode | src/widgets/itemviews/qheaderview.cpp, tests/auto/widgets/itemviews | e3d05b35b3（2026-10-27） | 未判定 |

## その他（構築、UI、プラットフォーム固有の調整）

| コミット | 件名 | 変更範囲 | 正式版 | 判定 |
| --- | --- | --- | --- | --- |
| a7c925fb66 | [windeployqt] Fix windeployqt to find ICU built with Meson | src/tools/windeployqt/main.cpp | なし | 未判定 |
| 55a0448923 | Use fast path for unsupported mime types | src/plugins/platforms/windows | なし | 未判定 |
| d002731cb6 | [moc] Remove double checks for file-exists in searchIncludePaths() | src/tools/moc/preprocessor.cpp | なし | 未判定 |
| f786da6451 | Add QMdiArea::AlwaysShowSubwindowNameInTitleBar | src/widgets/widgets/qmdiarea.h, src/widgets/widgets/qmdisubwindow.cpp, src/widgets/widgets/qmdisubwindow_p.h | なし | 未判定 |
| d343ee345f | Warn about downgraded precise timers | src/corelib/kernel/qeventdispatcher_win.cpp | cf8cd71476（2026-10-27） | 未判定 |
| 54de8a5b6d | Add icon caching in QFusionStyle | src/widgets/styles/qfusionstyle.cpp | なし | 未判定 |
| 7db708f3ff | Assume widgets are rendered in pass-through mode on MacOS | src/gui/painting/qbackingstoredefaultcompositor.cpp | なし | 未判定 |
| 81a5ac7eb5 | Allow widgets to block kinetic scrolling | src/widgets/util/qflickgesture.cpp, src/widgets/util/qscroller.cpp, src/widgets/util/qscroller.h | なし | 未判定 |
| 5874f1f131 | Implement QT_GLX_SWAP_INTERVAL_SYNC_WORKAROUND | src/plugins/platforms/xcb | なし | 未判定 |
| b2cc4909b0 | [macos] Add bundle-local path to GenericDataLocation | src/corelib/io/qstandardpaths_mac.mm | なし | 未判定 |
| 9dc7c77358 | Add a switch to disable accessibility on Windows | src/plugins/platforms/windows | なし | 未判定 |
