/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "flake/kis_dummies_facade.h"
#include "flake/kis_dummies_facade_base.h"
#include "nodes/kis_node_filter_proxy_model.h"

#include <QTest>

#include <type_traits>

namespace
{
class FacadeProbe final : public KisDummiesFacadeBase
{
public:
    using KisDummiesFacadeBase::KisDummiesFacadeBase;

    bool hasDummyForNode(KisNodeSP) const override;
    KisNodeDummy *dummyForNode(KisNodeSP) const override;
    KisNodeDummy *rootDummy() const override;
    int dummiesCount() const override;

protected:
    void addNodeImpl(KisNodeSP, KisNodeSP, KisNodeSP) override;
    void removeNodeImpl(KisNodeSP) override;
};
} // namespace

class KisDummiesFacadeBaseSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void imageAndActivationStateSignaturesRemainStable();
    void dummyLookupSignaturesRemainStable();
    void modelNotificationSignaturesRemainStable();
    void nodeFilterTypeConstructionAndLifetimeSchemaRemainStable();
    void nodeFilterModelAndDataSignaturesRemainStable();
    void nodeFilterCriteriaAndMappingSignaturesRemainStable();
    void nodeFilterActivationAndNotificationSignaturesRemainStable();
    void concreteFacadeTypeConstructionAndLifetimeSchemaRemainStable();
    void concreteFacadeDummyLookupSignaturesRemainStable();
};

void KisDummiesFacadeBaseSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Facade = KisDummiesFacadeBase;

    static_assert(std::is_class_v<Facade>);
    static_assert(std::is_base_of_v<QObject, Facade>);
    static_assert(std::is_abstract_v<Facade>);
    static_assert(std::is_constructible_v<FacadeProbe, QObject *>);
    static_assert(std::has_virtual_destructor_v<Facade>);
}

void KisDummiesFacadeBaseSchemaContractTest::imageAndActivationStateSignaturesRemainStable()
{
    using Facade = KisDummiesFacadeBase;
    using SetImage = void (Facade::*)(KisImageWSP);
    using SetImageAndNode = void (Facade::*)(KisImageWSP, KisNodeSP);

    static_assert(std::is_same_v<decltype(static_cast<SetImage>(&Facade::setImage)), SetImage>);
    static_assert(std::is_same_v<decltype(static_cast<SetImageAndNode>(&Facade::setImage)), SetImageAndNode>);
    static_assert(std::is_same_v<decltype(&Facade::lastActivatedNode), KisNodeSP (Facade::*)() const>);
    static_assert(std::is_same_v<decltype(&Facade::setLastActivatedNode), void (Facade::*)(KisNodeSP)>);
}

void KisDummiesFacadeBaseSchemaContractTest::dummyLookupSignaturesRemainStable()
{
    using Facade = KisDummiesFacadeBase;

    static_assert(std::is_same_v<decltype(&Facade::hasDummyForNode), bool (Facade::*)(KisNodeSP) const>);
    static_assert(std::is_same_v<decltype(&Facade::dummyForNode), KisNodeDummy *(Facade::*)(KisNodeSP) const>);
    static_assert(std::is_same_v<decltype(&Facade::rootDummy), KisNodeDummy *(Facade::*)() const>);
    static_assert(std::is_same_v<decltype(&Facade::dummiesCount), int (Facade::*)() const>);
}

void KisDummiesFacadeBaseSchemaContractTest::modelNotificationSignaturesRemainStable()
{
    using Facade = KisDummiesFacadeBase;

    static_assert(
        std::is_same_v<decltype(&Facade::sigBeginInsertDummy), void (Facade::*)(KisNodeDummy *, int, const QString &)>);
    static_assert(std::is_same_v<decltype(&Facade::sigEndInsertDummy), void (Facade::*)(KisNodeDummy *)>);
    static_assert(std::is_same_v<decltype(&Facade::sigBeginRemoveDummy), void (Facade::*)(KisNodeDummy *)>);
    static_assert(std::is_same_v<decltype(&Facade::sigEndRemoveDummy), void (Facade::*)()>);
    static_assert(std::is_same_v<decltype(&Facade::sigDummyChanged), void (Facade::*)(KisNodeDummy *)>);
    static_assert(std::is_same_v<decltype(&Facade::sigActivateNode), void (Facade::*)(KisNodeSP)>);
}

