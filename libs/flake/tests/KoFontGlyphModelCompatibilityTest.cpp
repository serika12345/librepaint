/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "text/KoFontGlyphModel.h"

#include <QTest>

// Compatibility requirement: GlyphPalette.qml depends on the openType, glyphLabel, and childCount model role names.
class KoFontGlyphModelCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void glyphPaletteRolesRemainAvailable();
};

void KoFontGlyphModelCompatibilityTest::glyphPaletteRolesRemainAvailable()
{
    // Consumer: the SVG text tool's glyph-palette QML delegates.
    // Operation: request the role names exposed by the glyph model.
    // Observable result: QML can resolve glyph rendering, labels, and variant indicators.
    // Failure impact: the glyph palette omits text, labels, or variant affordances.
    KoFontGlyphModel model;
    const QHash<int, QByteArray> roleNames = model.roleNames();

    QVERIFY(roleNames.values().contains(QByteArrayLiteral("openType")));
    QVERIFY(roleNames.values().contains(QByteArrayLiteral("glyphLabel")));
    QVERIFY(roleNames.values().contains(QByteArrayLiteral("childCount")));
}

QTEST_APPLESS_MAIN(KoFontGlyphModelCompatibilityTest)

#include "KoFontGlyphModelCompatibilityTest.moc"
