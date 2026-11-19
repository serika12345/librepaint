# L2Pの予測器の構成：OSへの委譲と自前の予測器

## 目的と範囲

この文書は、L2Pの予測の線で使う予測器を、アプリ内部の共通インターフェースと、プラットフォームごとの実装に分ける設計を定める。読み手は[`l2p-prediction.md`](l2p-prediction.md)と[`l2p-prediction-stop-overshoot.md`](l2p-prediction-stop-overshoot.md)を読んだ開発者である。

予測の線を表示だけの層に描くこと、画素の正本、履歴、保存結果に影響させないこと、設定で切り替えられることは、`l2p-prediction.md`の定めのまま変えない。この文書が変えるのは、予測点の作り方と、その部品の分け方である。

改善する指標は、Issue #108の主指標を補う距離と外れの指標で、`l2p.residual_px`、`l2p.stop_overshoot_px`、`l2p.stop_retract_px`が該当する。作業はCHARTERの「予測の線」の条文の範囲で行う。

## 決定事項

2026-11-19に利用者が次の5点を決めた。

1. iPadOSとAndroidでは、OSが提供する予測を使う。
2. Windows、macOS、Linuxでは、自前の予測器を高度化して使う。
3. アプリ内部の予測器のインターフェースは全プラットフォームで統一し、iPadOSとAndroidではその実装の内部でOSへ委譲する。
4. iPadOS、Windows、macOS、Linuxは、実装と実機での検証まで行う。
5. Androidはインターフェースの実装、実装の選択の分岐、Java側とJNIの受け口だけを先に作り、OSへ委譲する内部の処理は後に回す。Android機はCHARTERの計測対象機に入っていないため、性能は計測せず、実機の自動の結合試験で動作を確かめる。Android機での確認は最終のTODOとして残す。

この決定は`l2p-prediction.md`の「設計の判断」にあった「OSの予測入力を使わない」という判断を置き換える。

## 決定の根拠

2026-11-19に各プラットフォームの公式文書と、Qt 6.12.0のソースを調べた。

| プラットフォーム | OSまたは標準APIの予測 | Qt 6.12.0の経路 |
| --- | --- | --- |
| iPadOS | `UIEvent.predictedTouches(for:)`でシステムの予測タッチを取得できる。Appleの文書は新しいイベントごとに捨てる一時データとして扱うよう求めている | `quiview.mm`がApple Pencilについて読むのは、`coalescedTouchesForTouch:`の実際の点だけである |
| Android | Android 14以降は`android.view.MotionPredictor`で、対象はスタイラスである | `QtInputDelegate.java`のタブレット経路は、`MotionEvent`の最新の座標と`getEventTime`を送る |
| Windows | `IDCompositionDelegatedInkTrail`の`AddTrailPointsWithPrediction`は、アプリが渡した予測点をOSが描く。予測点を作るのはアプリである | `qwindowspointerhandler.cpp`は`GetPointerPenInfoHistory`で履歴の実際の点を読む |
| macOS | AppKitの`NSEvent`の文書で予測の記述がある箇所は0件だった | タブレットのコードは実際の点だけを扱う |
| Linux X11 | XInput2のプロトコル仕様で予測の記述がある箇所は0件だった | `qxcbconnection_xi2.cpp`で予測を扱う箇所は0件だった |
| Linux Wayland | `tablet-v2`の仕様で予測の記述がある箇所は0件だった | `qwaylandinputdevice.cpp`で予測を扱う箇所は0件だった |

Qtの`qevent.h`と`qeventpoint.h`でも予測を扱う箇所は0件だった。Qtの経路から得られる情報は、どのプラットフォームでも実際の点と履歴だけである。このため、OSの予測を使うにはQtの外側にプラットフォーム別のコードを置く必要がある。

## 先行例

AndroidXの`MotionEventPredictor`では、1つのインターフェースの裏に、OSの`MotionPredictor`へ委譲する実装と、Kalmanフィルタの実装が置かれている。Android 14以降では委譲する実装が選ばれ、機器と入力の組がOSの対象外なら、その実装の内部でKalmanフィルタへ切り替わる。この設計は同じ構造をLibrePaintの予測器に当てはめる。

AndroidXのコードは移さず、方式だけを参考に実装する。AndroidXのライセンスはApache-2.0で、LibrePaintのGPL-2.0-or-laterのコードと組み合わせたときのライセンスの判断を避けるためである。

## 構成

