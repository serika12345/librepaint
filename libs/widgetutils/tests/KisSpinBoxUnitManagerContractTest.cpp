/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_spin_box_unit_manager.h"

#include <QMetaMethod>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>
#include <QtMath>

class KisSpinBoxUnitManagerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dimensionsConstraintsAndInitialStateRemainStable();
    void unitListsAndModelRolesRemainStable();
    void conversionFactorsConstantsAndValuesRemainStable();
    void unitSelectionAndSignalsRemainStable();
    void managerSynchronizationAndDisconnectRemainStable();
};

void KisSpinBoxUnitManagerContractTest::dimensionsConstraintsAndInitialStateRemainStable()
{
    QCOMPARE(int(KisSpinBoxUnitManager::LENGTH), 0);
    QCOMPARE(int(KisSpinBoxUnitManager::IMLENGTH), 1);
    QCOMPARE(int(KisSpinBoxUnitManager::ANGLE), 2);
    QCOMPARE(int(KisSpinBoxUnitManager::TIME), 3);

    QCOMPARE(int(KisSpinBoxUnitManager::NOCONSTR), 0);
    QCOMPARE(int(KisSpinBoxUnitManager::REFISINT), 1);
    QCOMPARE(int(KisSpinBoxUnitManager::VALISINT), 2);

    KisSpinBoxUnitManager::Constrains constraints(KisSpinBoxUnitManager::REFISINT);
    constraints |= KisSpinBoxUnitManager::VALISINT;
    QVERIFY(constraints.testFlag(KisSpinBoxUnitManager::REFISINT));
    QVERIFY(constraints.testFlag(KisSpinBoxUnitManager::VALISINT));

    QVERIFY(KisSpinBoxUnitManager::isUnitId(KisSpinBoxUnitManager::LENGTH));
    QVERIFY(KisSpinBoxUnitManager::isUnitId(KisSpinBoxUnitManager::ANGLE));
    QVERIFY(KisSpinBoxUnitManager::isUnitId(KisSpinBoxUnitManager::TIME));
    QVERIFY(!KisSpinBoxUnitManager::isUnitId(-1));
    QVERIFY(!KisSpinBoxUnitManager::isUnitId(4));

    // Maintained characterization: IMLENGTH is a dimension but isUnitId() rejects it.
    QVERIFY(!KisSpinBoxUnitManager::isUnitId(KisSpinBoxUnitManager::IMLENGTH));

    QCOMPARE(KisSpinBoxUnitManager::referenceUnitSymbols,
             QStringList({QStringLiteral("pt"),
                          QStringLiteral("px"),
                          QString::fromUtf8("°"),
                          QStringLiteral("frame")}));

    QPointer<KisSpinBoxUnitManager> lifetime;
    {
        QObject parent;
        lifetime = new KisSpinBoxUnitManager(&parent);
        QCOMPARE(lifetime->getUnitDimensionType(), int(KisSpinBoxUnitManager::LENGTH));
        QCOMPARE(lifetime->getReferenceUnitSymbol(), QStringLiteral("pt"));
        QCOMPARE(lifetime->getApparentUnitSymbol(), QStringLiteral("pt"));
    }
    QVERIFY(lifetime.isNull());
}

