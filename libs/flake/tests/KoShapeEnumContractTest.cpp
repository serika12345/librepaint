/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>

#include <QTest>

#include <array>

class KoShapeEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void changeTypeValuesRemainStable();
    void paintOrderValuesRemainStable();
    void childZOrderValuesRemainStable();
};

void KoShapeEnumContractTest::changeTypeValuesRemainStable()
{
    const std::array<KoShape::ChangeType, 20> changes {{
        KoShape::PositionChanged,
        KoShape::RotationChanged,
        KoShape::ScaleChanged,
        KoShape::ShearChanged,
        KoShape::SizeChanged,
        KoShape::GenericMatrixChange,
        KoShape::KeepAspectRatioChange,
        KoShape::ParentChanged,
        KoShape::Deleted,
        KoShape::StrokeChanged,
        KoShape::BackgroundChanged,
        KoShape::BorderChanged,
        KoShape::ParameterChanged,
        KoShape::ContentChanged,
        KoShape::TextContourMarginChanged,
        KoShape::ChildChanged,
        KoShape::ConnectionPointChanged,
        KoShape::ClipPathChanged,
        KoShape::ClipMaskChanged,
        KoShape::TransparencyChanged,
    }};

    for (std::size_t index = 0; index < changes.size(); ++index) {
        QCOMPARE(int(changes[index]), int(index));
    }
}

void KoShapeEnumContractTest::paintOrderValuesRemainStable()
{
    QCOMPARE(int(KoShape::Fill), 0);
    QCOMPARE(int(KoShape::Stroke), 1);
    QCOMPARE(int(KoShape::Markers), 2);
}

void KoShapeEnumContractTest::childZOrderValuesRemainStable()
{
    QCOMPARE(int(KoShape::ChildZDefault), 0);
    QCOMPARE(int(KoShape::ChildZParentChild), int(KoShape::ChildZDefault));
    QCOMPARE(int(KoShape::ChildZPassThrough), 1);
}

QTEST_GUILESS_MAIN(KoShapeEnumContractTest)

#include "KoShapeEnumContractTest.moc"