| 部品 | 置き場所 | 責任 |
| --- | --- | --- |
| `KisStrokePredictor` | `libs/tools` | 共通インターフェースである。実点の追加、目標時刻を与えた予測、リセット、予測点の出どころの報告を定める |
| `KisLinearStrokePredictor` | `libs/tools` | 現在の1次線形の予測器`KisStrokePrediction`を共通インターフェースに合わせる適合器である。段階Dで自前のKalmanの予測器と比べ、合格したほうを残す |
| `KisKalmanStrokePredictor` | `libs/tools` | 自前の予測器である。Windows、macOS、Linuxと、OSの予測の対象外の入力で使う |
| `KisPlatformStrokePredictor` | `libs/tools` | OSへ委譲する予測器である。OSの予測点をバッファから受け取り、共通の結果に詰める。OSのコードは供給元に分け、この予測器は偽のバッファで試験できる形にする |
| `KisPlatformPredictionBuffer` | `libs/tools` | OSの予測点を実点の時刻をキーにして保持する小さな環状バッファである。排他制御を持ち、複数のスレッドから使える |
| iOSの供給元 | `krita/`の`KisIOSPencilInteraction.mm`と同じ場所 | Qtのビューに受け身のジェスチャ認識器を付け、予測タッチをバッファへ入れる |
| Androidの供給元 | `packaging/android/apk/src/org/krita/android/MainActivity.java`と、`libs/ui`のJNIの実装 | `dispatchTouchEvent`でOSの予測器を呼び、予測点をJNIでバッファへ入れる。段階Gでは受け口だけを作る |
| `KisStrokePredictorFactory` | `libs/ui` | ストロークの開始時に実装を1つ選ぶ。選んだ実装はストロークの間は固定する |
| 表示の方針 | 既存の`KisToolFreehand` | 予測の置き換え、停止後の期限、ペンを離したときの消去を受け持つ。すべての実装で共通にする |

AGENTS.mdは本番の実装が複数ある場合に抽象を足すと定めている。共通インターフェースには本番の実装が3つあり、この条件に当たる。

## インターフェース

```cpp
struct KisStrokePredictionSample {
    QPointF documentPos; // real point in document coordinates
    QPointF widgetPos;   // the same point in widget coordinates
    qreal timeMs = 0.0;  // Qt event time of the point
    qreal pressure = 0.0;
};

enum class KisStrokePredictionSource { None, Linear, Kalman, IosSystem, AndroidSystem };

class KisStrokePredictor
{
public:
    virtual ~KisStrokePredictor() = default;
    virtual void reset() = 0;
    virtual void addSample(const KisStrokePredictionSample &sample) = 0;
    /// targetTimeMs is the display time the caller wants the prediction to cover.
    virtual KisStrokePredictionResult predict(qreal targetTimeMs) const = 0;
    virtual KisStrokePredictionSource source() const = 0;
};
```

結果は既存の`KisStrokePredictionResult`に`source`を足して使う。折れ線は起点、最新の実点、予測点の順に並ぶ。描画部品の`KisPredictionLinePainter`は、折れ線と最新の実点の位置を受け取る形である。このため、予測点が複数になっても、`KisPredictionDecoration`はそのまま使える見込みである。起点から最新の実点までの履歴の管理は、自前の予測器とOSへ委譲する予測器で共通の部品にする。

## 各実装

### 自前の予測器

- 現在の1次線形の予測器をAndroidの方式に沿ったKalmanフィルタに置き換える。状態は位置、速度、加速度、jankの4つである。jankは加速度の変化率で、jankが大きいほど信頼度を下げて予測を縮める。
- 予測の長さは、目標時刻と最後の実点の時刻の差で決め、上限を置く。上限の値は、再生器で決める。
- 1次線形の実装は、再生器での比較が終わるまで`KisLinearStrokePredictor`として残す。Kalmanの実装が`l2p-prediction-stop-overshoot.md`の段階3の合格条件を満たした時点で削除する。

### OSへ委譲する予測器と受け渡しのバッファ

- 供給元はOSの予測点を、同じイベントの最後の実点からの差分としてバッファへ入れる。キーはその実点の時刻で、Qtがイベントに付ける時刻と同じ値にする。
- `KisPlatformStrokePredictor`は`addSample`で受け取った実点の時刻でバッファを引き、差分を実点のウィジェット座標に足してから実点と同じ変換で文書座標に直す。
- 差分で渡すのは、OSの座標とQtのウィンドウの座標のずれ、および画素密度の換算を、実点と同じ変換に任せるためである。
- 同じ時刻の予測点がバッファにない場合は、その実点では予測を出さない。ストロークの途中で自前の予測器へ切り替えないのは、予測の形が途中で変わるのを避けるためである。失敗の回数は計測の記録に残す。

### iOSの供給元

