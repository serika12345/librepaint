/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __KO_ANCHOR_SELECTION_WIDGET_TEST_H
#define __KO_ANCHOR_SELECTION_WIDGET_TEST_H

#include <QObject>

class KoAnchorSelectionWidgetTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testConstructionAndLifetime();
    void testSelectionAndNotifications();
    void testRectangleConversion();
    void testSlotValidation();
    void testSizeHints();
};

#endif /* __KO_ANCHOR_SELECTION_WIDGET_TEST_H */
