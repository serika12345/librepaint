/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISLAZYSTORAGETEST_H
#define KISLAZYSTORAGETEST_H

#include <QObject>

class KisLazyStorageTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testLazyConstructionAndLifetime();
    void testImmediateConstruction();
    void testMove();
    void testCopyDisabled();
};

#endif // KISLAZYSTORAGETEST_H
