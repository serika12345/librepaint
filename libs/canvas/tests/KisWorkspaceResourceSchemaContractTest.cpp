/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>

#include "workspace/kis_workspace_resource.h"

class KisWorkspaceResourceSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void workspaceResourceTypeAndLifetimeSchemaRemainsStable();
    void workspaceResourceCloneAndIdentitySchemaRemainsStable();
    void workspaceResourceDeviceIoSchemaRemainsStable();
    void workspaceResourceDockerStateSchemaRemainsStable();
};

void KisWorkspaceResourceSchemaContractTest::workspaceResourceTypeAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisWorkspaceResource>);
    static_assert(std::is_same_v<KisWorkspaceResourceSP, QSharedPointer<KisWorkspaceResource>>);
    static_assert(std::is_constructible_v<KisWorkspaceResource, const QString &>);
    static_assert(std::is_copy_constructible_v<KisWorkspaceResource>);
    static_assert(!std::is_copy_assignable_v<KisWorkspaceResource>);
    static_assert(std::has_virtual_destructor_v<KisWorkspaceResource>);

    QVERIFY(true);
}

void KisWorkspaceResourceSchemaContractTest::workspaceResourceCloneAndIdentitySchemaRemainsStable()
{
    using Clone = KoResourceSP (KisWorkspaceResource::*)() const;
    using DefaultFileExtension = QString (KisWorkspaceResource::*)() const;
    using ResourceType = QPair<QString, QString> (KisWorkspaceResource::*)() const;

    static_assert(std::is_same_v<decltype(&KisWorkspaceResource::clone), Clone>);
    static_assert(std::is_same_v<decltype(&KisWorkspaceResource::defaultFileExtension), DefaultFileExtension>);
    static_assert(std::is_same_v<decltype(&KisWorkspaceResource::resourceType), ResourceType>);

    QVERIFY(true);
}

void KisWorkspaceResourceSchemaContractTest::workspaceResourceDeviceIoSchemaRemainsStable()
{
    using LoadFromDevice = bool (KisWorkspaceResource::*)(QIODevice *, KisResourcesInterfaceSP);
    using SaveToDevice = bool (KisWorkspaceResource::*)(QIODevice *) const;

    static_assert(std::is_same_v<decltype(&KisWorkspaceResource::loadFromDevice), LoadFromDevice>);
    static_assert(std::is_same_v<decltype(&KisWorkspaceResource::saveToDevice), SaveToDevice>);

    QVERIFY(true);
}

void KisWorkspaceResourceSchemaContractTest::workspaceResourceDockerStateSchemaRemainsStable()
{
    using SetDockerState = void (KisWorkspaceResource::*)(const QByteArray &);
    using DockerState = QByteArray (KisWorkspaceResource::*)();

    static_assert(std::is_same_v<decltype(&KisWorkspaceResource::setDockerState), SetDockerState>);
    static_assert(std::is_same_v<decltype(&KisWorkspaceResource::dockerState), DockerState>);

    QVERIFY(true);
}

QTEST_MAIN(KisWorkspaceResourceSchemaContractTest)

#include "KisWorkspaceResourceSchemaContractTest.moc"
