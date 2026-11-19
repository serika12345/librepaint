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
| `KisStrokePredictor` | `libs/tools` | 共通インターフェースである。実点の追加、予測の長さを与えた予測、リセット、予測点の出どころの報告を定める |
| `KisStrokeSampleHistory` | `libs/tools` | 起点から最新の実点までの履歴である。規則は「履歴の規則」の節に定める |
| `KisLinearStrokePredictor` | `libs/tools` | 現在の1次線形の予測器`KisStrokePrediction`を共通インターフェースに合わせる適合器である。Kalmanの実装との比較が終わるまで残す |
| `KisKalmanStrokePredictor` | `libs/tools` | 自前の予測器である。Windows、macOS、Linuxと、OSの予測の対象外の入力で使う |
| `KisPlatformStrokePredictor` | `libs/tools` | OSへ委譲する予測器である。OSの予測点をバッファから受け取り、共通の結果に詰める。OSのコードは供給元に分け、この予測器は偽のバッファで試験できる形にする |
| `KisPlatformPredictionBuffer` | `libs/tools` | OSの予測点を、実点の時刻をキーにして保持する環状バッファである。排他制御を持ち、複数のスレッドから使える |
| `KisStrokePredictionCap` | `libs/tools` | 予測の移動量の上限を、すべての実装の結果に同じ規則でかける共通の後処理である |
| iOSの供給元 | `krita/`の`KisIOSPencilInteraction.mm`と同じ場所 | Qtのビューに受け身のジェスチャ認識器を付け、予測タッチをバッファへ入れる |
| Androidの供給元 | `packaging/android/apk/src/org/krita/android/MainActivity.java`と、`libs/ui`のJNIの実装 | `dispatchTouchEvent`でOSの予測器を呼び、予測点をJNIでバッファへ入れる。初めは受け口だけを作る |
| `KisStrokePredictorFactory` | `libs/ui` | ストロークの開始時に実装を1つ選ぶ。選んだ実装はストロークの間は固定する |
| 表示の方針 | 既存の`KisToolFreehand` | 予測の置き換え、停止後の期限、ペンを離したときの消去を受け持つ。期限の値は、予測器と別にツールが持つ。すべての実装で共通にする |

共通インターフェースは、段階CでKalmanの実装と1次線形の実装を入れるときに作る。このとき本番の実装が2つ並び、再生器が予測器を差し替えて比べる試験の継ぎ目も要る。AGENTS.mdが抽象を足す条件に挙げる「本番の実装が複数ある」と「必要な試験の継ぎ目」の両方に当たる。

## インターフェース

```cpp
struct KisStrokePredictionSample {
    QPointF documentPos; // real point in document coordinates
    qreal timeMs = 0.0;  // Qt event time of the point, KoPointerEvent::time()
    qreal pressure = 0.0;
    bool tablet = false; // KoPointerEvent::isTabletEvent()
};

enum class KisStrokePredictionSource { None = 0, Linear = 1, Kalman = 2, IosSystem = 3, AndroidSystem = 4 };

class KisStrokePredictor
{
public:
    virtual ~KisStrokePredictor() = default;
    virtual void reset() = 0;
    virtual void addSample(const KisStrokePredictionSample &sample) = 0;
    /// horizonMs is how far past the latest real point the caller wants the
    /// prediction to reach, measured from that point's event time.
    virtual KisStrokePredictionResult predict(qreal horizonMs) const = 0;
    virtual KisStrokePredictionSource source() const = 0;
};
```

予測の長さは、最後の実点からの相対の時間で渡す。`KoPointerEvent::time()`の基準はプラットフォームごとに違い、X11ではサーバーの時刻、Windowsでは32ビットで一巡する値である。このため、表示の時刻との差は予測器の中では求めない。フレームの時計から予測の長さを求める段階では、イベントの時刻と単調時計の対応づけを別の項目として設計する。

結果は既存の`KisStrokePredictionResult`に`source`を足して使う。折れ線は起点、最新の実点、予測点の順に並ぶ。描画部品の`KisPredictionLinePainter`は、折れ線と最新の実点の位置を受け取る形なので、予測点が複数になっても`KisPredictionDecoration`はそのまま使える見込みである。

結果の項目の意味は次のとおりにそろえる。

| 項目 | 自前の予測器 | OSへ委譲する予測器 |
| --- | --- | --- |
| `velocity`、`speed` | 予測に使った速度 | 最新の実点から最後の予測点への変位を、その時間で割った値 |
| `confidence` | 信頼度の0から1 | -1。記録では値なしとして扱う |
| `moveLength` | 最新の実点から最後の予測点までの折れ線の長さ | 同じ |

