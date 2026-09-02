/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kxmlguiclient.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_CLIENT_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisKXMLGUIClient::method)), signature>)
} // namespace

class KXMLGUIClientSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void xmlGuiClientIdentityAndStateTypesRemainStable();
    void xmlGuiClientStateActionSchemaRemainsStable();
    void xmlGuiClientDocumentAndComponentSignaturesRemainStable();
    void xmlGuiClientActionFactoryAndHierarchySignaturesRemainStable();
    void xmlGuiClientDynamicPlugSignaturesRemainStable();
};

void KXMLGUIClientSchemaContractTest::xmlGuiClientIdentityAndStateTypesRemainStable()
{
    using Client = KisKXMLGUIClient;

    static_assert(std::is_class_v<Client>);
    static_assert(std::is_constructible_v<Client>);
    static_assert(std::is_constructible_v<Client, Client *>);
    static_assert(std::has_virtual_destructor_v<Client>);
    static_assert(std::is_class_v<Client::StateChange>);

    QCOMPARE(static_cast<int>(Client::StateNoReverse), 0);
    QCOMPARE(static_cast<int>(Client::StateReverse), 1);

    Client::StateChange state;
    QVERIFY(state.actionsToEnable.isEmpty());
    QVERIFY(state.actionsToDisable.isEmpty());

    Client::StateChange copy = state;
    copy.actionsToEnable.append(QStringLiteral("copy-enable"));
    copy.actionsToDisable.append(QStringLiteral("copy-disable"));
    QVERIFY(state.actionsToEnable.isEmpty());
    QVERIFY(state.actionsToDisable.isEmpty());
    QCOMPARE(copy.actionsToEnable, QStringList{QStringLiteral("copy-enable")});
    QCOMPARE(copy.actionsToDisable, QStringList{QStringLiteral("copy-disable")});
}

void KXMLGUIClientSchemaContractTest::xmlGuiClientStateActionSchemaRemainsStable()
{
    using Client = KisKXMLGUIClient;

    static_assert(std::is_same_v<decltype(Client::StateChange::actionsToEnable), QStringList>);
    static_assert(std::is_same_v<decltype(Client::StateChange::actionsToDisable), QStringList>);
    ASSERT_CLIENT_SIGNATURE(addStateActionEnabled, void (Client::*)(const QString &, const QString &));
    ASSERT_CLIENT_SIGNATURE(addStateActionDisabled, void (Client::*)(const QString &, const QString &));
    ASSERT_CLIENT_SIGNATURE(getActionsToChangeForState, Client::StateChange (Client::*)(const QString &));
}

void KXMLGUIClientSchemaContractTest::xmlGuiClientDocumentAndComponentSignaturesRemainStable()
{
    using Client = KisKXMLGUIClient;

    ASSERT_CLIENT_SIGNATURE(componentName, QString (Client::*)() const);
    ASSERT_CLIENT_SIGNATURE(domDocument, QDomDocument (Client::*)() const);
    ASSERT_CLIENT_SIGNATURE(xmlFile, QString (Client::*)() const);
    ASSERT_CLIENT_SIGNATURE(localXMLFile, QString (Client::*)() const);
    ASSERT_CLIENT_SIGNATURE(setXMLGUIBuildDocument, void (Client::*)(const QDomDocument &));
    ASSERT_CLIENT_SIGNATURE(xmlguiBuildDocument, QDomDocument (Client::*)() const);
    ASSERT_CLIENT_SIGNATURE(reloadXML, void (Client::*)());
    ASSERT_CLIENT_SIGNATURE(replaceXMLFile, void (Client::*)(const QString &, const QString &, bool));
    static_assert(
        std::is_same_v<decltype(&Client::findMostRecentXMLFile), QString (*)(const QStringList &, QString &)>);

    static_assert(std::is_same_v<decltype(std::declval<Client &>().replaceXMLFile(std::declval<const QString &>(),
                                                                                  std::declval<const QString &>())),
                                 void>);
}

void KXMLGUIClientSchemaContractTest::xmlGuiClientActionFactoryAndHierarchySignaturesRemainStable()
{
    using Client = KisKXMLGUIClient;

    ASSERT_CLIENT_SIGNATURE(action, QAction * (Client::*)(const QDomElement &) const);
    ASSERT_CLIENT_SIGNATURE(action, QAction * (Client::*)(const char *) const);
    ASSERT_CLIENT_SIGNATURE(actionCollection, KisKActionCollection * (Client::*)() const);
    ASSERT_CLIENT_SIGNATURE(setFactory, void (Client::*)(KisKXMLGUIFactory *));
    ASSERT_CLIENT_SIGNATURE(factory, KisKXMLGUIFactory * (Client::*)() const);
    ASSERT_CLIENT_SIGNATURE(parentClient, Client * (Client::*)() const);
    ASSERT_CLIENT_SIGNATURE(insertChildClient, void (Client::*)(Client *));
    ASSERT_CLIENT_SIGNATURE(removeChildClient, void (Client::*)(Client *));
    ASSERT_CLIENT_SIGNATURE(childClients, QList<Client *> (Client::*)());
    ASSERT_CLIENT_SIGNATURE(setClientBuilder, void (Client::*)(KisKXMLGUIBuilder *));
    ASSERT_CLIENT_SIGNATURE(clientBuilder, KisKXMLGUIBuilder * (Client::*)() const);
}

void KXMLGUIClientSchemaContractTest::xmlGuiClientDynamicPlugSignaturesRemainStable()
{
    using Client = KisKXMLGUIClient;

    ASSERT_CLIENT_SIGNATURE(plugActionList, void (Client::*)(const QString &, const QList<QAction *> &));
    ASSERT_CLIENT_SIGNATURE(unplugActionList, void (Client::*)(const QString &));
    ASSERT_CLIENT_SIGNATURE(beginXMLPlug, void (Client::*)(QWidget *));
    ASSERT_CLIENT_SIGNATURE(endXMLPlug, void (Client::*)());
    ASSERT_CLIENT_SIGNATURE(prepareXMLUnplug, void (Client::*)(QWidget *));
}

QTEST_APPLESS_MAIN(KXMLGUIClientSchemaContractTest)

#include "KXMLGUIClientSchemaContractTest.moc"
