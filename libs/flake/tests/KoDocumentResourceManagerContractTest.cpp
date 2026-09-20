/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoDocumentResourceManager.h>

#include <QSignalSpy>
#include <QTest>

class KoDocumentResourceManagerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void documentGeometryIsAvailableAndNotifiesConsumers();
    void grabSensitivityHasASafeMinimum();
};

void KoDocumentResourceManagerContractTest::documentGeometryIsAvailableAndNotifiesConsumers()
{
    // Consumer: Shape controllers and shape-editing widgets read the document geometry.
    // Operation: Update the document resolution and pixel bounds.
    // Observable result: Readers receive the new values and one resourceChanged signal per update.
    // Failure impact: Shape tools use stale document geometry and place or size editing affordances incorrectly.
    KoDocumentResourceManager manager;
    QSignalSpy resourceChanges(&manager, &KoDocumentResourceManager::resourceChanged);

    const qreal resolution = 144.0;
    const QRectF bounds(10.0, 20.0, 640.0, 480.0);

    manager.setResource(KoDocumentResourceManager::DocumentResolution, resolution);
    manager.setResource(KoDocumentResourceManager::DocumentRectInPixels, bounds);

    QCOMPARE(manager.documentResolution(), resolution);
    QCOMPARE(manager.documentRectInPixels(), bounds);
    QCOMPARE(resourceChanges.count(), 2);
    QCOMPARE(resourceChanges.at(0).at(0).toInt(), int(KoDocumentResourceManager::DocumentResolution));
    QCOMPARE(resourceChanges.at(0).at(1).toReal(), resolution);
    QCOMPARE(resourceChanges.at(1).at(0).toInt(), int(KoDocumentResourceManager::DocumentRectInPixels));
    QCOMPARE(resourceChanges.at(1).at(1).toRectF(), bounds);
}

void KoDocumentResourceManagerContractTest::grabSensitivityHasASafeMinimum()
{
    // Consumer: Shape interaction strategies use the grab sensitivity to select handles with the pointer.
    // Operation: Set a value below the usable minimum, then a valid value.
    // Observable result: The minimum stays at five pixels and a valid requested sensitivity is retained.
    // Failure impact: Users cannot reliably select shape handles or get an unexpectedly large selection area.
    KoDocumentResourceManager manager;

    manager.setGrabSensitivity(2);
    QCOMPARE(manager.grabSensitivity(), 5);

    manager.setGrabSensitivity(12);
    QCOMPARE(manager.grabSensitivity(), 12);
}

QTEST_APPLESS_MAIN(KoDocumentResourceManagerContractTest)

#include "KoDocumentResourceManagerContractTest.moc"