## 履歴の規則

`KisStrokeSampleHistory`は、今の`KisStrokePrediction`と同じ次の規則を持つ。自前の予測器とOSへ委譲する予測器は、起点の折れ線をこの部品から作る。

- 同じ時刻の実点は、直前の実点を置き換える。
- 時刻が戻ったとき、または直前の実点から`gapResetMs`以上空いたときは、履歴を捨ててその点から始め直す。
- 最新の実点から`anchorMs`、速度の窓、`gapResetMs`、減速の長い窓の最大値より古い点を捨てる。直前の1点は残す。点の数の上限は64である。
- 起点は、最新の実点から`anchorMs`以内で最も古い点である。その範囲に最新の実点しかなければ、直前の点を起点にする。

## 移動量の上限

予測の移動量の上限は、すべての実装に同じ規則でかける。停止後の行き過ぎを抑える対策を、OSへ委譲する実装にも効かせるためである。`KisStrokePredictionCap`は、最新の実点から予測点までの折れ線の長さが上限を超えたとき、その長さが上限に等しくなる位置で折れ線を切る。予測点が1つの場合は、今の`KisStrokePrediction::predict`の上限と同じ結果になる。上限の値は、今と同じく直径の倍数`strokePredictionMaxMoveDiameters`にブラシの大きさを掛けて求める。

## 各実装

### 自前の予測器

- 1次線形の予測器に加えて、Androidの方式に沿ったKalmanフィルタの予測器を作る。状態は位置、速度、加速度、加速度の変化率の4つである。AndroidXのソースでは、加速度の変化率を`jank`と呼んでいる。加速度の変化率が大きいほど信頼度を下げて予測を縮める。
- 予測の長さには、今の`horizonMs`と同じく8msから25msの範囲の切り詰めをかける。
- 1次線形の実装は、再生器と計測での比較が終わるまで`KisLinearStrokePredictor`として残す。Kalmanの実装が`l2p-prediction-stop-overshoot.md`の段階3の合格条件を満たした時点で削除する。

### OSへ委譲する予測器と受け渡しのバッファ

- 供給元はOSの予測点を、同じイベントの最後の実点からの差分と、その実点からの相対時刻`dtMs`の組の列としてバッファへ入れる。差分の単位は、供給元の中でQtの論理画素に直す。
- バッファの1件には、キーの時刻、そのイベントに含まれる実点の数`coalescedCount`、予測点の列を持たせる。容量は250ms分とし、古い件から上書きする。
- `KisPlatformStrokePredictor`は、`addSample`で受け取った実点の時刻でバッファを引く。同じ時刻の実点が`coalescedCount`個そろった時点で、つまりそのイベントの最後の実点を受け取った時点で、予測を作る。途中の合体点では、直前の結果を保つ。
- 文書座標の予測点は、実点の`documentPos`に、差分をウィジェットから文書への変換の線形部分で写した値を足して求める。線形部分は`KisCoordinatesConverter`から取り、回転、鏡像、拡大を含む。`KoPointerEvent::pos()`は整数の座標なので、差分の計算には使わない。
- 予測の長さの`horizonMs`より`dtMs`が大きい予測点は切り捨てる。
- 対応する予測点がバッファにない場合は、直前の結果を保ち、`prediction_miss`の印を記録する。表示は停止後の期限で消える。ストロークの途中で自前の予測器へは切り替えない。予測の形が途中で変わるのを避けるためである。

### iOSの供給元

- 既存の`installKisIOSPencilInteraction`は、UIKitの部品をウィンドウに付けている。予測の供給元は、Qtのタッチを受けるビュー、つまり`quiview.mm`の`QUIView`に受け身のジェスチャ認識器を付ける。認識器は状態を「可能」のまま保ち、`cancelsTouchesInView`、`delaysTouchesBegan`、`delaysTouchesEnded`をすべてNOにする。
- UIKitは、ビューより先にジェスチャ認識器へタッチを渡すと考えている。この順序の前提は、段階Eの試作で確かめる。
- タッチが動くたびに`predictedTouchesForTouch:`を読み、同じUIEventの最後の実際の点からの差分と相対時刻を、バッファへ入れる。座標は、Qtと同じく`QUIView`を引数にした`locationInView:`で取る。UIKitのポイントはQtの論理画素に一致する。
- キーの時刻は、Qtと同じ式で求める。Qt 6.12.0の`quiview.mm`では、`getTimeStamp`が`ulong(event.timestamp * 1000)`を返し、この変換は小数を切り捨てる。`handleTouches:withEvent:withState:`は、この値を1回だけ求め、同じUIEventの合体点すべての`handleTabletEvent`に渡す。供給元も同じUIEventの`timestamp`から同じ式でキーを作る。
- Qtは改変しない。Qtのフォークを撤去したQ4の方針に合わせるためである。

