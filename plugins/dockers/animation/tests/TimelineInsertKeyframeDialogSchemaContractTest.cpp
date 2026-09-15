/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "timeline_insert_keyframe_dialog.h"

#include <QTest>

#include <type_traits>

class TimelineInsertKeyframeDialogSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void insertKeyframeDialogSchemaRemainsStable();
};

void TimelineInsertKeyframeDialogSchemaContractTest::insertKeyframeDialogSchemaRemainsStable()
{
    using Dialog = TimelineInsertKeyframeDialog;

    static_assert(std::is_base_of_v<QDialog, Dialog>);
    static_assert(std::is_constructible_v<Dialog, QWidget *>);
    static_assert(
        std::is_same_v<decltype(&Dialog::promptUserSettings), bool (Dialog::*)(int &, int &, TimelineDirection &)>);
    static_assert(std::is_same_v<decltype(&Dialog::defaultTimingOfAddedFrames), int (Dialog::*)() const>);
    static_assert(std::is_same_v<decltype(&Dialog::setDefaultTimingOfAddedFrames), void (Dialog::*)(int)>);
    static_assert(std::is_same_v<decltype(&Dialog::defaultNumberOfHoldFramesToRemove), int (Dialog::*)() const>);
    static_assert(std::is_same_v<decltype(&Dialog::setDefaultNumberOfHoldFramesToRemove), void (Dialog::*)(int)>);
}

QTEST_GUILESS_MAIN(TimelineInsertKeyframeDialogSchemaContractTest)

#include "TimelineInsertKeyframeDialogSchemaContractTest.moc"
