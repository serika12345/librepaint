/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisHighlightedToolButton.h"

#include <QTest>
#include <QWidget>

namespace
{
QPalette paletteWithButtonColors(const QColor &button, const QColor &highlight)
{
    QPalette palette;
    palette.setColor(QPalette::Button, button);
    palette.setColor(QPalette::Highlight, highlight);
    return palette;
}
} // namespace

class KisHighlightedToolButtonContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void checkStateSelectsParentPaletteColor();
    void ownPaletteChangeRestoresCurrentParentColor();
};

void KisHighlightedToolButtonContractTest::checkStateSelectsParentPaletteColor()
{
    const QColor buttonColor(31, 47, 59);
    const QColor highlightColor(83, 109, 137);
    QWidget parent;
    parent.setPalette(paletteWithButtonColors(buttonColor, highlightColor));

    KisHighlightedToolButton button(&parent);
    button.setCheckable(true);

    QCOMPARE(button.palette().color(QPalette::Button), buttonColor);

    button.setChecked(true);
    QCOMPARE(button.palette().color(QPalette::Button), highlightColor);

    button.setChecked(false);
    QCOMPARE(button.palette().color(QPalette::Button), buttonColor);
}

void KisHighlightedToolButtonContractTest::ownPaletteChangeRestoresCurrentParentColor()
{
    const QColor buttonColor(37, 53, 71);
    const QColor highlightColor(89, 107, 131);
    QWidget parent;
    parent.setPalette(paletteWithButtonColors(buttonColor, highlightColor));

    KisHighlightedToolButton button(&parent);
    button.setCheckable(true);
    button.setChecked(true);

    QPalette unrelatedPalette;
    unrelatedPalette.setColor(QPalette::Button, QColor(211, 223, 227));
    button.setPalette(unrelatedPalette);

    QCOMPARE(button.palette().color(QPalette::Button), highlightColor);
}

QTEST_MAIN(KisHighlightedToolButtonContractTest)

#include "KisHighlightedToolButtonContractTest.moc"
