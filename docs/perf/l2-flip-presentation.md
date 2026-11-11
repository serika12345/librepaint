# L2（Windows の遅れを直す）の表示経路: 合成を外して flip にする方法

## 0. 目的と範囲

この文書は、L2（Windows の遅れを直す）の項目 A「OpenGL の表示の経路を合成なしにし、ブラシから画面までの待ちを縮める」の調査である。対象の指標は、Surface Pro 7 のホバー輪郭とインクの入力から表示まで（中央値と P95）、アプリの CPU、電力である。

今回は調査と設計案までを扱い、アプリのコードは変えない。Windows の計測は Surface の計測が進行中のため行わない。数値は既存の回の再集計と、公開されている仕様・実装で示す。

用語は次のとおりである。

- flip モデル: DXGI のスワップチェーンの提示方式。バックバッファを DWM と共有し、DWM が追加の写しなしで合成する。
- blt（bitblt）モデル: `Present` のたびにバックバッファを DWM のリダイレクションサーフェスへ写す方式。
- DWM: Windows のデスクトップウィンドウマネージャー。
- Independent Flip: DWM がアプリのスワップチェーンをそのままスキャンアウトする提示。
- Composed: Flip: アプリのバックバッファを DWM が合成する提示。写しはない。
- Composed: Copy with GPU GDI: DWM が GPU の GDI で写して合成する提示。
- 待てるスワップチェーン: `DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT` を持つスワップチェーン。提示の完了を待つハンドルを提供する。

## 1. 既存の計測で分かっていること

同じ Surface、同じ窓（最大化、2736×1824@59）、同じ軌跡で、desktop の OpenGL（H5（ホバーの間引き）を適用）と ANGLE（H1（先積みのフレーム数）=1 と H5）を交互に各 5 回測った回がある。2026-11-10 17:32〜17:43 の `l2-desktop-check` と、2026-11-10 16:37 の `l1-h7-desktop-smoke` である。

| 値 | desktop（WGL） | ANGLE（H1=1 と H5） |
| --- | --- | --- |
| ホバーの入力から表示まで（中央値） | 12.4〜13.6ms | 35.1〜35.5ms |
| インクの入力から表示まで（中央値） | 17.6〜18.6ms | 38.9〜39.3ms |
| PresentMon の表示方式 | Composed: Copy with GPU GDI | Hardware Composed: Independent Flip（91〜95%） |
| `MsUntilDisplayed`（中央値） | 28.18〜28.56ms | 30.18〜30.43ms |
| アプリの CPU（入力区間、1 論理 CPU を 100） | 30.94% | 22.45% |
| GPU busy（`MsGPUBusy`、中央値） | 4.98ms | 6.03ms |
| present の間隔（中央値） | 16.06ms | 16.63ms |
| 表示の変化の間隔（中央値） | 16.68ms | 16.68ms |

この比較から、二つのことが分かる。

第一に、flip を持つ ANGLE の方が、合成を通る desktop より 20ms 以上遅い。flip は遅れの主因ではない。第二に、desktop のアプリ CPU は ANGLE より 37.8% 多い（22.45% から 30.94%。入力区間 32.68 秒の CPU 時間で 7.3 秒から 10.1 秒）。この差は flip の有無だけでは説明できない。CPU の比較は desktop の H5 と ANGLE の H1=1 と H5 の組であり、H1 の有無も含む。PBO と blt の差だけを分けた値ではない。

PresentMon の desktop の行は `PresentRuntime=Other`、`MsInPresentAPI=0` であり、提示の時刻の絶対値は ANGLE と直接比べられない。遅れの比較には取り込みの値（入力から表示まで）を使う。上の表の `MsUntilDisplayed` は参考値である。表示方式の違いは `progress.md` の 2026-11-11 09:40 の節にも記録がある。

## 2. flip の仕組み

### 2.1 DXGI の flip モデル

Microsoft の文書は、blt モデルでは `Present` のたびにバックバッファの内容を DWM のリダイレクションサーフェスへ写し、flip モデルではすべてのバックバッファを DWM と共有するため、DWM が追加の写しなしで合成できると説明している。ウィンドウ表示のアプリでは、この差がメモリー帯域と性能に大きく出る。

https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model

flip のスワップチェーンは `DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL` または `DXGI_SWAP_EFFECT_FLIP_DISCARD` で作り、`BufferCount` を 2〜16、`SampleDesc.Count` を 1（MSAA なし）、形式を `R16G16B16A16_FLOAT`、`B8G8R8A8_UNORM`、`R8G8B8A8_UNORM` のいずれかにする。同じ HWND に複数の flip スワップチェーンを付けず、GDI や blt モデルの提示と混ぜない。

https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model

