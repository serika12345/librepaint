/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SQUEEZEDCOMBOBOX_TEST_H
#define SQUEEZEDCOMBOBOX_TEST_H

#include <QObject>

class TestSqueezedComboBox : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructorPreservesParentNameAndOwnership();
    void textItemsPreserveOriginalValuesSelectionAndReset();
    void iconAndTextInsertionPreserveMetadataAtTheEnd();
    void squeezingPreservesTheRightmostVisibleSuffix();
    void middleInsertionAndRemovalExposeCurrentIndexMismatch();
};

#endif
