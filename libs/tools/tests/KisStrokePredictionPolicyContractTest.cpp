/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePredictionPolicy.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QTest>
#include <QVector>

namespace
{

KisStrokePredictionPolicyInput enabledInput()
{
    KisStrokePredictionPolicyInput input;
    input.settingEnabled = true;
    input.smoothingType = 1;
    input.paintOpId = QStringLiteral("paintbrush");
    return input;
}

QVector<KisStrokePredictionPolicyReason> allReasons()
{
    return QVector<KisStrokePredictionPolicyReason>{
        KisStrokePredictionPolicyReason::Enabled,
        KisStrokePredictionPolicyReason::SettingOff,
        KisStrokePredictionPolicyReason::Smoothing,
        KisStrokePredictionPolicyReason::PaintOp,
        KisStrokePredictionPolicyReason::Opacity,
        KisStrokePredictionPolicyReason::Flow,
        KisStrokePredictionPolicyReason::Eraser,
        KisStrokePredictionPolicyReason::Composite,
        KisStrokePredictionPolicyReason::Assistant,
        KisStrokePredictionPolicyReason::Tool,
        KisStrokePredictionPolicyReason::Delay};
}

} // namespace

class KisStrokePredictionPolicyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enabledWhenAllConditionsPass();
    void settingOffDisables();
    void unsupportedSmoothingDisables();
    void noAndSimpleSmoothingPass();
    void otherPaintOpDisables();
    void opacityBelowOneDisables();
    void flowBelowOneDisables();
    void eraserDisables();
    void nonNormalCompositeDisables();
    void activeAssistantDisables();
    void unsupportedToolDisables();
    void delayDistanceDisables();
    void firstReasonInOrderWins();
    void reasonNamesAreUniqueAndNotEmpty();
};

void KisStrokePredictionPolicyContractTest::enabledWhenAllConditionsPass()
{
    const auto reason = KisStrokePredictionPolicy::evaluate(enabledInput());
    QCOMPARE(reason, KisStrokePredictionPolicyReason::Enabled);
    QCOMPARE(QByteArray(KisStrokePredictionPolicy::reasonName(reason)), QByteArray("enabled"));
}

void KisStrokePredictionPolicyContractTest::settingOffDisables()
{
    KisStrokePredictionPolicyInput input = enabledInput();
    input.settingEnabled = false;
    QCOMPARE(KisStrokePredictionPolicy::evaluate(input),
             KisStrokePredictionPolicyReason::SettingOff);
}

void KisStrokePredictionPolicyContractTest::unsupportedSmoothingDisables()
{
    for (int type : {2, 3, 4}) {
        KisStrokePredictionPolicyInput input = enabledInput();
        input.smoothingType = type;
        QCOMPARE(KisStrokePredictionPolicy::evaluate(input),
                 KisStrokePredictionPolicyReason::Smoothing);
    }
}

void KisStrokePredictionPolicyContractTest::noAndSimpleSmoothingPass()
{
    for (int type : {0, 1}) {
        KisStrokePredictionPolicyInput input = enabledInput();
        input.smoothingType = type;
        QCOMPARE(KisStrokePredictionPolicy::evaluate(input),
                 KisStrokePredictionPolicyReason::Enabled);
    }
}

void KisStrokePredictionPolicyContractTest::otherPaintOpDisables()
{
    KisStrokePredictionPolicyInput input = enabledInput();
    input.paintOpId = QStringLiteral("colorsmudge");
    QCOMPARE(KisStrokePredictionPolicy::evaluate(input),
             KisStrokePredictionPolicyReason::PaintOp);
}

void KisStrokePredictionPolicyContractTest::opacityBelowOneDisables()
{
    for (qreal opacity : {0.5, 0.999}) {
        KisStrokePredictionPolicyInput input = enabledInput();
        input.opacity = opacity;
        QCOMPARE(KisStrokePredictionPolicy::evaluate(input),
                 KisStrokePredictionPolicyReason::Opacity);
    }
}

void KisStrokePredictionPolicyContractTest::flowBelowOneDisables()
{
    KisStrokePredictionPolicyInput input = enabledInput();
    input.flow = 0.8;
    QCOMPARE(KisStrokePredictionPolicy::evaluate(input),
             KisStrokePredictionPolicyReason::Flow);
}

void KisStrokePredictionPolicyContractTest::eraserDisables()
{
    KisStrokePredictionPolicyInput input = enabledInput();
    input.eraser = true;
    QCOMPARE(KisStrokePredictionPolicy::evaluate(input),
             KisStrokePredictionPolicyReason::Eraser);
}

void KisStrokePredictionPolicyContractTest::nonNormalCompositeDisables()
{
    KisStrokePredictionPolicyInput input = enabledInput();
    input.compositeNormal = false;
    QCOMPARE(KisStrokePredictionPolicy::evaluate(input),
             KisStrokePredictionPolicyReason::Composite);
}

void KisStrokePredictionPolicyContractTest::activeAssistantDisables()
{
    KisStrokePredictionPolicyInput input = enabledInput();
    input.assistantActive = true;
    QCOMPARE(KisStrokePredictionPolicy::evaluate(input),
             KisStrokePredictionPolicyReason::Assistant);
}

void KisStrokePredictionPolicyContractTest::unsupportedToolDisables()
{
    KisStrokePredictionPolicyInput input = enabledInput();
    input.toolSupported = false;
    QCOMPARE(KisStrokePredictionPolicy::evaluate(input),
             KisStrokePredictionPolicyReason::Tool);
}

void KisStrokePredictionPolicyContractTest::delayDistanceDisables()
{
    KisStrokePredictionPolicyInput input = enabledInput();
    input.delayDistance = true;
    QCOMPARE(KisStrokePredictionPolicy::evaluate(input),
             KisStrokePredictionPolicyReason::Delay);
}

void KisStrokePredictionPolicyContractTest::firstReasonInOrderWins()
{
    KisStrokePredictionPolicyInput input = enabledInput();
    input.smoothingType = 2;
    input.opacity = 0.5;
    QCOMPARE(KisStrokePredictionPolicy::evaluate(input),
             KisStrokePredictionPolicyReason::Smoothing);

    input = enabledInput();
    input.flow = 0.5;
    input.eraser = true;
    QCOMPARE(KisStrokePredictionPolicy::evaluate(input),
             KisStrokePredictionPolicyReason::Flow);
}

void KisStrokePredictionPolicyContractTest::reasonNamesAreUniqueAndNotEmpty()
{
    QStringList names;
    const auto reasons = allReasons();
    for (auto reason : reasons) {
        const QString name = QString::fromUtf8(KisStrokePredictionPolicy::reasonName(reason));
        QVERIFY(!name.isEmpty());
        QVERIFY(!names.contains(name));
        names << name;
    }
    QCOMPARE(names.size(), reasons.size());
}

QTEST_GUILESS_MAIN(KisStrokePredictionPolicyContractTest)

#include "KisStrokePredictionPolicyContractTest.moc"
