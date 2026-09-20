/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_color_label_button.h"

#include <QApplication>
#include <QImage>
#include <QPaintEvent>
#include <QPainter>
#include <QPointer>
#include <QTest>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEnterEvent>
#endif

namespace
{
QColor renderedPixel(KisColorLabelButton &button, const QPoint &position)
{
    button.resize(button.sizeHint());
    QImage image(button.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(button.palette().color(QPalette::Window));

    QPainter painter(&image);
    button.render(&painter);
    painter.end();

    return image.pixelColor(position);
}

class PaintCountingColorLabelButton : public KisColorLabelButton
{
public:
    explicit PaintCountingColorLabelButton(const QColor &color)
        : KisColorLabelButton(color)
    {
    }

    int paintCount = 0;

    void paintEvent(QPaintEvent *event) override
    {
        ++paintCount;
        KisColorLabelButton::paintEvent(event);
    }
};
} // namespace

class KisColorLabelButtonContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateEnumsAndParentLifetime();
    void renderingDistinguishesSelectionAndAvailability();
    void enterAndLeaveRequestRepainting();
    void nextCheckStateHonorsTheViableSelectionMinimum();
    void filterGroupTracksViableAndActiveLabels();
    void dragFilterUsesQObjectParentOwnership();
};

void KisColorLabelButtonContractTest::defaultStateEnumsAndParentLifetime()
{
    QCOMPARE(static_cast<int>(KisColorLabelButton::FillIn), 0);
    QCOMPARE(static_cast<int>(KisColorLabelButton::Outline), 1);

    auto *parent = new QWidget;
    QPointer<KisColorLabelButton> button = new KisColorLabelButton(QColor(180, 40, 80), 32, parent);
    QPointer<KisColorLabelFilterGroup> group = new KisColorLabelFilterGroup(parent);

    QCOMPARE(button->parentWidget(), parent);
    QVERIFY(button->isCheckable());
    QVERIFY(button->isChecked());
    QCOMPARE(button->sizeHint(), QSize(32, 32));
    QCOMPARE(button->selectionVisType(), KisColorLabelButton::FillIn);
    QCOMPARE(group->parent(), parent);
    QCOMPARE(group->minimumRequiredChecked(), 1);

    button->setSize(48);
    button->setSelectionVisType(KisColorLabelButton::Outline);
    QCOMPARE(button->sizeHint(), QSize(48, 48));
    QCOMPARE(button->selectionVisType(), KisColorLabelButton::Outline);

    delete parent;
    QVERIFY(button.isNull());
    QVERIFY(group.isNull());
}

void KisColorLabelButtonContractTest::renderingDistinguishesSelectionAndAvailability()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(250, 250, 250));

    KisColorLabelButton button(QColor(180, 40, 80), 32);
    button.setPalette(palette);

    const QColor checkedFill = renderedPixel(button, QPoint(16, 16));

    button.setChecked(false);
    const QColor uncheckedFill = renderedPixel(button, QPoint(16, 16));
    QVERIFY(checkedFill != uncheckedFill);

    button.setSelectionVisType(KisColorLabelButton::Outline);
    const QColor uncheckedOutline = renderedPixel(button, QPoint(16, 16));
    QVERIFY(uncheckedOutline != uncheckedFill);

    button.setEnabled(false);
    const QColor disabledOutline = renderedPixel(button, QPoint(16, 16));
    QVERIFY(disabledOutline != uncheckedOutline);

    KisColorLabelButton transparentButton(Qt::transparent, 32);
    transparentButton.setPalette(palette);
    const QColor transparentTile = renderedPixel(transparentButton, QPoint(20, 5));
    QVERIFY(transparentTile != checkedFill);
}

void KisColorLabelButtonContractTest::enterAndLeaveRequestRepainting()
{
    PaintCountingColorLabelButton button(QColor(30, 120, 210));
    button.resize(button.sizeHint());
    button.show();
    QVERIFY(QTest::qWaitForWindowExposed(&button));
    QCoreApplication::processEvents();

    int previousPaintCount = button.paintCount;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QEvent enterEvent(QEvent::Enter);
#else
    QEnterEvent enterEvent(QPointF(1.0, 1.0), QPointF(1.0, 1.0), QPointF(1.0, 1.0));
#endif
    button.enterEvent(&enterEvent);
    QTRY_VERIFY(button.paintCount > previousPaintCount);

    previousPaintCount = button.paintCount;
    QEvent leaveEvent(QEvent::Leave);
    button.leaveEvent(&leaveEvent);
    QTRY_VERIFY(button.paintCount > previousPaintCount);
}

