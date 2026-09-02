/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoDocumentResourceManager.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_MANAGER_SIGNATURE(method, signature)                                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoDocumentResourceManager::method)), signature>)
} // namespace

class KoDocumentResourceManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void documentResourceManagerIdentityAndLifecycleSignaturesRemainStable();
    void documentResourceKeySchemaRemainsStable();
    void documentResourceValueAccessSignaturesRemainStable();
    void documentResourceDocumentStateSignaturesRemainStable();
    void documentResourceChangeSignalSignatureRemainsStable();
};

void KoDocumentResourceManagerSchemaContractTest::documentResourceManagerIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoDocumentResourceManager>);
    static_assert(std::is_base_of_v<QObject, KoDocumentResourceManager>);
    static_assert(std::is_default_constructible_v<KoDocumentResourceManager>);
    static_assert(std::has_virtual_destructor_v<KoDocumentResourceManager>);
}

void KoDocumentResourceManagerSchemaContractTest::documentResourceKeySchemaRemainsStable()
{
    static_assert(std::is_enum_v<KoDocumentResourceManager::DocumentResource>);
    QCOMPARE(int(KoDocumentResourceManager::UndoStack), 0);
    QCOMPARE(int(KoDocumentResourceManager::OdfDocument), 1);
    QCOMPARE(int(KoDocumentResourceManager::GrabSensitivity), 2);
    QCOMPARE(int(KoDocumentResourceManager::MarkerCollection), 3);
    QCOMPARE(int(KoDocumentResourceManager::DocumentResolution), 4);
    QCOMPARE(int(KoDocumentResourceManager::DocumentRectInPixels), 5);
    QCOMPARE(int(KoDocumentResourceManager::KarbonStart), 1000);
    QCOMPARE(int(KoDocumentResourceManager::KexiStart), 2000);
    QCOMPARE(int(KoDocumentResourceManager::FlowStart), 3000);
    QCOMPARE(int(KoDocumentResourceManager::PlanStart), 4000);
    QCOMPARE(int(KoDocumentResourceManager::StageStart), 5000);
    QCOMPARE(int(KoDocumentResourceManager::KritaStart), 6000);
    QCOMPARE(int(KoDocumentResourceManager::SheetsStart), 7000);
    QCOMPARE(int(KoDocumentResourceManager::WordsStart), 8000);
    QCOMPARE(int(KoDocumentResourceManager::KoPageAppStart), 9000);
    QCOMPARE(int(KoDocumentResourceManager::KoTextStart), 10000);
}

void KoDocumentResourceManagerSchemaContractTest::documentResourceValueAccessSignaturesRemainStable()
{
    ASSERT_MANAGER_SIGNATURE(setResource, void (KoDocumentResourceManager::*)(int, const QVariant &));
    ASSERT_MANAGER_SIGNATURE(setResource, void (KoDocumentResourceManager::*)(int, const KoColor &));
    ASSERT_MANAGER_SIGNATURE(setResource, void (KoDocumentResourceManager::*)(int, KoShape *));
    ASSERT_MANAGER_SIGNATURE(setResource, void (KoDocumentResourceManager::*)(int, const KoUnit &));
    ASSERT_MANAGER_SIGNATURE(resource, QVariant (KoDocumentResourceManager::*)(int) const);
    ASSERT_MANAGER_SIGNATURE(boolResource, bool (KoDocumentResourceManager::*)(int) const);
    ASSERT_MANAGER_SIGNATURE(intResource, int (KoDocumentResourceManager::*)(int) const);
    ASSERT_MANAGER_SIGNATURE(koColorResource, KoColor (KoDocumentResourceManager::*)(int) const);
    ASSERT_MANAGER_SIGNATURE(koShapeResource, KoShape * (KoDocumentResourceManager::*)(int) const);
    ASSERT_MANAGER_SIGNATURE(stringResource, QString (KoDocumentResourceManager::*)(int) const);
    ASSERT_MANAGER_SIGNATURE(sizeResource, QSizeF (KoDocumentResourceManager::*)(int) const);
    ASSERT_MANAGER_SIGNATURE(unitResource, KoUnit (KoDocumentResourceManager::*)(int) const);
    ASSERT_MANAGER_SIGNATURE(hasResource, bool (KoDocumentResourceManager::*)(int) const);
    ASSERT_MANAGER_SIGNATURE(clearResource, void (KoDocumentResourceManager::*)(int));
}

void KoDocumentResourceManagerSchemaContractTest::documentResourceDocumentStateSignaturesRemainStable()
{
    ASSERT_MANAGER_SIGNATURE(setGrabSensitivity, void (KoDocumentResourceManager::*)(int));
    ASSERT_MANAGER_SIGNATURE(grabSensitivity, int (KoDocumentResourceManager::*)() const);
    ASSERT_MANAGER_SIGNATURE(setUndoStack, void (KoDocumentResourceManager::*)(KUndo2Stack *));
    ASSERT_MANAGER_SIGNATURE(undoStack, KUndo2Stack * (KoDocumentResourceManager::*)() const);
    ASSERT_MANAGER_SIGNATURE(documentResolution, qreal (KoDocumentResourceManager::*)() const);
    ASSERT_MANAGER_SIGNATURE(documentRectInPixels, QRectF (KoDocumentResourceManager::*)() const);
}

void KoDocumentResourceManagerSchemaContractTest::documentResourceChangeSignalSignatureRemainsStable()
{
    ASSERT_MANAGER_SIGNATURE(resourceChanged, void (KoDocumentResourceManager::*)(int, const QVariant &));
}

QTEST_APPLESS_MAIN(KoDocumentResourceManagerSchemaContractTest)

#include "KoDocumentResourceManagerSchemaContractTest.moc"
