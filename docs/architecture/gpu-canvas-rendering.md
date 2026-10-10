# GPUキャンバス画像の描画

## 目的と構成

`libs/canvas/gpu`はGPU文書から受け取った画像の座標変換、画素の補間と背景合成を所有する。
`kritacanvasgpurenderer`は`kritaimagegpudocument`とQt Guiを利用する。
`kritacanvasgpusurface`はこの描画側を利用し、専用のQtウィンドウへの提示を所有する。
macOSではAppKitとQuartzCore、LinuxのX11ではXlibをOS接続に利用する。
文書の画素・版・予算は[GPU文書所有者](gpu-document-tiles.md)に属し、表示側から依存する。

| ファイル | 所有内容 |
| --- | --- |
| `libs/canvas/gpu/GpuRenderer.h` | 描画条件、結果、資源の借用と完了の内部API |
| `libs/canvas/gpu/GpuRenderer.cpp` | 描画先の検査、有限の発行、資源保持と完了回収 |
| `libs/canvas/gpu/GpuRenderer.wgsl` | 座標変換、透明画素の補間と背景合成 |
| `libs/canvas/gpu/tests/KisGpuCanvasRendererTest.cpp` | 実GPUの画素、拒否、寿命と喪失の契約 |
| `libs/canvas/gpu/SurfaceRenderer.h`、`libs/canvas/gpu/SurfaceRenderer.cpp` | 表示条件、表示面の設定、有限のフレーム提示 |
| `libs/canvas/gpu/GpuWindowSurface_p.h` | 表示側内部のOS資源所有者 |
| `libs/canvas/gpu/GpuWindowSurface_mac.mm` | CocoaウィンドウのsRGB Metalレイヤー |
| `libs/canvas/gpu/GpuWindowSurface_x11.cpp` | X11ウィンドウの表示面と実寸法の検査 |
| `libs/canvas/gpu/tests/KisGpuSurfaceRendererTest.cpp` | 実ウィンドウの提示、サイズ変更、寿命とネイティブ失敗の契約 |

`LIBREPAINT_BUILD_GPU_DOCUMENT=ON`で構築する。
`Krita::Canvas::GpuRenderer`は、成功した`TextureSnapshot`を直接読み、指定したGPU描画先へ描く。
CPUへ渡す値は変換行列、背景色と補間方式の48バイトである。

## 画素と座標

入力はアルファと独立したsRGBのRGBA8、描画先は同じsRGBの`RGBA8Unorm`または
`BGRA8Unorm`とする。キャンバス座標から描画先の物理画素へのアフィン変換を指定し、
GPUは逆変換で画素中心を入力画像の座標へ戻す。移動、拡大縮小、任意角の回転と鏡像を扱う。
画像の原点は`TextureSnapshot::bounds()`のキャンバス座標に従う。

最近傍補間は対象画素をそのまま読む。線形補間は周囲の四画素へアルファを乗じてから
補間し、透明画素のRGBが輪郭へ混ざることを防ぐ。画像外は透明な黒とする。
補間した画素を指定した背景へ通常合成し、出力のRGBをアルファと独立した値へ戻す。
補間と合成はsRGBの符号化値で実行する。出力は成分を0〜1へ制限し、最も近い8ビット値へ
丸める。中間値は偶数側へ丸め、描画先形式の変換前に値を確定する。

ICCによる色変換と異なる色空間の描画先は、色精度の比較契約を持つ受入れ単位へ分ける。

## 実ウィンドウへの提示

専用の`QWindow`に対して、ネイティブ資源を生成する前に`SurfaceRenderer::prepareWindow()`を呼ぶ。
ウィンドウが表示可能になってから、デバイスとウィンドウを借用して`SurfaceRenderer`を生成する。
双方の所有者を表示側より長く保持する。生成、提示、完了回収と破棄はGUIスレッドで実行し、
デバイスの発行側スレッドと一致させる。ウィンドウのネイティブ資源の再作成は表示側の再生成で扱う。

`present()`は成功済みのGPU画像を取得した表示用テクスチャへ描き、同じキューへの発行後に
OSへ提示を要求する。ウィンドウの論理寸法に画素比を乗じた物理寸法を使う。
対応形式は`BGRA8Unorm`を優先し、`RGBA8Unorm`を代替とする。FIFOと不透明な表示面を使用し、
アルファを背景へ合成した画像を提示する。MetalレイヤーにはsRGB色空間を指定する。

