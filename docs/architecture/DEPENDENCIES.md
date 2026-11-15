# 対象側の依存の根拠

## 目的と読み方

この文書は、ソースから構築する対象側（Windows の MinGW 向け、Android 向け、iOS 向け）の依存について、LibrePaint がそれを必要とする根拠を記録する。根拠のない依存は構築しない。構築機側（Linux の構築機で動く道具）は対象としない。構築機側はバイナリキャッシュの標準版をそのまま使う方針で、手元で構築してよい部品は `nix/dependency-policy.json` の許可一覧が定める（`docs/perf/DECISIONS.md` の 2026-11-13 の行）。

読む人は、依存を足す人、nixpkgs を更新する人、構築の時間や配布サイズを調べる人を想定する。表の「根拠」は、LibrePaint のソースにある参照の場所である。行番号は 2026-11-13 の `CMakeLists.txt` のもので、ずれた場合は検索語で探す。

## 1. LibrePaint が直接要求する依存

トップレベルの `CMakeLists.txt` の `find_package` が要求するものと、同梱するプログラムである。

| 依存 | 用途 | 根拠 |
| --- | --- | --- |
| Qt（Core、Gui、Widgets、Xml、Network、Svg、Test、Concurrent、Sql、Core5Compat、OpenGL、OpenGLWidgets、SvgWidgets、Quick、QuickWidgets） | 画面とアプリ本体 | `CMakeLists.txt` の KRITA_REQUIRED_QT_COMPONENTS（556〜575 行）、`find_package(Qt…Quick)`（723 行）、`QuickWidget`（732 行）。Qt の各モジュールが連れる qtshadertools と qtlanguageserver は qtdeclarative の入力で、nixpkgs の定義が要求する |
| KDE Frameworks（Config、WidgetsAddons、Completion、CoreAddons、GuiAddons、I18n、ItemViews） | 設定、補完、多言語など | `find_package(KF6 …)`（544 行）。kcodecs は kcompletion の入力 |
| extra-cmake-modules | KDE の CMake 補助 | `find_package(ECM …)`（466 行） |
| Boost 1.69 以上 | 数値、コンテナー | 1137 行 |
| Immer、Zug、Lager | 保持データ構造、状態管理 | 1157〜1159 行 |
| GSL | 数値 | 1164 行 |
| Eigen3 | 行列 | 1387 行 |
| LCMS2 | 色管理 | 1404 行 |
| OpenColorIO | 色管理（OCIO の設定） | 1293 行 |
| FFTW3 | フーリエ変換（G'MIC のプラグインも使う） | 1281 行 |
| libpng（APNG 版）、libjpeg-turbo、libtiff、giflib、libwebp、OpenEXR、OpenJPEG、libheif、libjxl、libraw（libkdcraw 経由） | 画像の読み書き | 1124、1230、1240、1216、1253、1179、1210、1267、1260、1274、1575 行 |
| zlib | 圧縮 | 1202 行 |
| exiv2 | メタデータ | 1399 行 |
| Poppler（Qt6） | PDF の読み込み | 1490 行 |
| QuaZip | ZIP（KRA、ORA の読み書き） | 1563 行 |
| KSeExpr | 式のフィルター | 1190、1192 行 |
| libmypaint | MyPaint のブラシ | 1366 行 |
| xsimd | ベクトル演算 | 1442 行 |
| FreeType、HarfBuzz、Fontconfig、expat、libunibreak | 文字の描画と配置 | 1595、1607、1622、1631、1640 行 |
| fribidi | 双方向文字の配置（同梱の raqm が使う） | `3rdparty_vendor/raqm/CMakeLists.txt` |
| MLT 7 | アニメーションの再生の基盤 | 1353 行 |
| SDL2（sdl2-compat） | 音声の出力（MLT の sdl2_audio） | 1358 行、`libs/ui/animation/KisPlaybackEngineMLT.cpp` の `sdl2_audio`（232 行） |
| Python 3、PyQt6、PyQt6-sip、sip | Python プラグイン | 1315〜1339 行 |
| FFmpeg の ffmpeg、ffprobe | アニメーションの書き出しと、MLT が読む動画 | `libs/impex/animation/VideoExportOptionsDialog.cpp`（符号化器の一覧）、`libs/ui/animation/KisMLTProducerKrita.cpp`（avformat） |

## 2. 直接依存の機能の範囲（間接依存を削る方針）

直接依存の多くは、LibrePaint が使わない機能を持ち、その機能が追加の依存を連れてくる。使わない機能は無効にして、連れてくる依存を構築しない。