### Androidの供給元

- `MainActivity`は`QtActivity`を継承している。ここで`dispatchTouchEvent`を上書きし、Qtへ渡す前に`android.view.MotionPredictor`の`record`と`predict`を呼ぶ。
- `predict`の引数の目標時刻は、`getEventTime`をナノ秒にした値に、予測の長さの上限を足した値とする。上限の値は、ストロークの開始時にC++側からJNIで渡す。
- `ACTION_DOWN`の時点で`isPredictionAvailable`を呼び、その結果を受け口に保存する。`KisStrokePredictorFactory`はこの値を読む。
- 予測点は、同じ`MotionEvent`の`getX`と`getY`からの差分を、物理画素からQtの論理画素へ直してJNIで渡す。キーは`getEventTime`である。Qtのタブレット経路が送るのも同じ`getEventTime`である。
- `minSdkVersion`は28で、`MotionPredictor`はAPI 34から使える。呼び出しは`Build.VERSION.SDK_INT >= 34`の判定と`@RequiresApi(34)`で囲む。`compileSdkVersion`の`androidCompileSdkVersion`が34以上かは未確認で、段階Fの最初に確かめる。
- 段階Fでは`dispatchTouchEvent`の上書き、JNIの関数、C++側の受け口を空の処理として作り、`isPredictionAvailable`の保存値を常に偽にする。空の処理は、Android機を使った内部の実装で置き換える。

### 実装の選択

`KisStrokePredictorFactory::create`は、ストロークの開始時に、押下の`KoPointerEvent`と予測の設定を受け取り、次の順で実装を選ぶ。

1. iOSで供給元が取り付けられ、押下が`isTabletEvent()`で、つまりApple Pencilの場合は、`KisPlatformStrokePredictor`を選ぶ。画面への指のタッチは対象にしない。
2. Androidで受け口の`isPredictionAvailable`の保存値が真の場合は、`KisPlatformStrokePredictor`を選ぶ。
3. それ以外では自前の予測器を選ぶ。

## 時刻と予測の長さ

- 呼び出し側のツールは、予測の長さを最後の実点からの相対の時間として渡す。最初は今の設定`strokePredictionHorizonMs`の値を使い、後でフレームの時計から求める。
- 自前の予測器は、その長さまでを予測する。
- OSへ委譲する予測器では、予測点の時刻はOSが決め、渡された長さより先の予測点は切り捨てる。

## スレッド

- Androidでは`dispatchTouchEvent`がAndroidのUIスレッドで呼ばれ、ツールはQtのスレッドで動く。バッファは排他制御付きの環状バッファとし、JNIでは値の写しだけを渡す。250ms分の容量は、Qtのスレッドの遅れで、引く前の値が上書きされるのを避けるためである。
- iOSではUIKitとQtのイベント処理が同じ主スレッドで動くと推測している。同じバッファを使えば、この推測が外れても安全である。

## 表示の方針

予測の置き換え、停止後の期限、ペンを離したときの消去は、すべての実装で`KisToolFreehand`が受け持つ。停止後の扱いをOSへ委譲できないためである。Androidでは停止時の消去がアプリ側の実装に任されていることを、AndroidXとAOSPのソースで確認した。Appleの文書が求めているのは新しいイベントのたびに予測を捨てることで、停止中の扱いは文書では不明である。期限の値は、`l2p-prediction-stop-overshoot.md`の再生器で決める。

## 記録の印

| 印 | 時点 | 値 |
| --- | --- | --- |
| `prediction_source` | 実装を選んだとき | `extra`に`KisStrokePredictionSource`の整数の値を入れる |
| `prediction_point` | 予測を描いたとき | 今と同じく位置と、信頼度の1000倍を`extra`に入れる。OSへ委譲する実装では-1を入れる |
| `prediction_miss` | 対応する予測点がバッファになかったとき | 位置に実点の文書座標を入れる |

`capture_analyze.py`と`analyze.py`は、`prediction_source`ごとに指標を分けて報告する。

## 入力機器

