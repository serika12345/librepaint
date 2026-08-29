/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <type_traits>

#include <QTest>

#include <KisFigurePaintingOptions.h>

class KisFigurePaintingOptionsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fillStylesKeepTheirSharedOrdinals();
    void strokeStylesKeepTheirSharedOrdinals();
};

void KisFigurePaintingOptionsContractTest::fillStylesKeepTheirSharedOrdinals()
{
    using namespace KisFigurePaintingOptions;

    static_assert(std::is_enum_v<FillStyle>);
    static_assert(!std::is_same_v<FillStyle, StrokeStyle>);

    QCOMPARE(static_cast<int>(FillStyleNone), 0);
    QCOMPARE(static_cast<int>(FillStyleForegroundColor), 1);
    QCOMPARE(static_cast<int>(FillStyleBackgroundColor), 2);
    QCOMPARE(static_cast<int>(FillStylePattern), 3);
}

void KisFigurePaintingOptionsContractTest::strokeStylesKeepTheirSharedOrdinals()
{
    using namespace KisFigurePaintingOptions;

    static_assert(std::is_enum_v<StrokeStyle>);

    QCOMPARE(static_cast<int>(StrokeStyleNone), 0);
    QCOMPARE(static_cast<int>(StrokeStyleForeground), 1);
    QCOMPARE(static_cast<int>(StrokeStyleBackground), 2);
}

QTEST_GUILESS_MAIN(KisFigurePaintingOptionsContractTest)

#include "KisFigurePaintingOptionsContractTest.moc"