未回収フレーム数の上限は1または2とする。サイズ変更は前のフレームを全て回収した後に
適用する。X11ではサーバーのウィンドウ寸法も照合し、変更が未反映なら`ResizePending`を返す。
呼出し側はイベント処理と`poll()`を進めて再試行する。描画操作内でGPU完了を待たない。
終了時は描画側を先に破棄して全通知を回収し、その後に表示面とOS資源を解放する。

表示面の予算は物理画素数×4バイト×3画像を予約量として計上する。
これは描画中と表示中のRGBA8画像の論理予約量であり、ドライバーの内部領域は別に計測する。
サイズ超過は設定と発行前に`BudgetExceeded`として拒否する。
再設定に失敗しても、前に確保した表示面の予約量を保持する。
`presentationRequests`はOSが受理した提示要求数、`rendering`はGPU描画の完了と命令転送量を返す。
実表示時刻と入力から表示までの遅延は、OSの表示完了通知と連結する計測単位で受け入れる。

非表示、利用不能、別のネイティブ資源へ変わったウィンドウは`WindowUnavailable`とする。
未完了・失敗・別デバイスの画像は`ImageRejected`、発行数超過は`QueueFull`とする。
表示面の取得失敗は`SurfaceUnavailable`、提示失敗は`PresentFailed`を返す。
古い表示面や喪失した表示面は次の要求で再設定する。デバイス喪失は新規要求を拒否し、
未完了の描画結果を失敗へ確定する。これらの失敗を受けた呼出し側は結果の採用を止める。

対応するQt表示環境はmacOSのCocoaとLinuxのX11とし、他の環境は構築時に例外で拒否する。
Wayland固有の表示接続は、ネイティブ資源の所有と提示完了の検証を伴う別の受入れ単位とする。
OS接続には[QtのネイティブAPI](https://doc.qt.io/qt-6/qnativeinterface-qx11application.html)と
[CAMetalLayer](https://developer.apple.com/documentation/quartzcore/cametallayer)を利用する。

## 所有と完了

デバイス所有者は描画側より長く保持する。構築、描画、`poll()`と破棄はデバイスの発行側
スレッドで実行する。描画側は入力画像、描画先の参照、画像ビュー、命令用バッファーを
全ての完了通知が揃うまで保持し、`poll()`で回収する。通知は完了状態だけを更新する。
描画先の画素は`Frame::status()`が`Succeeded`になってから採用する。
描画側の破棄は自身の発行完了を待ち、外へ渡した結果を成功または失敗へ確定する。

回収前の最大発行数は構築時に指定する。上限に達した要求は`QueueFull`として拒否し、
完了回収後に再試行できる。一発行のGPU命令用領域は48バイトで、
`residentParameterBytes`が保持量を返す。入力画像と描画先の画素領域は各所有者が計上する。
`parameterUploadBytes`と`submissions`は受理した発行の累積を返す。

## 拒否と失敗

未初期化、失敗した結果、別デバイスの入力は`InvalidImage`、未完了の入力は`ImagePending`とする。
描画先の所有元、形式、用途、次元、標本数を発行前に検査し、不一致は`InvalidTarget`とする。
固定GPU依存の`wgpuLibrePaintTextureUsesDevice()`がネイティブ資源の所有元を検査する。
非アフィン、逆変換を持たない行列、単精度で有限値へ表現できない変換は`InvalidTransform`とする。
これらの拒否は確保と発行を伴わず、既存の結果を維持する。

GPU検査や描画先の破棄による発行失敗は`Failed`として採用を止め、全通知後に資源を回収する。
デバイス喪失は`poll()`で未完了結果を失敗へ確定し、新しい描画を`DeviceLost`として拒否する。
未対応の構築条件と利用不能なデバイスは`std::runtime_error`を返す。

## 利用方法と保守

検証は開発マニュアルの[GPU文書タイルの検証](DEVELOPMENT.md#gpu文書タイルの検証)に従う。
固定入力とQtの座標変換を比較し、移動、拡大縮小、直交・任意角の回転と鏡像の画素を検査する。
透明な有色画素の補間、背景合成、BGRAの成分順、画像の早期解放、発行数制限、
拒否の原子性、別デバイス・破棄済み描画先、デバイス喪失と描画側破棄後の完了を検査する。
試験側だけが結果をCPUへ読み戻す。実表示では同じ描画側を専用ウィンドウへ接続し、
連続した提示、サイズ変更、予算超過、拒否後の再試行、破棄とGPU喪失を検査する。
子プロセスでは画像を保持したまま次の画像を取得する操作、表示面の先行破棄、未設定状態、
喪失後の取得・設定と未対応のQt環境を検査し、通常終了と失敗値を確認する。