Independent Flip は、アプリのスワップチェーンが DWM の合成を経ずにスキャンアウトされる提示である。Composed: Flip は DWM がアプリのバックバッファを合成する提示で、写しはない。Composed: Copy with GPU GDI は DWM が GPU の GDI で写す提示である。PresentMon はこの区別を `PresentMode` として記録する。

https://raw.githubusercontent.com/GameTechDev/PresentMon/007f1cf009670e8b90d014324515ed41e5c54422/IntelPresentMon/Interprocess/source/metadata/EnumPresentMode.h

Independent Flip はトップレベルのウィンドウのスワップチェーンで起きる。子ウィンドウのスワップチェーンは DWM が親のウィンドウへ合成するため、Composed: Flip までになる。この区別は、LibrePaint のキャンバスだけを flip にしても Independent Flip には届かないことを意味する。この点は実機で確かめていない。

### 2.2 待てるスワップチェーン

`DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT` を付けて作ったスワップチェーンでは、`IDXGISwapChain2::GetFrameLatencyWaitableObject()` が提示の完了を待つハンドルを返す。アプリは各フレームの描画を始める前にこのハンドルを `WaitForSingleObjectEx` で待つ。最初のフレームも待つ。`IDXGISwapChain2::SetMaximumFrameLatency()` は待ち行列に積むフレーム数を決め、既定は 1 である。

https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_3/nf-dxgi1_3-idxgiswapchain2-getframelatencywaitableobject

https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_3/nf-dxgi1_3-idxgiswapchain2-setmaximumframelatency

この仕組みは、先積みのフレーム数を 1 に抑え、入力から表示までの待ちを短くする。L1 の H1（先積みのフレーム数）で ANGLE に `IDXGIDevice1::SetMaximumFrameLatency(1)` を設定したとき、`present_to_gpu_start` が 15.5〜19.6ms から 1.5〜2.1ms になり、ホバーが 13.2ms、インクが 21.8ms 縮んだ。flip のスワップチェーンは、この設定を ANGLE の外の経路でも使えるようにする。

待てるスワップチェーンの待ちは GUI スレッドで行う。待ちの場所は、描画の開始前のフレームの先頭に置き、入力の処理を待たせない形にする。交換間隔が 0 の flip のスワップチェーンでは、`Present(0, 0)` が待たないとは限らない。Qt の D3D11 の RHI は、対応する環境で `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` と `DXGI_PRESENT_ALLOW_TEARING` を使う。

https://raw.githubusercontent.com/qt/qtbase/6.11/src/gui/rhi/qrhid3d11.cpp

### 2.3 WGL_NV_DX_interop2 で GL から D3D11 のバックバッファに描く

`WGL_NV_DX_interop2` は、OpenGL から Direct3D 10、10.1、11 のリソースへ直接アクセスする拡張である。`WGL_NV_DX_interop` を前提とし、OpenGL 2.1、WDDM、D3D11 に対応する GPU を要求する。

https://registry.khronos.org/OpenGL/extensions/NV/WGL_NV_DX_interop.txt

https://registry.khronos.org/OpenGL/extensions/NV/WGL_NV_DX_interop2.txt

手順は次のとおりである。

1. アプリの HWND に対して D3D11 のデバイスと flip のスワップチェーンを作る。
2. `IDXGISwapChain::GetBuffer(0, ...)` でバックバッファの `ID3D11Texture2D` を取る。flip のバックバッファは `D3D11_USAGE_DEFAULT` であり、interop の条件を満たす。
3. `wglDXOpenDeviceNV(ID3D11Device)` で GL と D3D11 の interop デバイスを開く。
4. `wglDXRegisterObjectNV(..., backbuffer, glTexture, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV)` でバックバッファを GL のテクスチャとして登録し、そのテクスチャを FBO の色の添付にする。
5. 各フレームで、待てるスワップチェーンのハンドルを待ち、`wglDXLockObjectsNV` を呼び、GL で FBO に描き、`wglDXUnlockObjectsNV` を呼び、`IDXGISwapChain::Present` を呼ぶ。
6. リサイズでバックバッファが変わったら、登録を外して作り直す。

拡張の仕様は、GL と Direct3D が同じリソースへ同時に描くことを認めていない。lock と unlock が同期点であり、lock の間は Direct3D からリソースへアクセスできない。GL の描画が終わって unlock した後に D3D11 が提示する。仕様のサンプルコードも、D3D11 のバックバッファに GL で描いてから D3D11 で提示する形を示している。

https://registry.khronos.org/OpenGL/extensions/NV/WGL_NV_DX_interop2.txt

`wglDXOpenDeviceNV` は、GL からアクセスできないディスプレイアダプターを参照する D3D デバイスを受け取ると `ERROR_NOT_SUPPORTED` を返す。GL のコンテキストと D3D11 のデバイスが同じアダプターにあることが条件である。

