/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_categorized_item_delegate.h"

#include <QTest>

#include <type_traits>

class KisCategorizedItemDelegateSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void delegateTypeConstructionAndPresentationSchemaRemainStable();
};

void KisCategorizedItemDelegateSchemaContractTest::delegateTypeConstructionAndPresentationSchemaRemainStable()
{
    using Delegate = KisCategorizedItemDelegate;

    static_assert(std::is_base_of_v<QStyledItemDelegate, Delegate>);
    static_assert(std::is_constructible_v<Delegate, QObject *>);
    static_assert(std::has_virtual_destructor_v<Delegate>);
    static_assert(
        std::is_same_v<decltype(&Delegate::paint),
                       void (Delegate::*)(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const>);
    static_assert(std::is_same_v<decltype(&Delegate::sizeHint),
                                 QSize (Delegate::*)(const QStyleOptionViewItem &, const QModelIndex &) const>);
}

QTEST_GUILESS_MAIN(KisCategorizedItemDelegateSchemaContractTest)

#include "KisCategorizedItemDelegateSchemaContractTest.moc"
