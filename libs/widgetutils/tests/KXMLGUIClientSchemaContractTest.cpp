/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kxmlguiclient.h>

#include <QTest>

#include <utility>

namespace
{
} // namespace

class KXMLGUIClientSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void xmlGuiClientIdentityAndStateTypesRemainStable();
};

void KXMLGUIClientSchemaContractTest::xmlGuiClientIdentityAndStateTypesRemainStable()
{
    using Client = KisKXMLGUIClient;


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

QTEST_APPLESS_MAIN(KXMLGUIClientSchemaContractTest)

#include "KXMLGUIClientSchemaContractTest.moc"
