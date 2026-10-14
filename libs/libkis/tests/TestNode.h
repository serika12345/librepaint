/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2017 Boudewijn Rempt

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef TESTNODE_H
#define TESTNODE_H

#include <QObject>

class TestNode : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testLayerPropertiesPreservePublicHistorySemantics_data();
    void testLayerPropertiesPreservePublicHistorySemantics();
    void testGpuNodeNotificationKeepsReentrantPropertyWrites();
    void testDirectOpacityWritesPreserveRecordedPropertyUndo_data();
    void testDirectOpacityWritesPreserveRecordedPropertyUndo();
    void testSetColorSpace();
    void testSetColorProfile();
    void testPixelData();
    void testRawPixelWritesPreserveFormatHistoryAndFailures();
    void testRawPixelWritesPreserveOtherLayerHistoryAndRedo();
    void testGpuPixelDataPreservesFormatHistoryAndSharedLifetime();
    void testPixelCopiesAndNodesOutliveTheImageOwner();
    void testLayerAndDocumentProjectionsRemainDistinct();
    void testProjectionPixelData();
    void testThumbnail();
    void testGpuBoundsAndThumbnailMatchNativePixels_data();
    void testGpuBoundsAndThumbnailMatchNativePixels();
    void testMergeDown();
    void testFindChildNodes();
};

#endif

