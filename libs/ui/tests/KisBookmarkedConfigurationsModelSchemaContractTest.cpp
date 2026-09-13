/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <tool/kis_bookmarked_configurations_editor.h>
#include <tool/kis_bookmarked_configurations_model.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KisBookmarkedConfigurationsModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionLifetimeAndManagerSchemaRemainStable();
    void rowDataAndFlagsSchemaRemainStable();
    void configurationLookupSchemaRemainStable();
    void configurationMutationSchemaRemainStable();
    void bookmarkedConfigurationsEditorSchemaRemainStable();
};

void KisBookmarkedConfigurationsModelSchemaContractTest::typeConstructionLifetimeAndManagerSchemaRemainStable()
{
    using Model = KisBookmarkedConfigurationsModel;

    static_assert(std::is_class_v<Model>);
    static_assert(std::is_base_of_v<QAbstractListModel, Model>);
    static_assert(std::is_constructible_v<Model, KisBookmarkedConfigurationManager *>);
    static_assert(std::has_virtual_destructor_v<Model>);
    static_assert(std::is_same_v<decltype(&Model::bookmarkedConfigurationManager),
                                 KisBookmarkedConfigurationManager *(Model::*)()>);
}

void KisBookmarkedConfigurationsModelSchemaContractTest::rowDataAndFlagsSchemaRemainStable()
{
    using Model = KisBookmarkedConfigurationsModel;

    static_assert(std::is_same_v<decltype(&Model::rowCount), int (Model::*)(const QModelIndex &) const>);
    static_assert(std::is_same_v<decltype(std::declval<const Model &>().rowCount()), int>);
    static_assert(std::is_same_v<decltype(&Model::data), QVariant (Model::*)(const QModelIndex &, int) const>);
    static_assert(
        std::is_same_v<decltype(std::declval<const Model &>().data(std::declval<const QModelIndex &>())), QVariant>);
    static_assert(std::is_same_v<decltype(&Model::flags), Qt::ItemFlags (Model::*)(const QModelIndex &) const>);
}

void KisBookmarkedConfigurationsModelSchemaContractTest::configurationLookupSchemaRemainStable()
{
    using Model = KisBookmarkedConfigurationsModel;

    static_assert(std::is_same_v<decltype(&Model::configuration),
                                 KisSerializableConfigurationSP (Model::*)(const QModelIndex &) const>);
    static_assert(std::is_same_v<decltype(&Model::indexFor), QModelIndex (Model::*)(const QString &) const>);
    static_assert(std::is_same_v<decltype(&Model::isIndexDeletable), bool (Model::*)(const QModelIndex &) const>);
}

void KisBookmarkedConfigurationsModelSchemaContractTest::configurationMutationSchemaRemainStable()
{
    using Model = KisBookmarkedConfigurationsModel;

    static_assert(std::is_same_v<decltype(&Model::newConfiguration),
                                 void (Model::*)(KLocalizedString, KisSerializableConfigurationSP)>);
    static_assert(std::is_same_v<decltype(&Model::saveConfiguration),
                                 void (Model::*)(const QString &, KisSerializableConfigurationSP)>);
    static_assert(std::is_same_v<decltype(&Model::deleteIndex), void (Model::*)(const QModelIndex &)>);
    static_assert(
        std::is_same_v<decltype(&Model::setData), bool (Model::*)(const QModelIndex &, const QVariant &, int)>);
    static_assert(std::is_same_v<decltype(std::declval<Model &>().setData(std::declval<const QModelIndex &>(),
                                                                          std::declval<const QVariant &>())),
                                 bool>);
}

void KisBookmarkedConfigurationsModelSchemaContractTest::bookmarkedConfigurationsEditorSchemaRemainStable()
{
    using Editor = KisBookmarkedConfigurationsEditor;

    static_assert(std::is_class_v<Editor>);
    static_assert(std::is_base_of_v<QDialog, Editor>);
    static_assert(
        std::is_constructible_v<Editor, QWidget *, KisBookmarkedConfigurationsModel *, KisSerializableConfigurationSP>);
    static_assert(std::has_virtual_destructor_v<Editor>);
}

QTEST_GUILESS_MAIN(KisBookmarkedConfigurationsModelSchemaContractTest)

#include "KisBookmarkedConfigurationsModelSchemaContractTest.moc"
