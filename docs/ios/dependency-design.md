# iOS依存構築の設計

## 目的と責務

iOSの外部依存物、アプリケーション、IPAを変更頻度に合わせて分離し、
ソース変更時にも依存物とキャッシュを再利用するための構成を定めます。
構築と保守の操作は[iOS開発マニュアル](../development/ios.md)に記載します。

Nixは公開ソースの依存物とホストツールを固定し、XcodeはApple ClangとSDKを供給します。
サンドボックス内のXcodeは`__impureHostDeps`で宣言した派生物だけから参照し、版をplistから
読み取ります。境界の決定は[NixとXcodeの分担](adr/0001-nix-xcode-boundary.md)と
[依存物の派生物設計](adr/0002-nix-target-derivations.md)に記録します。

## 依存物と利用側の契約

`ios-dependencies`が現在の採用依存物を集約し、`kf6-consumer-check`が利用側の最終リンクを
検証します。各派生物はXcode／SDK／コンパイラーの版と静的アーカイブの全要素を検査します。

| 依存物 | 保持する契約 |
| --- | --- |
| libpng | `PNG::PNG`からzlibを含む利用側のリンクが成立する |
| FreeType | `Freetype::Freetype`だけの指定からzlib／libpngを推移的に解決する |
| HarfBuzz | FreeType接続と配置先に依存しないCoreText参照を維持する |
| Fontconfig | ホスト検出はiPhoneOS SDKを外したNixコンパイラーを使い、利用側はXML、FreeType、生成設定を含む5アーカイブをリンクする |
| Expat、Little CMS、Eigen、xsimd | インストール済みCMake対象を利用でき、xsimdはarm64 SIMDをコンパイルできる |
| libunibreak | 製品の`Findlibunibreak.cmake`からUTF-8改行APIをリンクできる |
| libjpeg-turbo | JPEG／TurboJPEGの静的公開対象を個別にリンクでき、arm64 NEONオブジェクトを含む |
| Exiv2 | 監査済みライブラリー機能、JPEG／Exifと文字変換、zlib依存、SDKに依存しない`-liconv`指定を維持する |
| Boost | Xcodeに依存しないヘッダー派生物と、Appleツールチェーンでの利用側検証を分離する |
| Immer、Zug | 再配置可能なCMakeメタデータ、同一メジャー版の範囲照合、C++14要件を公開する。ZugはC++17の`std::variant`経路も検証する |
| Lager | `lager`対象がBoost／ZugヘッダーとC++17を伝播し、state／cursor／watch／store APIを単独指定で利用できる。デバッガー用Immer／Cerealは任意ヘッダーが所有する |
| libintl | `gettext-runtime/intl`のヘッダーと静的ライブラリーを対象出力とし、gettextツールをホスト側に置く。`Intl::Intl`からgettext／domain／pluralとiconv／CoreFoundationをリンクする |
| FriBidi | Mesonのホスト／対象設定を分離し、7個の表生成器をmacOSで実行する。ヘッダー、`libfribidi.a`、`fribidi.pc`を公開し、製品の検出経路でbidi種別、括弧、段落APIを検証する |

## アプリとIPAの構築境界

アプリ派生物は採用した静的プラグインと資源を含め、アーキテクチャ、Appleプラットフォーム、
最小OS版、SDK、バンドル情報、未署名状態、一時構築パスの混入を検査します。
IPA派生物は完成したアプリを入力とし、順序、日時、権限を正規化します。
Nixストア内のアプリは不変・未署名のまま保持します。

配備用複製と再現可能なIPAは、ディレクトリー`0755`、データ`0644`、実行形式`0755`を共有します。
完成アーカイブはシンボリックリンク、特殊ファイル、余分なメタデータ、非Unix属性、危険な名前、
作業用配置との目録差、DOS読み取り専用属性を検査します。この契約は軽量な成功・失敗試験で維持します。

ソース絞り込みはNix式、生成物、移植文書、TODOを製品コンパイル入力から分離します。
レシピや文書の変更は該当層で処理し、製品ソースとCMakeの変更はアプリを再構築します。
