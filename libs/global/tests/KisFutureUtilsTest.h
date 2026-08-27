/*
 * SPDX-FileCopyrightText: 2026 LibrePaint Contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_FUTURE_UTILS_TEST_H
#define KIS_FUTURE_UTILS_TEST_H

#include <QObject>

class KisFutureUtilsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void readyValueFuture();
    void readyVoidFuture();
    void deferredContinuation();
};

#endif // KIS_FUTURE_UTILS_TEST_H