void KisSpinBoxUnitManagerContractTest::unitListsAndModelRolesRemainStable()
{
    const QStringList lengthSymbols({QStringLiteral("mm"),
                                     QStringLiteral("pt"),
                                     QStringLiteral("in"),
                                     QStringLiteral("cm"),
                                     QStringLiteral("dm"),
                                     QStringLiteral("pi"),
                                     QStringLiteral("cc")});

    KisSpinBoxUnitManager length;
    QCOMPARE(length.getsUnitSymbolList(), lengthSymbols);
    QCOMPARE(length.getsUnitSymbolList(true).size(), lengthSymbols.size());
    QCOMPARE(length.getApparentUnitId(), 1);
    QCOMPARE(length.getApparentUnitRecommendedDecimals(), 2);
    QCOMPARE(length.rowCount(), lengthSymbols.size());
    QCOMPARE(length.rowCount(length.index(0, 0)), 0);
    QCOMPARE(length.data(length.index(0, 0), Qt::DisplayRole).toString(), QStringLiteral("mm"));
    QVERIFY(!length.data(length.index(0, 0), Qt::ToolTipRole).toString().isEmpty());
    QVERIFY(!length.data(length.index(0, 0), Qt::UserRole).isValid());

    KisSpinBoxUnitManager imageLength;
    imageLength.setUnitDimension(KisSpinBoxUnitManager::IMLENGTH);
    QCOMPARE(imageLength.getsUnitSymbolList(), QStringList({QStringLiteral("px")}));
    QCOMPARE(imageLength.getApparentUnitId(), 0);
    QCOMPARE(imageLength.getApparentUnitRecommendedDecimals(), 0);

    KisSpinBoxUnitManager angle;
    angle.setUnitDimension(KisSpinBoxUnitManager::ANGLE);
    QCOMPARE(angle.getsUnitSymbolList(),
             QStringList({QString::fromUtf8("°"),
                          QStringLiteral("rad"),
                          QStringLiteral("gon"),
                          QStringLiteral("%")}));
    QCOMPARE(angle.getApparentUnitId(), 0);

    KisSpinBoxUnitManager time;
    time.setUnitDimension(KisSpinBoxUnitManager::TIME);
    QCOMPARE(time.getsUnitSymbolList(), QStringList({QStringLiteral("f")}));
    QCOMPARE(time.getApparentUnitSymbol(), QStringLiteral("frame"));
    QCOMPARE(time.getApparentUnitId(), -1);

    const QMetaMethod listChangedMethod = QMetaMethod::fromSignal(&KisSpinBoxUnitManager::unitListChanged);
    QVERIFY(listChangedMethod.isValid());
    QCOMPARE(listChangedMethod.methodSignature(), QByteArray("unitListChanged()"));

    KisSpinBoxUnitManager cached;
    QSignalSpy listChangedSpy(&cached, &KisSpinBoxUnitManager::unitListChanged);
    QCOMPARE(cached.getsUnitSymbolList(), lengthSymbols);
    cached.setUnitDimension(KisSpinBoxUnitManager::ANGLE);

    // Maintained characterization: dimension changes neither invalidate the list cache nor emit its signal.
    QCOMPARE(cached.getsUnitSymbolList(), lengthSymbols);
    QCOMPARE(listChangedSpy.count(), 0);
}

void KisSpinBoxUnitManagerContractTest::conversionFactorsConstantsAndValuesRemainStable()
{
    KisSpinBoxUnitManager manager;

    QCOMPARE(manager.getConversionFactor(KisSpinBoxUnitManager::LENGTH, QStringLiteral("pt")), 1.0);
    QVERIFY(qAbs(manager.getConversionFactor(KisSpinBoxUnitManager::LENGTH, QStringLiteral("in")) -
                 0.01388888888889) < 1e-14);
    QCOMPARE(manager.getConversionFactor(KisSpinBoxUnitManager::LENGTH, QStringLiteral("px")), -1.0);
    QCOMPARE(manager.getConversionFactor(KisSpinBoxUnitManager::IMLENGTH, QStringLiteral("px")), 1.0);
    QCOMPARE(manager.getConversionFactor(KisSpinBoxUnitManager::ANGLE, QString::fromUtf8("°")), 1.0);
    QCOMPARE(manager.getConversionFactor(KisSpinBoxUnitManager::ANGLE, QStringLiteral("gon")), 10.0 / 9.0);
    QCOMPARE(manager.getConversionFactor(KisSpinBoxUnitManager::ANGLE, QStringLiteral("%")), 2.5 / 9.0);
    QCOMPARE(manager.getConversionFactor(KisSpinBoxUnitManager::TIME, QStringLiteral("f")), 1.0);
    QCOMPARE(manager.getConversionFactor(99, QStringLiteral("unknown")), -1.0);
    QCOMPARE(manager.getConversionConstant(KisSpinBoxUnitManager::LENGTH, QStringLiteral("pt")), 0.0);

    manager.setUnitDimension(KisSpinBoxUnitManager::ANGLE);
    QSignalSpy factorChangedSpy(&manager, &KisSpinBoxUnitManager::conversionFactorChanged);
    QSignalSpy constantChangedSpy(&manager, &KisSpinBoxUnitManager::conversionConstantChanged);
    manager.setApparentUnitFromSymbol(QStringLiteral("gon"));

    QCOMPARE(factorChangedSpy.count(), 1);
    QCOMPARE(factorChangedSpy.at(0).at(0).toReal(), 10.0 / 9.0);
    QCOMPARE(factorChangedSpy.at(0).at(1).toReal(), 1.0);
    QCOMPARE(manager.getApparentValue(90.0), 100.0);
    QCOMPARE(manager.getReferenceValue(100.0), 90.0);
    QCOMPARE(constantChangedSpy.count(), 0);

    const QMetaMethod constantSignal = QMetaMethod::fromSignal(&KisSpinBoxUnitManager::conversionConstantChanged);
    QVERIFY(constantSignal.isValid());
    QCOMPARE(constantSignal.methodSignature(), QByteArray("conversionConstantChanged(double,double)"));
}

