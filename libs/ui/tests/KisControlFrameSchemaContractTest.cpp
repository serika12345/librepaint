/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "resources/kis_control_frame.h"

#include <QTest>

#include <type_traits>

class KisControlFrameSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void controlFrameSchemaRemainsStable();
};

void KisControlFrameSchemaContractTest::controlFrameSchemaRemainsStable()
{
    using Frame = KisControlFrame;

    static_assert(std::is_base_of_v<QObject, Frame>);
    static_assert(std::is_constructible_v<Frame, KisViewManager *, QWidget *, const char *>);
    static_assert(std::has_virtual_destructor_v<Frame>);
    static_assert(std::is_same_v<decltype(&Frame::setup), void (Frame::*)(QWidget *)>);
    static_assert(std::is_same_v<decltype(&Frame::paintopBox), KisPaintopBox *(Frame::*)()>);
}

QTEST_GUILESS_MAIN(KisControlFrameSchemaContractTest)

#include "KisControlFrameSchemaContractTest.moc"