void KisDummiesFacadeBaseSchemaContractTest::nodeFilterTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Filter = KisNodeFilterProxyModel;

    static_assert(std::is_class_v<Filter>);
    static_assert(std::is_base_of_v<QSortFilterProxyModel, Filter>);
    static_assert(std::is_constructible_v<Filter, QObject *>);
    static_assert(std::has_virtual_destructor_v<Filter>);
}

void KisDummiesFacadeBaseSchemaContractTest::nodeFilterModelAndDataSignaturesRemainStable()
{
    using Filter = KisNodeFilterProxyModel;

    static_assert(std::is_same_v<decltype(&Filter::setNodeModel), void (Filter::*)(KisNodeModel *)>);
    static_assert(
        std::is_same_v<decltype(&Filter::setData), bool (Filter::*)(const QModelIndex &, const QVariant &, int)>);
    static_assert(
        std::is_same_v<decltype(&Filter::filterAcceptsRow), bool (Filter::*)(int, const QModelIndex &) const>);
}

void KisDummiesFacadeBaseSchemaContractTest::nodeFilterCriteriaAndMappingSignaturesRemainStable()
{
    using Filter = KisNodeFilterProxyModel;

    static_assert(std::is_same_v<decltype(&Filter::setAcceptedLabels), void (Filter::*)(const QSet<int> &)>);
    static_assert(std::is_same_v<decltype(&Filter::setTextFilter), void (Filter::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Filter::nodeFromIndex), KisNodeSP (Filter::*)(const QModelIndex &) const>);
    static_assert(std::is_same_v<decltype(&Filter::indexFromNode), QModelIndex (Filter::*)(KisNodeSP) const>);
}

void KisDummiesFacadeBaseSchemaContractTest::nodeFilterActivationAndNotificationSignaturesRemainStable()
{
    using Filter = KisNodeFilterProxyModel;

    static_assert(std::is_same_v<decltype(&Filter::setActiveNode), void (Filter::*)(KisNodeSP)>);
    static_assert(std::is_same_v<decltype(&Filter::unsetDummiesFacade), void (Filter::*)()>);
    static_assert(
        std::is_same_v<decltype(&Filter::sigBeforeBeginRemoveRows), void (Filter::*)(const QModelIndex &, int, int)>);
}

void KisDummiesFacadeBaseSchemaContractTest::concreteFacadeTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Facade = KisDummiesFacade;

    static_assert(std::is_class_v<Facade>);
    static_assert(std::is_base_of_v<KisDummiesFacadeBase, Facade>);
    static_assert(std::is_default_constructible_v<Facade>);
    static_assert(std::is_constructible_v<Facade, QObject *>);
    static_assert(std::has_virtual_destructor_v<Facade>);
}

void KisDummiesFacadeBaseSchemaContractTest::concreteFacadeDummyLookupSignaturesRemainStable()
{
    using Facade = KisDummiesFacade;

    static_assert(std::is_same_v<decltype(&Facade::hasDummyForNode), bool (Facade::*)(KisNodeSP) const>);
    static_assert(std::is_same_v<decltype(&Facade::dummyForNode), KisNodeDummy *(Facade::*)(KisNodeSP) const>);
    static_assert(std::is_same_v<decltype(&Facade::rootDummy), KisNodeDummy *(Facade::*)() const>);
    static_assert(std::is_same_v<decltype(&Facade::dummiesCount), int (Facade::*)() const>);
}

QTEST_GUILESS_MAIN(KisDummiesFacadeBaseSchemaContractTest)

#include "KisDummiesFacadeBaseSchemaContractTest.moc"
