/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <text/KoFontGlyphModel.h>

#include <QTest>

#include <utility>

namespace
{
using FeatureInfoMap = QMap<QString, KoOpenTypeFeatureInfo>;
using RoleNameMap = QHash<int, QByteArray>;

} // namespace

class KoFontGlyphModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fontGlyphModelRoleAndGlyphTypeValuesRemainStable();
};

void KoFontGlyphModelSchemaContractTest::fontGlyphModelRoleAndGlyphTypeValuesRemainStable()
{
    QCOMPARE(int(KoFontGlyphModel::Base), 0);
    QCOMPARE(int(KoFontGlyphModel::UnicodeVariationSelector), 1);
    QCOMPARE(int(KoFontGlyphModel::OpenType), 2);

    QCOMPARE(int(KoFontGlyphModel::OpenTypeFeatures), int(Qt::UserRole) + 1);
    QCOMPARE(int(KoFontGlyphModel::GlyphLabel), int(Qt::UserRole) + 2);
    QCOMPARE(int(KoFontGlyphModel::ChildCount), int(Qt::UserRole) + 3);
}

QTEST_APPLESS_MAIN(KoFontGlyphModelSchemaContractTest)

#include "KoFontGlyphModelSchemaContractTest.moc"
