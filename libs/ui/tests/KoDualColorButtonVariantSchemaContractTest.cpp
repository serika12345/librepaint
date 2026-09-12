/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-only
 */

#include <widgets/KoBackgroundColour.h>
#include <widgets/KoForegroundColour.h>
#include <widgets/KoResetBgFgColours.h>
#include <widgets/KoSwapBgFgColours.h>

#include <QTest>

#include <type_traits>

#define ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(type, method, signature)                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

class KoDualColorButtonVariantSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resetButtonSchemaRemainsStable();
    void swapButtonSchemaRemainsStable();
    void backgroundButtonSchemaRemainsStable();
    void foregroundButtonSchemaRemainsStable();
};

void KoDualColorButtonVariantSchemaContractTest::resetButtonSchemaRemainsStable()
{
    using Button = KoResetBgFgColours;

    static_assert(std::is_class_v<Button>);
    static_assert(std::is_base_of_v<KoDualColorButton, Button>);
    static_assert(std::is_constructible_v<Button,
                                          KisCanvasResourceProvider *,
                                          const KoColorDisplayRendererInterface *,
                                          QWidget *,
                                          QWidget *>);
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, event, bool (Button::*)(QEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, mouseMoveEvent, void (Button::*)(QMouseEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, mousePressEvent, void (Button::*)(QMouseEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, mouseReleaseEvent, void (Button::*)(QMouseEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, paint_icons, void (Button::*)(QPainter &));
}

void KoDualColorButtonVariantSchemaContractTest::swapButtonSchemaRemainsStable()
{
    using Button = KoSwapBgFgColours;

    static_assert(std::is_class_v<Button>);
    static_assert(std::is_base_of_v<KoDualColorButton, Button>);
    static_assert(std::is_constructible_v<Button,
                                          KisCanvasResourceProvider *,
                                          const KoColorDisplayRendererInterface *,
                                          QWidget *,
                                          QWidget *>);
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, event, bool (Button::*)(QEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, mouseMoveEvent, void (Button::*)(QMouseEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, mousePressEvent, void (Button::*)(QMouseEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, mouseReleaseEvent, void (Button::*)(QMouseEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, paint_icons, void (Button::*)(QPainter &));
}

void KoDualColorButtonVariantSchemaContractTest::backgroundButtonSchemaRemainsStable()
{
    using Button = KoBackgroundColour;

    static_assert(std::is_class_v<Button>);
    static_assert(std::is_base_of_v<KoDualColorButton, Button>);
    static_assert(std::is_constructible_v<Button,
                                          KisCanvasResourceProvider *,
                                          const KoColorDisplayRendererInterface *,
                                          QWidget *,
                                          QWidget *>);
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, event, bool (Button::*)(QEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, mousePressEvent, void (Button::*)(QMouseEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, mouseReleaseEvent, void (Button::*)(QMouseEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, paint_icons, void (Button::*)(QPainter &));
}

void KoDualColorButtonVariantSchemaContractTest::foregroundButtonSchemaRemainsStable()
{
    using Button = KoForegroundColour;

    static_assert(std::is_class_v<Button>);
    static_assert(std::is_base_of_v<KoDualColorButton, Button>);
    static_assert(std::is_constructible_v<Button,
                                          KisCanvasResourceProvider *,
                                          const KoColorDisplayRendererInterface *,
                                          QWidget *,
                                          QWidget *>);
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, event, bool (Button::*)(QEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, mousePressEvent, void (Button::*)(QMouseEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, mouseReleaseEvent, void (Button::*)(QMouseEvent *));
    ASSERT_DUAL_COLOR_BUTTON_SIGNATURE(Button, paint_icons, void (Button::*)(QPainter &));
}

#undef ASSERT_DUAL_COLOR_BUTTON_SIGNATURE

QTEST_GUILESS_MAIN(KoDualColorButtonVariantSchemaContractTest)

#include "KoDualColorButtonVariantSchemaContractTest.moc"
