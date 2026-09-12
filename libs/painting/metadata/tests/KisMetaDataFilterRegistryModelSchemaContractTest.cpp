/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_meta_data_filter_registry_model.h"

#include <QTest>

#include <type_traits>

class KisMetaDataFilterRegistryModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void modelDataAndEditingSignaturesRemainStable();
    void enabledFilterSignaturesRemainStable();
};

void KisMetaDataFilterRegistryModelSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Model = KisMetaData::FilterRegistryModel;

    static_assert(std::is_class_v<Model>);
    static_assert(std::is_default_constructible_v<Model>);
    static_assert(std::is_destructible_v<Model>);
}

void KisMetaDataFilterRegistryModelSchemaContractTest::modelDataAndEditingSignaturesRemainStable()
{
    using Model = KisMetaData::FilterRegistryModel;
    using Data = QVariant (Model::*)(const QModelIndex &, int) const;
    using Flags = Qt::ItemFlags (Model::*)(const QModelIndex &) const;
    using SetData = bool (Model::*)(const QModelIndex &, const QVariant &, int);

    static_assert(std::is_same_v<decltype(&Model::data), Data>);
    static_assert(std::is_same_v<decltype(&Model::flags), Flags>);
    static_assert(std::is_same_v<decltype(&Model::setData), SetData>);
}

void KisMetaDataFilterRegistryModelSchemaContractTest::enabledFilterSignaturesRemainStable()
{
    using Model = KisMetaData::FilterRegistryModel;
    using Filter = KisMetaData::Filter;
    using EnabledFilters = QList<const Filter *> (Model::*)() const;
    using SetEnabledFilters = void (Model::*)(const QStringList &);

    static_assert(std::is_same_v<decltype(&Model::enabledFilters), EnabledFilters>);
    static_assert(std::is_same_v<decltype(&Model::setEnabledFilters), SetEnabledFilters>);
}

QTEST_GUILESS_MAIN(KisMetaDataFilterRegistryModelSchemaContractTest)

#include "KisMetaDataFilterRegistryModelSchemaContractTest.moc"
