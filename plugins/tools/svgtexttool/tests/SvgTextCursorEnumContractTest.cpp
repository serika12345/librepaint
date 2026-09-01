/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "SvgTextCursor.h"

#include <QTest>

#include <array>
#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_SVG_TEXT_CURSOR_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&SvgTextCursor::method)), signature>)
} // namespace

class SvgTextCursorEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void movementValuesRemainStable();
    void typeSettingHandleValuesRemainStable();
    void cursorPositionAndNavigationSignaturesRemainStable();
    void cursorSelectionSignaturesRemainStable();
    void cursorEditingSignaturesRemainStable();
    void cursorInputAndFocusSignaturesRemainStable();
    void cursorEditingStateSignaturesRemainStable();
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

void SvgTextCursorEnumContractTest::cursorPositionAndNavigationSignaturesRemainStable()
{
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(getPos, int (SvgTextCursor::*)());
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(getAnchor, int (SvgTextCursor::*)());
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(setPos, void (SvgTextCursor::*)(int, int));
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(setPosToPoint, void (SvgTextCursor::*)(QPointF, bool));
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(moveCursor, void (SvgTextCursor::*)(SvgTextCursor::MoveMode, bool));
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(setVisualMode, void (SvgTextCursor::*)(bool));
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(updateModifiers, void (SvgTextCursor::*)(Qt::KeyboardModifiers));
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(notifyCursorPosChanged, void (SvgTextCursor::*)(int, int));

    static_assert(
        std::is_same_v<decltype(std::declval<SvgTextCursor &>().setPosToPoint(std::declval<QPointF>())), void>);
    static_assert(
        std::is_same_v<decltype(std::declval<SvgTextCursor &>().moveCursor(std::declval<SvgTextCursor::MoveMode>())),
                       void>);
    static_assert(std::is_same_v<decltype(std::declval<SvgTextCursor &>().setVisualMode()), void>);
}

void SvgTextCursorEnumContractTest::cursorSelectionSignaturesRemainStable()
{
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(hasSelection, bool (SvgTextCursor::*)());
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(deselectText, void (SvgTextCursor::*)());
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(removeSelection, void (SvgTextCursor::*)());
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(copy, void (SvgTextCursor::*)() const);
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(paste, bool (SvgTextCursor::*)());
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(selectionChanged, void (SvgTextCursor::*)());
}

void SvgTextCursorEnumContractTest::cursorEditingSignaturesRemainStable()
{
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(insertText, void (SvgTextCursor::*)(QString));
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(insertRichText, void (SvgTextCursor::*)(KoSvgTextShape *, bool));
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(removeText,
                                     void (SvgTextCursor::*)(SvgTextCursor::MoveMode, SvgTextCursor::MoveMode));
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(removeLastCodePoint, void (SvgTextCursor::*)());
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(keyPressEvent, void (SvgTextCursor::*)(QKeyEvent *));
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(setPasteRichTextByDefault, void (SvgTextCursor::*)(bool));

    static_assert(
        std::is_same_v<decltype(std::declval<SvgTextCursor &>().insertRichText(static_cast<KoSvgTextShape *>(nullptr))),
                       void>);
    static_assert(std::is_same_v<decltype(std::declval<SvgTextCursor &>().setPasteRichTextByDefault()), void>);
}

void SvgTextCursorEnumContractTest::cursorInputAndFocusSignaturesRemainStable()
{
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(inputMethodQuery, QVariant (SvgTextCursor::*)(Qt::InputMethodQuery) const);
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(inputMethodEvent, void (SvgTextCursor::*)(QInputMethodEvent *));
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(focusIn, void (SvgTextCursor::*)());
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(focusOut, void (SvgTextCursor::*)());
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(setCaretSetting, void (SvgTextCursor::*)(int, int, int, bool));

    static_assert(std::is_same_v<decltype(std::declval<SvgTextCursor &>().setCaretSetting()), void>);
}

void SvgTextCursorEnumContractTest::cursorEditingStateSignaturesRemainStable()
{
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(setTypeSettingModeActive, void (SvgTextCursor::*)(bool));
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(isAddingCommand, bool (SvgTextCursor::*)() const);
    ASSERT_SVG_TEXT_CURSOR_SIGNATURE(notifyMarkupChanged, void (SvgTextCursor::*)());
}

QTEST_GUILESS_MAIN(SvgTextCursorEnumContractTest)

#include "SvgTextCursorEnumContractTest.moc"
