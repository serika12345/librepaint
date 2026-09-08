/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "document/kis_image_manager.h"

#include <QTest>

#include <type_traits>

#define ASSERT_IMAGE_MANAGER_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisImageManager::method)), signature>)

class KisImageManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageManagerTypeConstructionAndLifetimeSchemaRemainStable();
    void imageManagerViewSetupAndDialogSignaturesRemainStable();
    void imageManagerLayerImportSignaturesRemainStable();
    void imageManagerExternalTransferAndDropSignaturesRemainStable();
    void imageManagerGeometryTransformationSignaturesRemainStable();
};

void KisImageManagerSchemaContractTest::imageManagerTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisImageManager>);
    static_assert(std::is_constructible_v<KisImageManager, KisViewManager *>);
    static_assert(std::has_virtual_destructor_v<KisImageManager>);
}

void KisImageManagerSchemaContractTest::imageManagerViewSetupAndDialogSignaturesRemainStable()
{
    ASSERT_IMAGE_MANAGER_SIGNATURE(setView, void (KisImageManager::*)(QPointer<KisView>));
    ASSERT_IMAGE_MANAGER_SIGNATURE(setup, void (KisImageManager::*)(KisActionManager *));
    ASSERT_IMAGE_MANAGER_SIGNATURE(slotImageColor, void (KisImageManager::*)());
    ASSERT_IMAGE_MANAGER_SIGNATURE(slotImageProperties, void (KisImageManager::*)());
}

void KisImageManagerSchemaContractTest::imageManagerLayerImportSignaturesRemainStable()
{
    ASSERT_IMAGE_MANAGER_SIGNATURE(importImage, qint32 (KisImageManager::*)(const QUrl &, const QString &));
    ASSERT_IMAGE_MANAGER_SIGNATURE(slotImportLayerAsFilterMask, void (KisImageManager::*)());
    ASSERT_IMAGE_MANAGER_SIGNATURE(slotImportLayerAsSelectionMask, void (KisImageManager::*)());
    ASSERT_IMAGE_MANAGER_SIGNATURE(slotImportLayerAsTransparencyMask, void (KisImageManager::*)());
    ASSERT_IMAGE_MANAGER_SIGNATURE(slotImportLayerFromFile, void (KisImageManager::*)());
}

void KisImageManagerSchemaContractTest::imageManagerExternalTransferAndDropSignaturesRemainStable()
{
    ASSERT_IMAGE_MANAGER_SIGNATURE(adaptClipToImageColorSpace, void (*)(KisPaintDeviceSP, KisImageWSP));
    ASSERT_IMAGE_MANAGER_SIGNATURE(fetchRemoteFile, bool (*)(const QUrl &, QTemporaryFile *));
    ASSERT_IMAGE_MANAGER_SIGNATURE(handleImageDrop, void (KisImageManager::*)(KisView *, QDropEvent *, const QPoint &));
    ASSERT_IMAGE_MANAGER_SIGNATURE(importFileFormatError, QString (*)(const QString &));
}

void KisImageManagerSchemaContractTest::imageManagerGeometryTransformationSignaturesRemainStable()
{
    ASSERT_IMAGE_MANAGER_SIGNATURE(resizeCurrentImage, void (KisImageManager::*)(qint32, qint32, qint32, qint32));
    ASSERT_IMAGE_MANAGER_SIGNATURE(rotateCurrentImage, void (KisImageManager::*)(double));
    ASSERT_IMAGE_MANAGER_SIGNATURE(scaleCurrentImage,
                                   void (KisImageManager::*)(const QSize &, qreal, qreal, KisFilterStrategy *));
    ASSERT_IMAGE_MANAGER_SIGNATURE(shearCurrentImage, void (KisImageManager::*)(double, double));
}

QTEST_APPLESS_MAIN(KisImageManagerSchemaContractTest)

#include "KisImageManagerSchemaContractTest.moc"
