/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_model_index_converter_show_all.h"

#include <QTest>

#include <type_traits>

class KisModelIndexConverterShowAllSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void showAllIndexConverterSchemaRemainStable();
};

void KisModelIndexConverterShowAllSchemaContractTest::showAllIndexConverterSchemaRemainStable()
{
    using Converter = KisModelIndexConverterShowAll;

    static_assert(std::is_class_v<Converter>);
    static_assert(std::is_base_of_v<KisModelIndexConverterBase, Converter>);
    static_assert(std::is_constructible_v<Converter, KisDummiesFacadeBase *, KisNodeModel *>);
    static_assert(std::is_same_v<decltype(&Converter::dummyFromRow), KisNodeDummy *(Converter::*)(int, QModelIndex)>);
    static_assert(std::is_same_v<decltype(&Converter::dummyFromIndex), KisNodeDummy *(Converter::*)(QModelIndex)>);
    static_assert(std::is_same_v<decltype(&Converter::indexFromDummy), QModelIndex (Converter::*)(KisNodeDummy *)>);
    static_assert(std::is_same_v<decltype(&Converter::indexFromAddedDummy),
                                 bool (Converter::*)(KisNodeDummy *, int, const QString &, QModelIndex &, int &)>);
    static_assert(std::is_same_v<decltype(&Converter::rowCount), int (Converter::*)(QModelIndex)>);
}

QTEST_GUILESS_MAIN(KisModelIndexConverterShowAllSchemaContractTest)

#include "KisModelIndexConverterShowAllSchemaContractTest.moc"