https://registry.khronos.org/OpenGL/extensions/NV/WGL_NV_DX_interop.txt

### 2.4 ANGLE の提示経路

LibrePaint の ANGLE は、Qt の Windows の EGL プラグインが `EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE` で作る。ANGLE の `NativeWindow11Win32` は、`CreateSwapChainForHwnd` の経路では `BufferCount=1`、`DXGI_SWAP_EFFECT_SEQUENTIAL` のスワップチェーンを作り、DirectComposition の経路では `BufferCount=2`、`DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL` のスワップチェーンを `CreateSwapChainForComposition` で作る。どちらの経路も待てるスワップチェーンを使わない。

https://raw.githubusercontent.com/google/angle/chromium/5005/src/libANGLE/renderer/d3d/d3d11/win32/NativeWindow11Win32.cpp

https://raw.githubusercontent.com/google/angle/chromium/5005/src/libANGLE/renderer/d3d/d3d11/Renderer11.cpp

LibrePaint の PresentMon の記録では ANGLE が Hardware Composed: Independent Flip になる。Qt の EGL プラグインは `EGL_DIRECT_COMPOSITION_ANGLE` を設定していないため、この flip がどこで作られているかは、実機の EGL の属性とスワップチェーンの作成を記録して確かめる必要がある。この文書では、観測された表示方式を事実として扱い、ANGLE の内部の経路の断定はしない。

## 3. 採用例

| ソフト | 提示の方式 | GL と D3D の関係 | 出典 |
| --- | --- | --- | --- |
| Chrome | DXGI の flip スワップチェーン。`DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT` と `SetMaximumFrameLatency` を使う。 | GL は ANGLE（D3D11）で描き、提示はコンポジターの D3D11 のスワップチェーンが行う。WGL interop は使わない。 | https://raw.githubusercontent.com/chromium/chromium/main/gpu/command_buffer/service/shared_image/dxgi_swap_chain_image_backing.cc |
| mpv | `--gpu-context=dxinterop` は D3D9Ex の `D3DSWAPEFFECT_FLIPEX` のスワップチェーンに GL で描く。`--gpu-context=d3d11` は D3D11 の flip スワップチェーンと `IDXGIDevice1::SetMaximumFrameLatency` を使う。 | dxinterop は `WGL_NV_DX_interop` を使う。D3D11 の経路は GL を使わない。 | https://raw.githubusercontent.com/mpv-player/mpv/master/video/out/opengl/context_dxinterop.c と https://raw.githubusercontent.com/mpv-player/mpv/master/video/out/gpu/d3d11_helpers.c |
| Firefox | `CompositorD3D11` が `BufferCount=2`、`DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL` のスワップチェーンを作る。このスワップチェーンは待てるスワップチェーンを使わない。 | 描画は D3D11。GL は使わない。 | https://raw.githubusercontent.com/mozilla/gecko-dev/master/gfx/layers/d3d11/CompositorD3D11.cpp |
| Blender | OpenXR の D3D11 のレンダーターゲットに GL で描く。`WGL_NV_DX_interop2` を要求し、`wglDXOpenDeviceNV`、`wglDXRegisterObjectNV`、`wglDXLockObjectsNV`、`wglDXUnlockObjectsNV` を使う。 | GL の描画を D3D11 のテクスチャへ渡す。メインウィンドウの提示は WGL のままである。 | https://raw.githubusercontent.com/blender/blender/main/intern/ghost/intern/GHOST_ContextD3D.cc |
| OBS | D3D11 のレンダラーが `DXGI_SWAP_EFFECT_FLIP_DISCARD` と `DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT` を使う。 | GL のキャプチャのフックは `wglDXOpenDeviceNV` と `wglDXRegisterObjectNV` をテクスチャの共有に使う。提示は D3D11 が行う。 | https://raw.githubusercontent.com/obsproject/obs-studio/master/libobs-d3d11/d3d11-subsystem.cpp と https://raw.githubusercontent.com/obsproject/obs-studio/master/plugins/win-capture/graphics-hook/gl-capture.c |

この表から、次の傾向が分かる。主要なソフトは、提示を D3D11 の flip スワップチェーンで行い、GL は ANGLE か、テクスチャの共有（WGL interop）でつなぐ。WGL interop をメインウィンドウの提示に使う例は少ない。mpv の dxinterop は D3D9Ex の flip であり、Blender と OBS の WGL interop はテクスチャの共有である。LibrePaint で WGL interop を提示に使う案は、技術的には可能であるが、採用例の多い形ではない。

## 4. Intel のドライバーの対応

Surface Pro 7 の GPU は Intel Iris Plus Graphics、ドライバーは 31.0.101.2130（2024-08-13）である。Intel の資料は、Iris Plus Graphics が DirectX 12 と OpenGL 4.6 に対応するとしている。