- 既存の`installKisIOSPencilInteraction`は、QtのビューにUIKitの部品を付けている。同じ方法で、受け身のジェスチャ認識器を付ける。認識器は状態を「可能」のまま保ち、`cancelsTouchesInView`、`delaysTouchesBegan`、`delaysTouchesEnded`をすべてNOにする。
- タッチが動くたびに`predictedTouchesForTouch:`を読み、同じUIEventの最後の実際の点からの差分として、予測点をバッファへ入れる。座標はQtと同じ`locationInView:`で取る。
- キーはQtと同じくUIEventの時刻をミリ秒にした値である。Qtは同じUIEventの合体点すべてにこの時刻を付けるので、予測点はそのUIEventの最後の実点に対応づけられる。
- Qtは改変しない。Qtのフォークを撤去したQ4の方針に合わせるためである。

### Androidの供給元

- `MainActivity`は`QtActivity`を継承している。ここで`dispatchTouchEvent`を上書きし、Qtへ渡す前に`android.view.MotionPredictor`の`record`と`predict`を呼ぶ。
- OSの予測器を直接呼ぶので、依存はAndroidのSDKの範囲に収まる。Android 14未満の端末と、`isPredictionAvailable`が偽になる機器では、自前の予測器が選ばれる。
- 予測点は同じ`MotionEvent`の`getX`と`getY`からの差分としてJNIで渡す。Qtのタブレット経路が送るのも同じ`getX`、`getY`、`getEventTime`なので、キーは`getEventTime`になる。
- 段階Gでは`dispatchTouchEvent`の上書きと、JNIの関数と、C++側の受け口を空の処理として作る。実装の選択は、Androidでも自前の予測器を返す。

### 実装の選択

`KisStrokePredictorFactory`はストロークの開始時に次の順で実装を選ぶ。

1. iOSで供給元が取り付けられ、入力がApple Pencilの場合は、`KisPlatformStrokePredictor`を選ぶ。
2. Androidで供給元がその機器を対象と報告した場合は、`KisPlatformStrokePredictor`を選ぶ。段階Gの時点では、この報告は常に偽である。
3. それ以外では自前の予測器を選ぶ。

選んだ実装の`source`は、計測の記録に`prediction_source`の印として残す。

## 時刻と目標時刻

- 呼び出し側のツールは、予測で覆いたい表示の時刻を目標時刻として渡す。最初は最後の実点の時刻に設定の予測の長さを足した値とし、後でフレームの時計から求める。
- 自前の予測器は、目標時刻までを予測する。
- OSへ委譲する予測器では、予測の長さはOSが決める。目標時刻より後の予測点は切り捨て、目標時刻は上限として働く。

## スレッド

- Androidでは`dispatchTouchEvent`がAndroidのUIスレッドで呼ばれ、ツールはQtのスレッドで動く。バッファは排他制御付きの環状バッファとし、JNIでは値の写しだけを渡す。
- iOSではUIKitとQtのイベント処理が同じ主スレッドで動くと推測している。同じバッファを使えば、この推測が外れても安全である。

## 表示の方針

予測の置き換え、停止後の期限、ペンを離したときの消去は、すべての実装で`KisToolFreehand`が受け持つ。停止後の扱いをOSへ委譲できないためである。Androidでは停止時の消去がアプリ側の実装に任されていることを、AndroidXとAOSPのソースで確認した。Appleの文書が求めているのは新しいイベントのたびに予測を捨てることで、停止中の扱いは文書では不明である。期限の値は、`l2p-prediction-stop-overshoot.md`の再生器で決める。

## 入力機器

予測を有効にする条件の`KisStrokePredictionPolicy`は、どの入力機器でも同じ条件で判定する。予測はブラシツールの`continuePrimaryAction`の中で動き、この処理は板タブレット、液晶タブレット、マウスで共通である。OSへ委譲する実装は、iPadOSの画面へのタッチとApple Pencil、Androidのスタイラスに限られ、ほかの入力では自前の予測器が使われる。

見え方は入力機器で違うと推測している。液晶タブレットではペン先と線の端の距離として、入力から表示までの遅れの全体が見える。板タブレットでは画面上のカーソルが基準になる。基準がLibrePaintの描くブラシの輪郭の場合は、輪郭も表示の経路を通って遅れるため、遅れの全体を予測で埋めると、動いている間も線の端が輪郭より先へ出るおそれがある。目標時刻を呼び出し側が決める形にしたので、機器の種類ごとに目標時刻を変える余地は残っている。描画中の輪郭の表示の既定と、輪郭とインクの遅れの差は未確認である。

## 検証

