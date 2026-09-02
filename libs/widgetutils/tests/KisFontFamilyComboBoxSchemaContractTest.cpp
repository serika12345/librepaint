/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>
#include <kis_font_family_combo_box.h>

#include <type_traits>
#include <utility>

#define ASSERT_FONT_SELECTOR_SIGNATURE(type, method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

class KisFontFamilyComboBoxSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fontFamilySelectorTypeAndLifetimeSchemaRemainsStable();
    void fontFamilySelectorCurrentValueSchemaRemainsStable();
    void fontFamilySelectorPopulationSchemaRemainsStable();
    void fontFamilyComboPopulationSchemaRemainsStable();
    void pinnedFontsSeparatorPresentationSchemaRemainsStable();
};

void KisFontFamilyComboBoxSchemaContractTest::fontFamilySelectorTypeAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisFontComboBoxes>);
    static_assert(std::is_class_v<KisFontFamilyComboBox>);
    static_assert(std::is_class_v<PinnedFontsSeparator>);
    static_assert(std::is_constructible_v<KisFontComboBoxes, QWidget *>);
    static_assert(std::is_default_constructible_v<KisFontComboBoxes>);
    static_assert(std::is_constructible_v<KisFontFamilyComboBox, QWidget *>);
    static_assert(std::is_default_constructible_v<KisFontFamilyComboBox>);
    static_assert(std::is_constructible_v<PinnedFontsSeparator, QAbstractItemDelegate *, QWidget *>);
    static_assert(std::is_constructible_v<PinnedFontsSeparator, QAbstractItemDelegate *>);
}

void KisFontFamilyComboBoxSchemaContractTest::fontFamilySelectorCurrentValueSchemaRemainsStable()
{
    using W = KisFontComboBoxes;
    ASSERT_FONT_SELECTOR_SIGNATURE(W, setCurrentFont, void (W::*)(QFont));
    ASSERT_FONT_SELECTOR_SIGNATURE(W, setCurrentFamily, void (W::*)(QString));
    ASSERT_FONT_SELECTOR_SIGNATURE(W, setCurrentStyle, void (W::*)(QString));
    ASSERT_FONT_SELECTOR_SIGNATURE(W, currentFamily, QString (W::*)() const);
    ASSERT_FONT_SELECTOR_SIGNATURE(W, currentStyle, QString (W::*)() const);
    ASSERT_FONT_SELECTOR_SIGNATURE(W, currentFont, QFont (W::*)(int) const);
    static_assert(std::is_same_v<decltype(std::declval<const W &>().currentFont()), QFont>);
}

void KisFontFamilyComboBoxSchemaContractTest::fontFamilySelectorPopulationSchemaRemainsStable()
{
    using W = KisFontComboBoxes;
    using WritingSystems = QVector<QFontDatabase::WritingSystem>;
    ASSERT_FONT_SELECTOR_SIGNATURE(W, refillComboBox, void (W::*)(WritingSystems));
    static_assert(std::is_same_v<decltype(std::declval<W &>().refillComboBox()), void>);
    ASSERT_FONT_SELECTOR_SIGNATURE(W, setInitialized, void (W::*)());
    ASSERT_FONT_SELECTOR_SIGNATURE(W, fontChanged, void (W::*)(QString));
}

void KisFontFamilyComboBoxSchemaContractTest::fontFamilyComboPopulationSchemaRemainsStable()
{
    using W = KisFontFamilyComboBox;
    using WritingSystems = QVector<QFontDatabase::WritingSystem>;
    ASSERT_FONT_SELECTOR_SIGNATURE(W, refillComboBox, void (W::*)(WritingSystems));
    static_assert(std::is_same_v<decltype(std::declval<W &>().refillComboBox()), void>);
    ASSERT_FONT_SELECTOR_SIGNATURE(W, setTopFont, void (W::*)(const QString &));
    ASSERT_FONT_SELECTOR_SIGNATURE(W, setInitialized, void (W::*)());
}

void KisFontFamilyComboBoxSchemaContractTest::pinnedFontsSeparatorPresentationSchemaRemainsStable()
{
    using W = PinnedFontsSeparator;
    ASSERT_FONT_SELECTOR_SIGNATURE(W,
                                   paint,
                                   void (W::*)(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const);
    ASSERT_FONT_SELECTOR_SIGNATURE(W, setSeparatorIndex, void (W::*)(int));
    ASSERT_FONT_SELECTOR_SIGNATURE(W, setSeparatorAdded, void (W::*)());
    ASSERT_FONT_SELECTOR_SIGNATURE(W, sizeHint, QSize (W::*)(const QStyleOptionViewItem &, const QModelIndex &) const);
}

QTEST_GUILESS_MAIN(KisFontFamilyComboBoxSchemaContractTest)
#include "KisFontFamilyComboBoxSchemaContractTest.moc"