https://www.intel.com/content/www/us/en/support/articles/000005524/graphics.html

`WGL_NV_DX_interop2` の可否は、ドライバーが公開する WGL の拡張一覧で決まる。仕様は NVIDIA が策定したが、拡張の公開は各ドライバーに依存する。仕様の条件は WDDM と D3D11 に対応する GPU であり、Surface の Intel の構成はこの条件を満たす。GL のコンテキストと D3D11 のデバイスが同じアダプターにあることも条件である。Surface は Intel の 1 アダプター構成なので、この条件も満たす。Intel の OpenGL 4.6 のドライバは WGL の拡張を公開するが、`WGL_NV_DX_interop2` を含むかは世代とドライバーによる。

https://registry.khronos.org/OpenGL/extensions/NV/WGL_NV_DX_interop.txt

拡張の有無は実機で読む必要がある。WGL のコンテキストを作り、`wglGetExtensionsStringARB` の文字列に `WGL_NV_DX_interop2` があるかを確かめる。確認の形は次のとおりである。

```c
const char *ext = wglGetExtensionsStringARB(hdc);
bool hasInterop2 = ext && strstr(ext, "WGL_NV_DX_interop2") != nullptr;
```

この確認は Surface の計測が進行中のため、今回は行っていない。拡張がない場合は、上策の WGL interop は成立しない。中策の QRhi の D3D11 は、この拡張に依存しない。

## 5. Qt 6 の QRhi と QOpenGLWidget の関係

`QOpenGLWidget` は、トップレベルウィンドウの RHI のバックエンドが OpenGL であることを前提にする。バックエンドが D3D11 などの別の API のとき、Qt は「The top-level window is not using OpenGL for composition, ... is not compatible with QOpenGLWidget」という警告を出し、描画を成立させない。`QOpenGLWidget` はウィジェットの FBO のテクスチャを OpenGL の RHI のテクスチャとして包み、ウィンドウのバックエンドストアで合成する。

https://raw.githubusercontent.com/qt/qtbase/6.11/src/openglwidgets/qopenglwidget.cpp

LibrePaint の `krita/main.cc` は、Windows で `QT_WIDGETS_RHI=1`、`QT_WIDGETS_RHI_BACKEND=opengl`、`QSG_RHI_BACKEND=opengl` を設定する。この設定は、`KisOpenGLCanvas2` と `KisGLImageWidget` の 2 つの `QOpenGLWidget` を動かすために必要である。

`QRhiWidget` は `QOpenGLWidget` の移植可能な代替であり、Windows では既定で D3D11 を使う。`QRhiWidget` はウィジェットのバックグラウンドのテクスチャに描き、ウィンドウの RHI がそのテクスチャを合成する。1 つのウィンドウは 1 つの RHI のバックエンドだけを使うため、同じウィンドウに `QOpenGLWidget` と `QRhiWidget` を混在させられない。

https://doc.qt.io/qt-6/qrhiwidget.html

https://raw.githubusercontent.com/qt/qtbase/6.11/src/widgets/kernel/qrhiwidget.cpp

Qt の D3D11 の RHI は、スワップチェーンを `DXGI_SWAP_EFFECT_FLIP_DISCARD`、`BufferCount=2` で作る。`QT_D3D_MAX_FRAME_LATENCY` を設定し、交換間隔が 0 でないとき、`DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT` と `SetMaximumFrameLatency` を使う。交換間隔が 0 のときは待てるスワップチェーンを使わないため、中策では交換間隔を 1 にする。`QT_D3D_NO_FLIP` を設定したときだけ、従来の `DXGI_SWAP_EFFECT_DISCARD` に戻る。HDR の形式（`R16G16B16A16_FLOAT`、`R10G10B10A2_UNORM`）と色空間にも対応する。

https://raw.githubusercontent.com/qt/qtbase/6.11/src/gui/rhi/qrhid3d11.cpp

D3D12 の RHI も同じ形である。`DXGI_SWAP_EFFECT_FLIP_DISCARD`、`BufferCount=2`、`QT_D3D_MAX_FRAME_LATENCY` による待てるスワップチェーンと `SetMaximumFrameLatency` を持つ。

https://raw.githubusercontent.com/qt/qtbase/6.11/src/gui/rhi/qrhid3d12.cpp

Qt の OpenGL の RHI は、`QRhiGles2::endFrame` で `QOpenGLContext::swapBuffers(surface)` を呼び、ウィンドウの GL のサーフェスを提示する。この経路には DXGI の flip のスワップチェーンがない。

https://raw.githubusercontent.com/qt/qtbase/6.11/src/gui/rhi/qrhigles2.cpp

## 6. LibrePaint の変更点

