/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QDomDocument>
#include <QObject>
#include <QPointF>
#include <QTest>

#include <canvas/KisMirrorAxisConfig.h>

void kis_assert_exception(const char *condition, const char *file, int line)
{
    qFatal("unexpected assertion: %s at %s:%d", condition, file, line);
}

void kis_safe_assert_recoverable(const char *condition, const char *file, int line)
{
    qFatal("unexpected recoverable assertion: %s at %s:%d", condition, file, line);
}

class KisMirrorAxisConfigContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsExposeStableDecorationState();
    void settersUpdateIndependentValues();
    void copiesAssignmentsAndEqualityPreserveValueSemantics();
    void xmlRoundTripPreservesConfiguration();
};

void KisMirrorAxisConfigContractTest::defaultsExposeStableDecorationState()
{
    const KisMirrorAxisConfig config;

    QVERIFY(config.isDefault());
    QVERIFY(!config.mirrorHorizontal());
    QVERIFY(!config.mirrorVertical());
    QVERIFY(!config.lockHorizontal());
    QVERIFY(!config.lockVertical());
    QVERIFY(!config.hideHorizontalDecoration());
    QVERIFY(!config.hideVerticalDecoration());
    QCOMPARE(config.handleSize(), 32.0f);
    QCOMPARE(config.horizontalHandlePosition(), 64.0f);
    QCOMPARE(config.verticalHandlePosition(), 64.0f);
    QCOMPARE(config.axisPosition(), QPointF());
}

void KisMirrorAxisConfigContractTest::settersUpdateIndependentValues()
{
    KisMirrorAxisConfig config;
    config.setMirrorHorizontal(true);
    config.setMirrorVertical(true);
    config.setLockHorizontal(true);
    config.setLockVertical(true);
    config.setHideHorizontalDecoration(true);
    config.setHideVerticalDecoration(true);
    config.setHandleSize(41.5f);
    config.setHorizontalHandlePosition(-7.25f);
    config.setVerticalHandlePosition(93.75f);
    config.setAxisPosition(QPointF(12.5, -4.25));

    QVERIFY(config.mirrorHorizontal());
    QVERIFY(config.mirrorVertical());
    QVERIFY(config.lockHorizontal());
    QVERIFY(config.lockVertical());
    QVERIFY(config.hideHorizontalDecoration());
    QVERIFY(config.hideVerticalDecoration());
    QCOMPARE(config.handleSize(), 41.5f);
    QCOMPARE(config.horizontalHandlePosition(), -7.25f);
    QCOMPARE(config.verticalHandlePosition(), 93.75f);
    QCOMPARE(config.axisPosition(), QPointF(12.5, -4.25));
    QVERIFY(!config.isDefault());
}

void KisMirrorAxisConfigContractTest::copiesAssignmentsAndEqualityPreserveValueSemantics()
{
    KisMirrorAxisConfig source;
    source.setMirrorHorizontal(true);
    source.setHandleSize(18.0f);
    source.setAxisPosition(QPointF(3.0, 9.0));

    KisMirrorAxisConfig copy(source);
    QCOMPARE(copy, source);
    copy.setMirrorVertical(true);
    QVERIFY(copy != source);
    QVERIFY(!source.mirrorVertical());

    KisMirrorAxisConfig assigned;
    assigned = source;
    QCOMPARE(assigned, source);
    const KisMirrorAxisConfig *self = &assigned;
    assigned = *self;
    QCOMPARE(assigned, source);
}

void KisMirrorAxisConfigContractTest::xmlRoundTripPreservesConfiguration()
{
    KisMirrorAxisConfig source;
    source.setMirrorHorizontal(true);
    source.setLockVertical(true);
    source.setHideHorizontalDecoration(true);
    source.setHandleSize(27.5f);
    source.setHorizontalHandlePosition(6.25f);
    source.setVerticalHandlePosition(71.5f);
    source.setAxisPosition(QPointF(-2.5, 8.75));

    QDomDocument document;
    const QDomElement element = source.saveToXml(document, QStringLiteral("mirror-axis"));
    QCOMPARE(element.tagName(), QStringLiteral("mirror-axis"));

    KisMirrorAxisConfig restored;
    QVERIFY(restored.loadFromXml(element));
    QCOMPARE(restored, source);
}

QTEST_GUILESS_MAIN(KisMirrorAxisConfigContractTest)

#include "KisMirrorAxisConfigContractTest.moc"
