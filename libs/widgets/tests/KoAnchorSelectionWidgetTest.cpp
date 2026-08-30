/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoAnchorSelectionWidgetTest.h"

#include <KoAnchorSelectionWidget.h>

#include <QFontMetrics>
#include <QIcon>
#include <QPointer>
#include <QTest>
#include <QToolButton>

#include <type_traits>

namespace
{
struct AnchorConversionState {
    int callCount{0};
    KoFlake::AnchorPosition anchor{KoFlake::NoAnchor};
    QRectF rect;
    bool *validPointer{nullptr};
    QPointF result{91.25, -44.5};
    bool validResult{true};
};

AnchorConversionState anchorConversionState;
int safeAssertCount = 0;
} // namespace

namespace KisIconUtils
{
QIcon loadIcon(const QString &)
{
    return {};
}
} // namespace KisIconUtils

namespace KoFlake
{
QPointF anchorToPoint(AnchorPosition anchor, const QRectF rect, bool *valid)
{
    anchorConversionState.callCount++;
    anchorConversionState.anchor = anchor;
    anchorConversionState.rect = rect;
    anchorConversionState.validPointer = valid;

    if (valid) {
        *valid = anchorConversionState.validResult;
    }

    return anchorConversionState.result;
}
} // namespace KoFlake

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    safeAssertCount++;
}

void KoAnchorSelectionWidgetTest::testConstructionAndLifetime()
{
    static_assert(std::has_virtual_destructor_v<KoAnchorSelectionWidget>);

    QPointer<KoAnchorSelectionWidget> guardedWidget;
    {
        QWidget parent;
        auto *widget = new KoAnchorSelectionWidget(&parent);
        guardedWidget = widget;

        QCOMPARE(widget->parentWidget(), &parent);
        QCOMPARE(widget->findChildren<QToolButton *>(QString(), Qt::FindDirectChildrenOnly).size(),
                 static_cast<int>(KoFlake::NumAnchorPositions));
    }

    QVERIFY(guardedWidget.isNull());
}

void KoAnchorSelectionWidgetTest::testSelectionAndNotifications()
{
    KoAnchorSelectionWidget widget;
    QVector<KoFlake::AnchorPosition> notifications;
    connect(&widget, &KoAnchorSelectionWidget::valueChanged, &widget, [&notifications](KoFlake::AnchorPosition value) {
        notifications.append(value);
    });

    QCOMPARE(static_cast<int>(widget.value()), -1);

    for (int i = KoFlake::TopLeft; i <= KoFlake::NoAnchor; ++i) {
        const auto value = static_cast<KoFlake::AnchorPosition>(i);

        widget.setValue(value);
        QCOMPARE(widget.value(), value);
        QCOMPARE(notifications.size(), 1);
        QCOMPARE(notifications.constLast(), value);

        widget.setValue(value);
        QCOMPARE(widget.value(), value);
        QCOMPARE(notifications.size(), 1);

        notifications.clear();
    }
}

void KoAnchorSelectionWidgetTest::testRectangleConversion()
{
    KoAnchorSelectionWidget widget;
    widget.setValue(KoFlake::BottomRight);

    anchorConversionState = {};
    anchorConversionState.result = QPointF(91.25, -44.5);
    anchorConversionState.validResult = true;

    const QRectF rect(-3.5, 7.25, 12.0, 18.5);
    bool valid = false;
    const QPointF result = widget.value(rect, &valid);

    QCOMPARE(anchorConversionState.callCount, 1);
    QCOMPARE(anchorConversionState.anchor, KoFlake::BottomRight);
    QCOMPARE(anchorConversionState.rect, rect);
    QCOMPARE(anchorConversionState.validPointer, &valid);
    QCOMPARE(result, anchorConversionState.result);
    QVERIFY(valid);

    anchorConversionState.callCount = 0;
    anchorConversionState.validPointer = &valid;
    QCOMPARE(widget.value(rect, nullptr), anchorConversionState.result);
    QCOMPARE(anchorConversionState.callCount, 1);
    QCOMPARE(anchorConversionState.validPointer, nullptr);
}

void KoAnchorSelectionWidgetTest::testSlotValidation()
{
    KoAnchorSelectionWidget widget;
    widget.setValue(KoFlake::Left);

    QVector<KoFlake::AnchorPosition> notifications;
    connect(&widget, &KoAnchorSelectionWidget::valueChanged, &widget, [&notifications](KoFlake::AnchorPosition value) {
        notifications.append(value);
    });

    safeAssertCount = 0;
    widget.slotGroupClicked(KoFlake::TopRight);
    QCOMPARE(notifications, QVector<KoFlake::AnchorPosition>{KoFlake::TopRight});
    QCOMPARE(widget.value(), KoFlake::Left);
    QCOMPARE(safeAssertCount, 0);

    notifications.clear();
    widget.slotGroupClicked(-1);
    widget.slotGroupClicked(KoFlake::NumAnchorPositions);
    QVERIFY(notifications.isEmpty());
    QCOMPARE(widget.value(), KoFlake::Left);
    QCOMPARE(safeAssertCount, 2);
}

void KoAnchorSelectionWidgetTest::testSizeHints()
{
    KoAnchorSelectionWidget widget;
    const int minimumSide = 3 * (QFontMetrics(widget.font()).height() + 5);

    QCOMPARE(widget.minimumSizeHint(), QSize(minimumSide, minimumSide));

    widget.resize(20, minimumSide + 37);
    QCOMPARE(widget.sizeHint(), QSize(minimumSide + 37, minimumSide + 37));

    widget.resize(20, 1);
    QCOMPARE(widget.sizeHint(), QSize(minimumSide, minimumSide));
}

QTEST_MAIN(KoAnchorSelectionWidgetTest)