### 6.1 現在の構成

| ファイルと関数 | 役割 | flip にするときの関係 |
| --- | --- | --- |
| `libs/ui/opengl/kis_opengl_canvas2.h` の `KisOpenGLCanvas2 : public QOpenGLWidget` | キャンバスのウィジェット。`initializeGL`、`resizeGL`、`paintGL`、`paintEvent` を持つ。 | ウィジェットの FBO に描き、提示は Qt のウィンドウの RHI が行う。キャンバスだけでは提示を変えられない。 |
| `libs/ui/opengl/kis_opengl_canvas2.cpp` の `KisOpenGLCanvas2::initializeGL()` | GL の初期化、`KisOpenGLSync::init(context())`、H1 の設定。 | interop のデバイスとスワップチェーンを作る場所の候補。 |
| `KisOpenGLCanvas2::resizeGL(int, int)` | GL の大きさの変更。 | flip のバックバッファの再作成の候補。 |
| `KisOpenGLCanvas2::paintGL()` | `KisOpenGLCanvasRenderer` でキャンバスを描く。`paintgl_begin` と `paintgl_end` の印を出す。 | waitable を待つ位置、interop の lock と unlock、`Present` の位置の候補。 |
| `KisOpenGLCanvas2::paintEvent(QPaintEvent *)` | 更新の間引きと `QOpenGLWidget::paintEvent` の呼び出し。 | flip の提示とは独立である。 |
| `KisOpenGLCanvas2::updateCanvasImage(const QRect &)` と `updateCanvasDecorations(const QRect &)` | `QWidget::update` で更新を要求する。 | 更新の入口は変わらない。 |
| `KisOpenGLCanvas2::isBusy() const` | `KisOpenGLSync` のフェンスで前のフレームの完了を見る。 | waitable の待ちに置き換えるか、併用する。 |
| `libs/ui/canvas/kis_canvas2.cpp` の `KisCanvas2::createCanvas(bool)` | `KisOpenGLCanvas2` か `KisQPainterCanvas` を選ぶ。 | 新しい flip のキャンバスを選ぶ入口。 |
| `libs/ui/opengl/kis_opengl.cpp` の `generateSurfaceConfig()` | `format.setSwapInterval(0)` を設定する。 | flip のスワップチェーンの交換間隔と提示の設定。 |
| `libs/ui/opengl/kis_opengl.cpp` の `selectSurfaceConfig()` と `setDefaultSurfaceConfig()` | renderer と形式の選択。 | 上策では Qt の RHI の設定が中心になり、この選択は補助になる。 |
| `krita/main.cc` の `QT_WIDGETS_RHI` と `QT_WIDGETS_RHI_BACKEND` | ウィンドウの RHI を OpenGL にする。 | 中策では D3D11 に変える。上策では OpenGL のまま。 |
| `plugins/dockers/smallcolorselector/KisGLImageWidget.{h,cpp}` | 2 つ目の `QOpenGLWidget`。 | 中策では QRhiWidget へ移す必要がある。 |

### 6.2 上策の変更点

上策は、トップレベルウィンドウの GL の提示を、WGL interop を使った D3D11 の flip のスワップチェーンに置き換える。変更の中心は Qt の OpenGL の RHI と Windows のプラットフォーム層であり、LibrePaint のキャンバスの関数ではない。

- Qt の `src/gui/rhi/qrhigles2.cpp` の `QRhiGles2::endFrame` が呼ぶ `QOpenGLContext::swapBuffers(surface)` を、D3D11 の flip のスワップチェーンの提示に置き換える。GL の描画は、`wglDXRegisterObjectNV` で登録したバックバッファを色の添付にした FBO に対して行う。
- Windows の GL のウィンドウサーフェスの作成（`src/plugins/platforms/windows/qwindowsglcontext.cpp`）で、同じ HWND の D3D11 のデバイスと flip のスワップチェーンを作り、interop のデバイスを開く。
- 各フレームの前に `GetFrameLatencyWaitableObject` を待ち、GL の描画の後に `wglDXUnlockObjectsNV` と `Present` を呼ぶ。`SetMaximumFrameLatency(1)` を設定する。
- LibrePaint の `KisOpenGLCanvas2` は、`QOpenGLWidget` の FBO に描く今の形のままでも、Qt の RHI がそのテクスチャを flip のバックバッファへ合成する形でもよい。Qt の RHI がウィンドウ全体を 1 枚のテクスチャに描き、そのテクスチャを flip のバックバッファとして提示する構成では、キャンバスの FBO から RHI のテクスチャへの合成が 1 回残る。この 1 回を残すか、キャンバスの FBO を直接バックバッファにするかは、Qt の RHI の構成で決める。Qt の RHI のスワップチェーンを差し替えるため、アプリのコードだけでは完結しない。
- `KisOpenGLCanvas2::paintGL()` の `KisOpenGLSync` のフェンスは、waitable の待ちと重複する。上策では、waitable を主にし、GL のフェンスは描画の完了の確認に残す。