void KisColorLabelButtonContractTest::nextCheckStateHonorsTheViableSelectionMinimum()
{
    KisColorLabelButton standalone(QColor(10, 20, 30));
    standalone.nextCheckState();
    QVERIFY(!standalone.isChecked());
    standalone.nextCheckState();
    QVERIFY(standalone.isChecked());

    KisColorLabelButton first(QColor(100, 20, 30));
    KisColorLabelButton second(QColor(20, 100, 30));
    KisColorLabelFilterGroup group(nullptr);
    group.setExclusive(false);
    group.addButton(&first, 0);
    group.addButton(&second, 1);
    group.setViableLabels(QSet<int>{0, 1});
    group.setMinimumRequiredChecked(1);

    first.nextCheckState();
    QVERIFY(!first.isChecked());
    QVERIFY(second.isChecked());

    second.nextCheckState();
    QVERIFY(second.isChecked());

    first.nextCheckState();
    QVERIFY(first.isChecked());
}

void KisColorLabelButtonContractTest::filterGroupTracksViableAndActiveLabels()
{
    KisColorLabelButton first(QColor(100, 20, 30));
    KisColorLabelButton second(QColor(20, 100, 30));
    KisColorLabelButton third(QColor(20, 30, 100));
    KisColorLabelFilterGroup group(nullptr);
    group.setExclusive(false);
    group.addButton(&first, 0);
    group.addButton(&second, 1);
    group.addButton(&third, 2);

    group.setViableLabels(QSet<int>{0, 1, 2});
    QCOMPARE(group.countViableButtons(), 3);
    QCOMPARE(group.viableButtons().size(), 3);
    QVERIFY(group.viableButtons().contains(&first));
    QVERIFY(group.viableButtons().contains(&second));
    QVERIFY(group.viableButtons().contains(&third));
    QCOMPARE(group.countCheckedViableButtons(), 3);
    QCOMPARE(group.checkedViableButtons().size(), 3);
    QVERIFY(group.getActiveLabels().isEmpty());

    second.setChecked(false);
    QCOMPARE(group.countCheckedViableButtons(), 2);
    QCOMPARE(group.getActiveLabels(), QSet<int>({0, 2}));

    group.reset();
    QCOMPARE(group.countCheckedViableButtons(), 3);

    group.setMinimumRequiredChecked(0);
    QCOMPARE(group.minimumRequiredChecked(), 0);
    QCOMPARE(group.getActiveLabels(), QSet<int>({0, 1, 2}));

    group.setViableLabels(QList<int>{0, 1, 1, 2});
    QCOMPARE(group.countViableButtons(), 3);

    group.setAllVisibility(false);
    QVERIFY(first.isHidden());
    QVERIFY(second.isHidden());
    QVERIFY(third.isHidden());

    group.setAllVisibility(true);
    QVERIFY(!first.isHidden());
    QVERIFY(!second.isHidden());
    QVERIFY(!third.isHidden());

    second.setChecked(false);
    group.setViableLabels(QSet<int>{0, 2});
    QCOMPARE(group.countViableButtons(), 2);
    QVERIFY(group.viableButtons().contains(&first));
    QVERIFY(group.viableButtons().contains(&third));
    QVERIFY(second.isChecked());
    QVERIFY(!second.isEnabled());
}

void KisColorLabelButtonContractTest::dragFilterUsesQObjectParentOwnership()
{
    KisColorLabelMouseDragFilter standalone;
    QVERIFY(!standalone.parent());

    auto *parent = new QObject;
    QPointer<KisColorLabelMouseDragFilter> filter = new KisColorLabelMouseDragFilter(parent);
    QCOMPARE(filter->parent(), parent);

    delete parent;
    QVERIFY(filter.isNull());
}

QTEST_MAIN(KisColorLabelButtonContractTest)

#include "KisColorLabelButtonContractTest.moc"