| 直接依存 | 使う機能 | 無効にする機能 | 連れてくる依存（構築しなくなる） |
| --- | --- | --- | --- |
| FFmpeg | 符号化器 libx264、libx265、libtheora、libwebp、apng、gif と、復号に使う libaom、dav1d、libopus、libvorbis、libogg、zlib、bzip2、lzma、iconv（`VideoExportOptionsDialog.cpp` の符号化器、MLT の avformat） | 通信（gnutls、libssh など）、字幕と字形（libass 系）、OpenCL、Vulkan、Bluray、OpenMPT、rist、srt、zimg、soxr、speex、vidstab、xvid、lame、xml2、ffplay | gnutls、nettle、libssh、libbluray、libopenmpt、librist、srt、ocl-icd、opencl-headers、zimg、soxr、speex、vid.stab、xvidcore、lame、libxml2 など |
| MLT | core、avformat、sdl2 の各モジュール（`KisMLTProducerKrita.cpp` の avformat と count、`KisPlaybackEngineMLT.cpp` の sdl2_audio、volume、blank、null は core にある） | その他のモジュール（frei0r、jack、ladspa、lv2、rtaudio、rubberband、vid.stab、qt、sox、xml、kdenlive など） | libebur128、libsamplerate、rnnoise、rtaudio、rubberband、libdv、libexif、jack、serd、sord、lilv、lv2、gdk-pixbuf、Qt の追加モジュールなど |
| Poppler | PDF のページを画像に描く | 署名の検証（NSS）、URL の読み込み（curl）、GPGME | nss、nspr、curl、nghttp2、libssh2 など |
| libheif | libaom と dav1d による AVIF の符号化と復号 | rav1e、gdk-pixbuf のローダー、例のプログラム | rav1e（Rust の構築）、auditable-cargo |
| libjxl | 画像の読み込みと書き出し | プラグイン、コマンドの道具、ベンチマーク、例、テスト | gperftools、gtest、gdk-pixbuf |
| OpenColorIO | 色変換のライブラリ | Python の接続部品、コマンドの道具 | python3（対象側）、pybind11、scikit-build-core |
| glib | LibrePaint のソースは glib を使わない | 構築しない | glib、gettext の一部、libffi、pcre2、harfbuzz の glib 連携 |
| zug、lager、xsimd | ヘッダーのライブラリ | テスト | catch2、doctest、cereal |

## 3. 根拠が未確認の依存

次は、構築に含まれているが、根拠を確かめていない。確かめた結果は、この表に追記する。

| 依存 | 引き込み元 | 確認の方法 |
| --- | --- | --- |
| qtlanguageserver | qtdeclarative | qtdeclarative の QML 言語サーバーの機能を無効にして外せるか |
| vulkan-loader | sdl3（sdl2-compat 経由） | sdl2-compat の Vulkan の連携を無効にできるか |
| libmng | qtsvg の入力（Qt の画像形式） | LibrePaint が MNG を使わないか。使わないなら Qt の機能を無効にする |
| python3.11 の道具（約 20 個） | 対象側の PyQt6 を作る sip、pyqt-builder、setuptools など | 構築機側の標準版にできるか（キャッシュを外れる理由が未調査） |

## 4. 変更の手順

依存を足す、または機能を有効にするときは、この文書の表に根拠（ソースの参照、または nixpkgs の定義が要求する理由）を書いてから定義を変える。依存の検査（`scripts/platform/check-nix-deps.py`）は、版の整合と構築機側の手元構築を確かめる。対象側の依存の根拠は、この文書で確かめる。

## 5. ビルドが必須な依存のサイズ

バイナリキャッシュにないために手元でソースから構築する派生物が、どれだけの容量を占めるかを集計する。目的は、キャッシュにない依存を減らす作業の効果を、件数だけでなくサイズでも比べられるようにすることである。

次のコマンドで集計する。

```sh
python3 scripts/platform/report-build-sizes.py
```

`--platform` で 1 つのプラットフォームを選び、`--top` で大きく表示する件数を変える。`--graph-dir` と `--cache-snapshot` は `scripts/platform/check-nix-deps.py` と同じ意味で、保存したグラフとキャッシュの照会結果を使う。macOS と iOS は macOS 上で、Linux と Windows と Android は Linux 上で実行する。サイズは、それぞれの実行環境のストアに出力があるときだけ分かる。

集計の定義は、キャッシュにない派生物の出力の NAR のサイズの合計である。分類は検査と同じ own、small、target、host を使い、own（LibrePaint 自身）は依存の合計に含めず、別の行に示す。依存の合計は small、target、host の合計である。まだ構築されていない出力はサイズ不明として件数だけを数え、サイズを推定して補わない。