### 6.3 中策の変更点

中策は、メインウィンドウの RHI を D3D11 にし、キャンバスを QRhiWidget（D3D11）にする。変更の中心は LibrePaint のアプリコードである。

- `krita/main.cc` の `QT_WIDGETS_RHI_BACKEND` を `d3d11` にする。`QT_D3D_MAX_FRAME_LATENCY=1` を設定し、交換間隔を 1 にして待てるスワップチェーンを使う。
- `libs/ui/canvas/kis_canvas2.cpp` の `KisCanvas2::createCanvas()` で、`KisOpenGLCanvas2` の代わりに新しい `KisRhiCanvas`（`QRhiWidget`）を選ぶ。
- 新しいキャンバスは、`initialize()` で QRhi のリソースを作り、`render()` で `KisOpenGLCanvasRenderer` に相当する描画を QRhi のコマンドバッファで行う。`KisOpenGLCanvasRenderer`、`KisOpenGLImageTextures`、`KisOpenGLShaderLoader`、`kis_texture_tile` の GL の資源とシェーダーを QRhi の資源と HLSL（または QShader）へ移す。
- `plugins/dockers/smallcolorselector/KisGLImageWidget.{h,cpp}` を QRhiWidget へ移す。同じウィンドウに `QOpenGLWidget` を残せないためである。
- `libs/ui/opengl/KisOpenGLStartupCheck` と `KisOpenGL::selectSurfaceConfig()` の GL の選択は、キャンバスの描画には使わなくなる。Windows のペン入力のパッチと HDR の扱いは別に保つ。

### 6.4 下策の変更点

下策は、表示経路を変えない。`KisOpenGLCanvas2` と Qt の OpenGL の RHI はそのままにし、ANGLE の Independent Flip と、既に実装した H1（先積みのフレーム数）=1 と H5（ホバーの間引き）を使う。desktop の CPU の差は、`LIBREPAINT_L1_TEXTURE_BUFFERS`（PBO の有無）、`LIBREPAINT_L1_SWAP_INTERVAL`、`thread_cpu.ps1` のスレッドの内訳で別に調べる。

## 7. 三つの案の比較

### 7.1 遅れの見込み

1 フレームは 16.7ms である。flip で変わる部分と変わらない部分を分ける。

| 遅れの要素 | 今の desktop（WGL） | flip にしたとき | 1 フレーム 16.7ms のうち減る量 |
| --- | --- | --- | --- |
| DWM の GPU GDI の写し | あり（キャンバス領域で約 7MB の表面。読み書きで約 15MB） | なし | GPU 時間は 1ms 未満の見込み。位相が合わないときに最大 1 フレーム。実測の desktop は写しがあっても ANGLE より速いため、0〜2ms の見込み。 |
| DWM の合成の待ち（表示の周期への量子化） | あり | あり（Independent Flip でも残る） | 0ms。 |
| present の待ち行列（先積み） | あり（`present_to_gpu_start` は 15.9〜19.6ms の回がある） | 待てるスワップチェーンと `SetMaximumFrameLatency(1)` で 1 に抑える | 最大 16.7ms。ANGLE の H1=1 の結果（15.5〜19.6 → 1.5〜2.1ms）が同じ大きさの候補。取り込みの値に現れる量は計測で決まる。 |
| アプリの描画（`paintGL`、ブラシ、輪郭） | 0.3〜0.5ms 程度 | 同じ | 0ms。 |
| 入力の待ち（GUI スレッドの空き待ち） | H5 で削除済み | 同じ | 0ms。 |
| 表示の周期の位相 | 16.68ms の周期に量子化 | 同じ | 0ms。 |

上策と中策の遅れの見込みは、どちらも写しの削除（0〜2ms）と待ち行列の削除（0〜16.7ms）の合計で、0〜16.7ms、見込み 4〜12ms である。下策は 0ms である。

この見込みの不確かさは大きい。desktop の PresentMon は `PresentRuntime=Other` であり、`present_to_gpu_start` の 15.9〜19.6ms が実際の待ち行列を表すかは確定していない。desktop の取り込みのインクは 17.6〜18.6ms であり、待ち行列の全量が遅れとして現れる形にはなっていない。最初に小さな実験で待てるスワップチェーンの効果だけを測る必要がある。

### 7.2 CPU と電力

