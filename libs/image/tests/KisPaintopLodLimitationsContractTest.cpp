/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/kis_paintop_lod_limitations.h"

#include <QTest>

class KisPaintopLodLimitationsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void equalityAndHashingUseIdentifierCollections();
    void mergeUnitesLimitationsAndBlockers();
};

void KisPaintopLodLimitationsContractTest::equalityAndHashingUseIdentifierCollections()
{
    KisPaintopLodLimitations first;
    KisPaintopLodLimitations second;

    QVERIFY(first.limitations.isEmpty());
    QVERIFY(first.blockers.isEmpty());
    QCOMPARE(first, second);

    const KoID firstName(QStringLiteral("stable-id"), QStringLiteral("First name"));
    const KoID secondName(QStringLiteral("stable-id"), QStringLiteral("Second name"));
    QCOMPARE(qHash(firstName), qHash(secondName));
    QCOMPARE(qHash(firstName), static_cast<uint>(qHash(QStringLiteral("stable-id"))));

    first.limitations.insert(firstName);
    QVERIFY(first != second);
    second.limitations.insert(secondName);
    QCOMPARE(first, second);
}

void KisPaintopLodLimitationsContractTest::mergeUnitesLimitationsAndBlockers()
{
    const KoID sizeLimit(QStringLiteral("size"), QStringLiteral("Size"));
    const KoID textureLimit(QStringLiteral("texture"), QStringLiteral("Texture"));
    const KoID timingBlocker(QStringLiteral("timing"), QStringLiteral("Timing"));
    const KoID spacingBlocker(QStringLiteral("spacing"), QStringLiteral("Spacing"));

    KisPaintopLodLimitations destination;
    destination.limitations.insert(sizeLimit);
    destination.blockers.insert(timingBlocker);

    KisPaintopLodLimitations source;
    source.limitations.insert(sizeLimit);
    source.limitations.insert(textureLimit);
    source.blockers.insert(spacingBlocker);

    KisPaintopLodLimitations *const result = &(destination |= source);

    QCOMPARE(result, &destination);
    QCOMPARE(destination.limitations, (QSet<KoID>{sizeLimit, textureLimit}));
    QCOMPARE(destination.blockers, (QSet<KoID>{timingBlocker, spacingBlocker}));
}

QTEST_GUILESS_MAIN(KisPaintopLodLimitationsContractTest)

#include "KisPaintopLodLimitationsContractTest.moc"