void KisSpinBoxUnitManagerContractTest::unitSelectionAndSignalsRemainStable()
{
    KisSpinBoxUnitManager manager;
    QSignalSpy dimensionChangedSpy(&manager, &KisSpinBoxUnitManager::unitDimensionChanged);
    QSignalSpy aboutToChangeSpy(&manager, &KisSpinBoxUnitManager::unitAboutToChange);
    QSignalSpy symbolChangedSpy(
        &manager,
        static_cast<void (KisSpinBoxUnitManager::*)(QString)>(&KisSpinBoxUnitManager::unitChanged));
    QSignalSpy indexChangedSpy(
        &manager,
        static_cast<void (KisSpinBoxUnitManager::*)(int)>(&KisSpinBoxUnitManager::unitChanged));

    manager.setUnitDimension(KisSpinBoxUnitManager::ANGLE);
    QCOMPARE(dimensionChangedSpy.count(), 1);
    QCOMPARE(dimensionChangedSpy.at(0).at(0).toInt(), int(KisSpinBoxUnitManager::ANGLE));
    QCOMPARE(manager.getReferenceUnitSymbol(), QString::fromUtf8("°"));
    QCOMPARE(manager.getApparentUnitSymbol(), QString::fromUtf8("°"));

    manager.setUnitDimension(KisSpinBoxUnitManager::ANGLE);
    QCOMPARE(dimensionChangedSpy.count(), 1);

    manager.setApparentUnitFromSymbol(QStringLiteral(" deg "));
    QCOMPARE(aboutToChangeSpy.count(), 1);
    QCOMPARE(symbolChangedSpy.count(), 1);
    QCOMPARE(symbolChangedSpy.at(0).at(0).toString(), QString::fromUtf8("°"));
    QCOMPARE(indexChangedSpy.count(), 1);
    QCOMPARE(indexChangedSpy.at(0).at(0).toInt(), 0);

    manager.selectApparentUnitFromIndex(2);
    QCOMPARE(manager.getApparentUnitSymbol(), QStringLiteral("gon"));
    QCOMPARE(indexChangedSpy.last().at(0).toInt(), 2);

    const int aboutBeforeInvalid = aboutToChangeSpy.count();
    const int symbolBeforeInvalid = symbolChangedSpy.count();
    manager.setApparentUnitFromSymbol(QStringLiteral("not-a-unit"));

    // Maintained characterization: an invalid symbol emits about-to-change before rejection.
    QCOMPARE(aboutToChangeSpy.count(), aboutBeforeInvalid + 1);
    QCOMPARE(symbolChangedSpy.count(), symbolBeforeInvalid);
    QCOMPARE(manager.getApparentUnitSymbol(), QStringLiteral("gon"));

    manager.selectApparentUnitFromIndex(-1);
    manager.selectApparentUnitFromIndex(manager.rowCount());
    QCOMPARE(manager.getApparentUnitSymbol(), QStringLiteral("gon"));
}

void KisSpinBoxUnitManagerContractTest::managerSynchronizationAndDisconnectRemainStable()
{
    KisSpinBoxUnitManager left;
    KisSpinBoxUnitManager right;
    left.setUnitDimension(KisSpinBoxUnitManager::ANGLE);
    right.setUnitDimension(KisSpinBoxUnitManager::ANGLE);

    left.syncWithOtherUnitManager(&right);
    left.selectApparentUnitFromIndex(2);
    QCOMPARE(left.getApparentUnitSymbol(), QStringLiteral("gon"));
    QCOMPARE(right.getApparentUnitSymbol(), QStringLiteral("gon"));

    left.syncWithOtherUnitManager(&right);
    QSignalSpy leftIndexChangedSpy(
        &left,
        static_cast<void (KisSpinBoxUnitManager::*)(int)>(&KisSpinBoxUnitManager::unitChanged));
    right.selectApparentUnitFromIndex(1);
    QCOMPARE(left.getApparentUnitSymbol(), QStringLiteral("rad"));
    QCOMPARE(leftIndexChangedSpy.count(), 1);

    left.clearSyncWithOtherUnitManager(&right);
    left.selectApparentUnitFromIndex(0);
    QCOMPARE(left.getApparentUnitSymbol(), QString::fromUtf8("°"));
    QCOMPARE(right.getApparentUnitSymbol(), QStringLiteral("rad"));

    left.clearSyncWithOtherUnitManager(&right);

    KisSpinBoxUnitManager otherDimension;
    otherDimension.setUnitDimension(KisSpinBoxUnitManager::TIME);
    left.syncWithOtherUnitManager(&otherDimension);
    left.selectApparentUnitFromIndex(2);
    QCOMPARE(otherDimension.getApparentUnitSymbol(), QStringLiteral("frame"));
}

QTEST_GUILESS_MAIN(KisSpinBoxUnitManagerContractTest)

#include "KisSpinBoxUnitManagerContractTest.moc"