| 案 | アプリの CPU | 電力 | 根拠と不確かさ |
| --- | --- | --- | --- |
| 上策 | DWM の写しの提出の分だけ下がる可能性。waitable で描画を抑える分も加わる。0〜5% の削減見込み。 | 写しの削除で 1〜5% の削減見込み。 | 条件 6 のアプリ CPU +37.8% は、DWM の写しではなく、PBO と直接転送の差かドライバーの blt の可能性が高い。flip だけでは +37.8% は解消しない見込み。電力は未計測。 |
| 中策 | QRhi と HLSL の描画の効率次第。GL のドライバーの blt が消える分は下がる。 | 写しの削除で 1〜5% の削減見込み。 | レンダラーの移植のできによって上下する。HDR と画素の一致の確認が要る。電力は未計測。 |
| 下策 | 現状の +37.8% が残る。PBO の切り替えで下がる可能性を別に測る。 | 現状のまま。 | `l2-cpu-cause` の結果を先に読む。 |

### 7.3 コード量

| 案 | コード量 | 内容 |
| --- | --- | --- |
| 上策 | 900〜1,500 行（Qt のパッチ）と、LibrePaint 側の設定の変更 | `qrhigles2.cpp` のスワップチェーンと提示、Windows の GL のウィンドウサーフェス、interop の資源の管理、fallback、テスト。Qt の更新のたびに再ベースする。 |
| 中策 | 数千行（数週間） | キャンバスのレンダラー、テクスチャの転送、シェーダー、HDR、カラーセレクタ、部分更新、テスト。 |
| 下策 | 0 行 | 表示経路を変えない。CPU の原因の計測だけを進める。 |

### 7.4 Q4（フォークをやめ、正式版の Qt にする）との関係

| 案 | Q4 との関係 |
| --- | --- |
| 上策 | Qt のフォークのパッチになる。Q4 の「正式版の同じ Qt」と衝突する。Qt 本家に取り込まれなければ、Q4 の後に維持できない。ANGLE を外せることと、HDR を DXGI の形式で残せることは利点である。 |
| 中策 | 正式版 Qt の D3D11 の RHI を使うため、Q4 と両立する。ANGLE と GL のフォークのパッチを外せる。Windows のペン入力の 9 件のパッチは別に必要である。レンダラーの移植が Q4 の完了条件（主指標が悪化しない）を満たすかを確かめる必要がある。 |
| 下策 | 現状の ANGLE とフォークが残る。Q4 の判断は変わらない。Q4 の後に desktop の GL だけになると、DWM の写しは残り、flip の課題も残る。 |

## 8. 見込みの根拠と不確かさ

- flip の効果は、写しの削除（0〜2ms）と待ち行列の削除（0〜16.7ms）に限られる。後者は flip そのものではなく、待てるスワップチェーンと `SetMaximumFrameLatency(1)` の効果である。
- 既存の測定では、flip を持つ ANGLE が desktop より 20ms 以上遅い。flip は遅れの主因ではない。desktop の写しは、取り込みの値では大きな遅れとして現れていない。
- 条件 6 のアプリ CPU +37.8% は、flip では解消しない見込みである。CPU の原因の計測（`l2-cpu-cause`、PBO、交換間隔、スレッドの内訳）の結果を先に読む。
- 電力は未計測である。写しの削除の効果は数 % の見込みで、10% の条件を満たすかは不明である。
- `WGL_NV_DX_interop2` の Intel での可否は、Surface の拡張一覧で確かめる。無ければ上策は成立しない。
- 子ウィンドウの flip が PresentMon で Composed: Flip になり、Independent Flip にならないことは、DWM の合成の仕組みからの推論であり、実機では未確認である。Independent Flip にはトップレベルウィンドウの flip が必要である。
- Windows の計測は Surface の計測が進行中のため行っていない。

## 9. 計測の計画

1. 待てるスワップチェーンの効果を、LibrePaint とは別の小さな Qt の D3D11 の QRhiWidget の試験アプリで測る。Surface で同じ合成入力と取り込みを使い、`QT_D3D_MAX_FRAME_LATENCY` の 0 と 1、交換間隔の 0 と 1 を比べる。flip の有無と待ち行列の効果を分ける。
2. WGL interop の可否を Surface で確かめる。WGL の拡張一覧に `WGL_NV_DX_interop2` があること、D3D11 のデバイスを開けること、flip のバックバッファを登録できること、lock と unlock と `Present` の往復が動くことを 1 回確かめる。
3. 上策を試す場合は、Qt のパッチを当てた版を作り、`l2-flip` のラベルで desktop の H5 と交互に各 5 回測る。`PresentMode` が Hardware Composed: Independent Flip になること、取り込みのホバーとインク、`present_to_display`、CPU、電力を測る。
4. 中策を試す場合は、QRhiWidget の試作をキャンバスの 1 経路として作り、同じ計測を行う。画素の一致（8bit で 1 階調以内）、HDR、部分更新、HDR の表示を先に確かめる。
5. 合格の目安は、取り込みのホバーとインクの中央値が desktop の現状（12.4〜13.6ms と 17.6〜18.6ms）から 2ms 以上改善すること、または P95 が 5% 以上改善することである。CPU はアプリのプロセスで +10% を超えない。電力は +10% を超えない。回ごとの中央値のばらつきは 5% 未満にする。