| プラットフォーム | 検証の手段 | 状態 |
| --- | --- | --- |
| Linux | 開発機での取り込みの計測と、再生器 | 手段がある |
| Windows | Surfaceでの取り込みの計測と、自動の結合試験 | 手段がある |
| macOS | Macでの取り込みの計測 | 手段がある |
| iPadOS | 実機での動作の確認。Apple Pencilの入力を合成できない項目は、最後に利用者へ実機での確認を頼む | 性能の計測手段はL3で作る |
| Android | 実機の自動の結合試験で、予測点の取得、表示の層への描画、ペンを離したときの消去、対象外の入力での自前の予測器への切り替えを確かめる | Android機の入手後に行う。最終のTODOとして残す |

共通の契約テストでは偽のバッファを使い、対応づけ、差分の変換、目標時刻での切り捨て、失敗時の扱いを確かめる。

## 段階

| 段階 | 作業 | 改善する指標または条文 | 完了条件 |
| --- | --- | --- | --- |
| A：測定の修正 | 減速して止まる試験軌跡、速さと遅れの換算の固定、停止の指標 | `l2p.stop_overshoot_px`と`l2p.stop_retract_px`の測定の信頼性 | Pythonの試験が通り、既存の指標の値が保たれる |
| B：共通インターフェース | `KisStrokePredictor`、`KisLinearStrokePredictor`、`KisStrokePredictorFactory`、ツールの切り替え、記録の印 | CHARTERの「予測の線」の構造の準備 | 予測の線の振る舞いが保たれる。契約テストと`verify-quick`が通る |
| C：再生器 | 記録から予測器、期限、フレームの時計を再生する | すべての`l2p.*`の評価 | 既存の取り込みの記録から、取り込みの指標と同じ向きの結果が出る |
| D：自前のKalman | `KisKalmanStrokePredictor`を作り、再生器で1次線形と比べ、Linuxで計測する | `l2p.residual_px`、`l2p.stop_overshoot_px`、`l2p.stop_retract_px` | `l2p-prediction-stop-overshoot.md`の段階3の合格条件を満たす |
| E：WindowsとmacOS | 同じ計測を行う | 同上 | 同上 |
| F：iPadOS | バッファ、`KisPlatformStrokePredictor`、iOSの供給元 | 同上 | 実機で予測点の取得、描画、消去を確かめる。性能はL3で測る |
| G：Android | 受け口と実装の選択の分岐 | 利用者の決定の5点目 | 構築が通り、Androidで自前の予測器が選ばれる |

## 既知の制約

- iOSではQtが同じUIEventの合体点に同じ時刻を付ける。`KisStrokePrediction::addSample`は同じ時刻の点を置き換えるため、iOSで自前の予測器を使うと合体点の多くが履歴から落ちる。iOSで自前の予測器を代替に使うには供給元から点ごとの時刻を受け取る必要がある。
- AndroidではQtのタブレット経路が`MotionEvent`の最新の座標だけを送り、まとめられた履歴の点を送らない。Androidで自前の予測器を使うと、標本が粗くなる。
- iOSのジェスチャ認識器が、Qtの認識器や既存の`UIPencilInteraction`と共存できるかは未検証で、段階Fの試作で確かめる。

## 出典

- Apple, Minimizing latency with predicted touches: https://developer.apple.com/documentation/uikit/minimizing-latency-with-predicted-touches
- Apple, `UIEvent.predictedTouches(for:)`: https://developer.apple.com/documentation/uikit/uievent/predictedtouches(for:)
- AndroidX, `MotionEventPredictor.java`: https://android.googlesource.com/platform/frameworks/support/+/androidx-main/input/input-motionprediction/src/main/java/androidx/input/motionprediction/MotionEventPredictor.java
- AOSP, `MotionPredictor.cpp`: https://android.googlesource.com/platform/frameworks/native/+/refs/heads/main/libs/input/MotionPredictor.cpp
- Microsoft, `IDCompositionDelegatedInkTrail`: https://learn.microsoft.com/en-us/windows/win32/api/dcomp/nn-dcomp-idcompositiondelegatedinktrail
- Wayland, `tablet-v2`: https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/main/stable/tablet/tablet-v2.xml
- X.Org, XInput2プロトコル: https://www.x.org/releases/current/doc/inputproto/XI2proto.txt
- Qt 6.12.0, `quiview.mm`: https://github.com/qt/qtbase/blob/v6.12.0/src/plugins/platforms/ios/quiview.mm
- Qt 6.12.0, `qwindowspointerhandler.cpp`: https://github.com/qt/qtbase/blob/v6.12.0/src/plugins/platforms/windows/qwindowspointerhandler.cpp
- Qt 6.12.0, `QtInputDelegate.java`: https://github.com/qt/qtbase/blob/v6.12.0/src/android/jar/src/org/qtproject/qt/android/QtInputDelegate.java
