/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "SvgTextCursor.h"

#include <QTest>

#include <array>

class SvgTextCursorEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void movementValuesRemainStable();
    void typeSettingHandleValuesRemainStable();
};

void SvgTextCursorEnumContractTest::movementValuesRemainStable()
{
    const std::array<SvgTextCursor::MoveMode, 17> modes {{
        SvgTextCursor::MoveNone,
        SvgTextCursor::MoveLeft,
        SvgTextCursor::MoveRight,
        SvgTextCursor::MoveUp,
        SvgTextCursor::MoveDown,
        SvgTextCursor::MoveNextChar,
        SvgTextCursor::MovePreviousChar,
        SvgTextCursor::MoveNextLine,
        SvgTextCursor::MovePreviousLine,
        SvgTextCursor::MoveWordLeft,
        SvgTextCursor::MoveWordRight,
        SvgTextCursor::MoveWordEnd,
        SvgTextCursor::MoveWordStart,
        SvgTextCursor::MoveLineStart,
        SvgTextCursor::MoveLineEnd,
        SvgTextCursor::ParagraphStart,
        SvgTextCursor::ParagraphEnd,
    }};

    for (std::size_t index = 0; index < modes.size(); ++index) {
        QCOMPARE(int(modes[index]), int(index));
    }
}

void SvgTextCursorEnumContractTest::typeSettingHandleValuesRemainStable()
{
    const std::array<SvgTextCursor::TypeSettingModeHandle, 14> handles {{
        SvgTextCursor::NoHandle,
        SvgTextCursor::StartPos,
        SvgTextCursor::EndPos,
        SvgTextCursor::BaselineShift,
        SvgTextCursor::Ascender,
        SvgTextCursor::Descender,
        SvgTextCursor::LineHeightTop,
        SvgTextCursor::LineHeightBottom,
        SvgTextCursor::BaselineAlphabetic,
        SvgTextCursor::BaselineIdeographic,
        SvgTextCursor::BaselineMiddle,
        SvgTextCursor::BaselineHanging,
        SvgTextCursor::BaselineMathematical,
        SvgTextCursor::BaselineCentral,
    }};

    for (std::size_t index = 0; index < handles.size(); ++index) {
        QCOMPARE(int(handles[index]), int(index));
    }
}

QTEST_GUILESS_MAIN(SvgTextCursorEnumContractTest)

#include "SvgTextCursorEnumContractTest.moc"