## 10. 判定してほしいこと

- A: 上策を試す。Qt の OpenGL の RHI に WGL interop の flip を入れ、Qt のパッチとして維持する。Q4 との衝突を許容する。
- B: 中策を Q4 の一部として進める。QRhiWidget の D3D11 へキャンバスとカラーセレクタを移す。数週間を見込む。
- C: 下策を採り、表示経路は変えない。CPU の原因の計測を先に進め、flip は Q4 の後に再検討する。

推奨は C である。既存の測定では flip の遅れの効果が確認できず、条件 6 の CPU の +37.8% は flip では解消しない見込みである。`l2-cpu-cause` の結果で CPU の原因が PBO かドライバーの blt かに決まった後、flip の効果を小さな試験アプリで測り、その結果で A か B を選ぶ。

## 11. 出典

- DXGI flip model: https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model
- `IDXGISwapChain2::GetFrameLatencyWaitableObject`: https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_3/nf-dxgi1_3-idxgiswapchain2-getframelatencywaitableobject
- `IDXGISwapChain2::SetMaximumFrameLatency`: https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_3/nf-dxgi1_3-idxgiswapchain2-setmaximumframelatency
- `WGL_NV_DX_interop`: https://registry.khronos.org/OpenGL/extensions/NV/WGL_NV_DX_interop.txt
- `WGL_NV_DX_interop2`: https://registry.khronos.org/OpenGL/extensions/NV/WGL_NV_DX_interop2.txt
- PresentMon の PresentMode の定義: https://raw.githubusercontent.com/GameTechDev/PresentMon/007f1cf009670e8b90d014324515ed41e5c54422/IntelPresentMon/Interprocess/source/metadata/EnumPresentMode.h
- ANGLE の Windows のスワップチェーン: https://raw.githubusercontent.com/google/angle/chromium/5005/src/libANGLE/renderer/d3d/d3d11/win32/NativeWindow11Win32.cpp
- ANGLE の DirectComposition の属性: https://raw.githubusercontent.com/google/angle/chromium/5005/src/libANGLE/renderer/d3d/d3d11/Renderer11.cpp
- Qt の D3D11 の RHI のスワップチェーン: https://raw.githubusercontent.com/qt/qtbase/6.11/src/gui/rhi/qrhid3d11.cpp
- Qt の D3D12 の RHI のスワップチェーン: https://raw.githubusercontent.com/qt/qtbase/6.11/src/gui/rhi/qrhid3d12.cpp
- Qt の OpenGL の RHI の提示: https://raw.githubusercontent.com/qt/qtbase/6.11/src/gui/rhi/qrhigles2.cpp
- `QOpenGLWidget` の RHI の前提: https://raw.githubusercontent.com/qt/qtbase/6.11/src/openglwidgets/qopenglwidget.cpp
- `QRhiWidget` の API: https://doc.qt.io/qt-6/qrhiwidget.html
- `QRhiWidget` の実装: https://raw.githubusercontent.com/qt/qtbase/6.11/src/widgets/kernel/qrhiwidget.cpp
- Chrome の flip のスワップチェーンと待てるスワップチェーン: https://raw.githubusercontent.com/chromium/chromium/main/gpu/command_buffer/service/shared_image/dxgi_swap_chain_image_backing.cc
- mpv の WGL interop の GL コンテキスト: https://raw.githubusercontent.com/mpv-player/mpv/master/video/out/opengl/context_dxinterop.c
- mpv の D3D11 のスワップチェーンと待ち行列: https://raw.githubusercontent.com/mpv-player/mpv/master/video/out/gpu/d3d11_helpers.c
- Firefox の D3D11 の合成: https://raw.githubusercontent.com/mozilla/gecko-dev/master/gfx/layers/d3d11/CompositorD3D11.cpp
- Blender の WGL interop と D3D11 の共有: https://raw.githubusercontent.com/blender/blender/main/intern/ghost/intern/GHOST_ContextD3D.cc
- OBS の D3D11 の flip と待てるスワップチェーン: https://raw.githubusercontent.com/obsproject/obs-studio/master/libobs-d3d11/d3d11-subsystem.cpp
- OBS の GL のキャプチャの WGL interop: https://raw.githubusercontent.com/obsproject/obs-studio/master/plugins/win-capture/graphics-hook/gl-capture.c
- Intel の Iris Plus Graphics の API: https://www.intel.com/content/www/us/en/support/articles/000005524/graphics.html