予測を有効にする条件の`KisStrokePredictionPolicy`は、どの入力機器でも同じ条件で判定する。予測はブラシツールの`continuePrimaryAction`の中で動き、この処理は板タブレット、液晶タブレット、マウスで共通である。OSへ委譲する実装は、iPadOSのApple PencilとAndroidのスタイラスに限られ、ほかの入力では自前の予測器が使われる。

見え方は入力機器で違うと推測している。液晶タブレットではペン先と線の端の距離として、入力から表示までの遅れの全体が見える。板タブレットでは画面上のカーソルが基準になる。基準がLibrePaintの描くブラシの輪郭の場合は、輪郭も表示の経路を通って遅れるため、遅れの全体を予測で埋めると、動いている間も線の端が輪郭より先へ出るおそれがある。予測の長さを呼び出し側が決める形にしたので、機器の種類ごとに長さを変える余地は残っている。描画中の輪郭の表示の既定と、輪郭とインクの遅れの差は未確認である。

## 検証

| プラットフォーム | 検証の手段 | 状態 |
| --- | --- | --- |
| Linux | 開発機での取り込みの計測と、再生器 | 手段がある |
| Windows | Surfaceでの取り込みの計測と、自動の結合試験 | 手段がある |
| macOS | Macでの取り込みの計測 | 手段がある |
| iPadOS | 実機での動作の確認。Apple Pencilの入力を合成できない項目は、最後に利用者へ実機での確認を頼む | 性能の計測手段はL3で作る |
| Android | 実機の自動の結合試験で、予測点の取得、表示の層への描画、ペンを離したときの消去、対象外の入力での自前の予測器への切り替えを確かめる | Android機の入手後に行う。最終のTODOとして残す |

共通の契約テストでは、偽のバッファを使い、対応づけ、合体点の数、差分の変換、予測の長さでの切り捨て、失敗時の扱いを確かめる。差分の変換には、回転、鏡像、拡大、画素密度2の場合を入れる。

## 段階

| 段階 | 作業 | 改善する指標または条文 | 完了条件 |
| --- | --- | --- | --- |
| A：測定の修正 | 減速して止まる試験軌跡、停止後の指標、予測の消去の理由の集計 | `l2p.stop_overshoot_px`と`l2p.stop_retract_px`の測定の信頼性 | Pythonの試験が通り、既存の指標の値が保たれる |
| B：再生器 | 記録から、今の`KisStrokePrediction`、期限、フレームの時計を再生する。記録にイベントの時刻を足す | すべての`l2p.*`の評価 | 既存の取り込みの記録から、取り込みの指標と同じ向きの結果が出る |
| C：共通インターフェースと自前のKalman | `KisStrokePredictor`、`KisLinearStrokePredictor`、`KisKalmanStrokePredictor`、`KisStrokePredictionCap`、`KisStrokePredictorFactory`、ツールの切り替え、記録の印を作り、再生器で比べ、Linuxで計測する | `l2p.residual_px`、`l2p.stop_overshoot_px`、`l2p.stop_retract_px` | 同じ入力の列で、旧`KisStrokePrediction`と`KisLinearStrokePredictor`の折れ線、`latestIndex`、信頼度が一致する契約テストが通る。Kalmanが`l2p-prediction-stop-overshoot.md`の段階3の合格条件を満たす |
| D：WindowsとmacOS | 同じ計測を行う | 同上 | 同上 |
| E：iPadOS | `KisStrokeSampleHistory`、`KisPlatformPredictionBuffer`、`KisPlatformStrokePredictor`、iOSの供給元 | 同上 | 実機で予測点の取得、描画、消去を確かめ、キーの一致率が99%以上になる。性能はL3で測る |
| F：Android | 受け口と実装の選択の分岐 | 利用者の決定の5点目による。指標は、Android機での内部の実装の段階で測る | 構築が通り、Androidで自前の予測器が選ばれる |

## 既知の制約

- iOSではQtが同じUIEventの合体点に同じ時刻を付ける。iOSで自前の予測器を代替に使うと、同じ時刻の点の置き換えにより、合体点の多くが履歴から落ちる。iOSで自前の予測器を代替に使うには、供給元から点ごとの時刻を受け取る必要がある。
- AndroidではQtのタブレット経路が`MotionEvent`の最新の座標だけを送り、まとめられた履歴の点を送らない。Androidで自前の予測器を使うと、標本が粗くなる。
- iOSのジェスチャ認識器が、Qtの認識器や既存の`UIPencilInteraction`と共存できるかは未検証で、段階Eの試作で確かめる。

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
