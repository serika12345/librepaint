/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <ui/workspace/KisWindowLayoutResource.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_WINDOW_LAYOUT_RESOURCE_SIGNATURE(method, signature)                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisWindowLayoutResource::method)), signature>)
} // namespace

class KisWindowLayoutResourceSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void copyAndAssignmentSchemaRemainStable();
    void windowCreationAndApplicationSignaturesRemainStable();
    void devicePersistenceSignaturesRemainStable();
    void resourceIdentitySignaturesRemainStable();
};

void KisWindowLayoutResourceSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Resource = KisWindowLayoutResource;

    static_assert(std::is_same_v<KisWindowLayoutResourceSP, QSharedPointer<Resource>>);
    static_assert(std::is_class_v<Resource>);
    static_assert(std::is_constructible_v<Resource, const QString &>);
    static_assert(std::is_copy_constructible_v<Resource>);
    static_assert(std::has_virtual_destructor_v<Resource>);
}

void KisWindowLayoutResourceSchemaContractTest::copyAndAssignmentSchemaRemainStable()
{
    using Resource = KisWindowLayoutResource;

    ASSERT_WINDOW_LAYOUT_RESOURCE_SIGNATURE(clone, KoResourceSP (Resource::*)() const);
    static_assert(!std::is_copy_assignable_v<Resource>);
}

void KisWindowLayoutResourceSchemaContractTest::windowCreationAndApplicationSignaturesRemainStable()
{
    using Resource = KisWindowLayoutResource;
    using FromCurrentWindowsSignature = KisWindowLayoutResourceSP (*)(const QString &,
                                                                      const QList<QPointer<KisMainWindow>> &,
                                                                      bool,
                                                                      bool,
                                                                      KisMainWindow *);

    static_assert(std::is_same_v<decltype(static_cast<FromCurrentWindowsSignature>(&Resource::fromCurrentWindows)),
                                 FromCurrentWindowsSignature>);
    ASSERT_WINDOW_LAYOUT_RESOURCE_SIGNATURE(applyLayout, void (Resource::*)());
}

void KisWindowLayoutResourceSchemaContractTest::devicePersistenceSignaturesRemainStable()
{
    using Resource = KisWindowLayoutResource;

    ASSERT_WINDOW_LAYOUT_RESOURCE_SIGNATURE(saveToDevice, bool (Resource::*)(QIODevice *) const);
    ASSERT_WINDOW_LAYOUT_RESOURCE_SIGNATURE(loadFromDevice, bool (Resource::*)(QIODevice *, KisResourcesInterfaceSP));
}

void KisWindowLayoutResourceSchemaContractTest::resourceIdentitySignaturesRemainStable()
{
    using Resource = KisWindowLayoutResource;
    using ResourceTypeResult = QPair<QString, QString>;

    ASSERT_WINDOW_LAYOUT_RESOURCE_SIGNATURE(resourceType, ResourceTypeResult (Resource::*)() const);
    ASSERT_WINDOW_LAYOUT_RESOURCE_SIGNATURE(defaultFileExtension, QString (Resource::*)() const);
}

#undef ASSERT_WINDOW_LAYOUT_RESOURCE_SIGNATURE

QTEST_GUILESS_MAIN(KisWindowLayoutResourceSchemaContractTest)

#include "KisWindowLayoutResourceSchemaContractTest.moc"
