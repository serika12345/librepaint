
/*
 *  SPDX-FileCopyrightText: 2025 Ross Rosales <ross.erosales@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "selection/kis_selection_actions_panel.h"
#include "selection/kis_selection_actions_panel_source_p.h"

#include <QAbstractButton>
#include <QAction>
#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QWidget>

namespace Source = KisSelectionActionsPanelSource;

static constexpr int BUTTON_SIZE = 30;
static constexpr int BUFFER_SPACE = 5;

static constexpr int BIG_BUFFER_SPACE_X = 30;
static constexpr int BIG_BUFFER_SPACE_Y = 30;



struct KisSelectionActionsPanel::Private {
    Private()
    {
    }
    KisViewManager *m_viewManager = nullptr;

    bool m_pressed = false;
    bool m_visible = false;
    bool m_enabled = true;

    struct DragHandle {
        QPoint position = QPoint(0, 0);
        QPoint dragOrigin = QPoint(0, 0);
        QPoint dragOffset = QPoint(0, 0);
    };

    DragHandle m_dragHandle;

    QWidget *m_handleWidget {nullptr};
    QList<QAbstractButton *> m_buttons;
    int m_buttonCount = 1;

    int m_actionBarWidth = m_buttonCount * BUTTON_SIZE;
    int m_actionBarHeight = BUTTON_SIZE;
    int m_innerActionBarWidth = (m_buttonCount - 1) * BUTTON_SIZE;
    int m_innerActionBarHeight = BUTTON_SIZE;

    QAction *disable_action {nullptr};
    QAction *configure_action {nullptr};

    Orientation orientation = Orientation::Horizontal;
    Position position = Position::Bottom;
    Behavior behavior = Behavior::FreeFloating;

    int sapPinButtonIndex = {-1};
};



KisSelectionActionsPanel::KisSelectionActionsPanel(KisViewManager *viewManager)
    : d(new Private)
{
    d->m_viewManager = viewManager;
    Source::initializeLegacyPosition();

    // Setup buttons...
    const QVector<Source::ActionButtonData> data = Source::actionButtons();
    QWidget *canvas = Source::canvasWidget(viewManager);
    for (int i = 0; i < data.length(); i++) {
        const Source::ActionButtonData &buttonData = data[i];
        QAbstractButton *button = Source::createActionButton(buttonData, BUTTON_SIZE, canvas);
        button->setObjectName(buttonData.iconName);
        Source::connectActionButton(button, viewManager, buttonData.action);
        connect(button, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
        d->m_buttons.append(button);

        if (buttonData.action == Source::Action::TogglePin) {
            d->sapPinButtonIndex = i;
            button->setCheckable(true);
        }
    }
    d->m_buttonCount = d->m_buttons.size() + 1;

    d->m_handleWidget = Source::createHandle(BUTTON_SIZE, d->orientation, canvas);
    d->m_handleWidget->setObjectName(QStringLiteral("selection-actions-handle"));
    connect(d->m_handleWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

    d->disable_action = Source::createDisableAction(this);
    connect(d->disable_action, SIGNAL(triggered()), SLOT(disableSelectionActionsPanel()));

    d->configure_action = Source::configureAction(viewManager);
    connect(d->configure_action, SIGNAL(triggered()), SLOT(configureSelectionActionsPanel()));

    Source::connectEnvironment(viewManager, this);
    d->m_dragHandle.dragOffset = Source::dragOffset();

    configChanged(true);

}

KisSelectionActionsPanel::~KisSelectionActionsPanel()
{
}

void KisSelectionActionsPanel::draw(QPainter &painter, const KoColorDisplayRendererInterface *displayRendererInterface)
{
    if (!Source::hasSelection(d->m_viewManager) || !d->m_enabled || !d->m_visible) {
        return;
    }

    if (d->m_pressed && d->behavior == Behavior::FreeFloating) {
        drawAnchorWhileMoving(painter);
    }

    //drawDebugRectanglesForFreeFloatingBehaviour(painter);

    drawActionBarBackground(painter, displayRendererInterface);
    Q_FOREACH(QAbstractButton *button, d->m_buttons) {
        Source::drawActionButton(button, painter, displayRendererInterface);
    }

    if (d->m_handleWidget->isEnabled()) {
        Source::drawHandle(d->m_handleWidget, painter, displayRendererInterface);
    }

}

void KisSelectionActionsPanel::setOrientation(Orientation mode)
{
    d->orientation = mode;
    Source::setHandleOrientation(d->m_handleWidget, mode);
    recalculateDimensions();

    //Recalcute the position of the bar to be inside the canvas
    updatePositioning();
}

void KisSelectionActionsPanel::setHandleEnabled(bool enabled)
{
    if (enabled && !d->m_handleWidget->isEnabled()) {
        d->m_handleWidget->setEnabled(enabled);
        d->m_buttonCount++;
    }

    if (!enabled && d->m_handleWidget->isEnabled()) {
        d->m_handleWidget->setEnabled(enabled);
        d->m_buttonCount--;
    }
    recalculateDimensions();
}

QPoint KisSelectionActionsPanel::getFixedPosition() const
{
    QWidget *canvasWidget = Source::canvasWidget(d->m_viewManager);
    if (!canvasWidget) {
        return QPoint();
    }

    QRect canvasBounds = canvasWidget->rect();
    QPoint result = QPoint();

    switch (d->position) {
    case Position::Bottom:
        result.setY(canvasBounds.bottom());
        result.setX((canvasBounds.width() - d->m_actionBarWidth) / 2);
        break;
    case Position::BottomLeft:
        result.setY(canvasBounds.bottom());
        result.setX(canvasBounds.left());
        break;
    case Position::BottomRight:
        result.setY(canvasBounds.bottom());
        result.setX(canvasBounds.right());
        break;
    case Position::Left:
        result.setY((canvasBounds.height() - d->m_actionBarHeight) / 2);
        result.setX(canvasBounds.left());
        break;
    case Position::Right:
        result.setY((canvasBounds.height() - d->m_actionBarHeight) / 2);
        result.setX(canvasBounds.right());
        break;
    case Position::Top:
        result.setY(canvasBounds.top());
        result.setX((canvasBounds.width() - d->m_actionBarWidth) / 2);
        break;
    case Position::TopLeft:
        result.setY(canvasBounds.top());
        result.setX(canvasBounds.left());
        break;
    case Position::TopRight:
        result.setY(canvasBounds.top());
        result.setX(canvasBounds.right());
        break;
    }

    return clipPositionToCanvasBoundaries(result, canvasWidget);
}

void KisSelectionActionsPanel::recalculateDimensions()
{
    if (!d->m_handleWidget) return;

    if (d->orientation == Orientation::Horizontal) {
        d->m_innerActionBarHeight = d->m_actionBarHeight = BUTTON_SIZE;
        d->m_innerActionBarWidth = d->m_actionBarWidth = BUTTON_SIZE * d->m_buttonCount;
        if (d->m_handleWidget->isEnabled()) {
            d->m_innerActionBarWidth = BUTTON_SIZE * (d->m_buttonCount - 1);
        }
    } else if (d->orientation == Orientation::Vertical) {
        d->m_innerActionBarHeight = d->m_actionBarHeight = BUTTON_SIZE * d->m_buttonCount;
        d->m_innerActionBarWidth = d->m_actionBarWidth = BUTTON_SIZE;
        if (d->m_handleWidget->isEnabled()) {
            d->m_innerActionBarHeight = BUTTON_SIZE * (d->m_buttonCount - 1);
        }
    }

    movePanelWidgets();
}

void KisSelectionActionsPanel::setVisible(bool p_visible)
{
    QWidget *canvasWidget = Source::canvasWidget(d->m_viewManager);
    if (!canvasWidget) {
        return;
    }

    p_visible &= d->m_enabled;

    const bool VISIBILITY_CHANGED = d->m_visible != p_visible;
    if (!VISIBILITY_CHANGED) {
        return;
    }

    d->configure_action->setVisible(p_visible && Source::hasSelection(d->m_viewManager));

    // movePanelWidgets() uses d->m_visible to decide whether to make the widgets
    // visible or not
    d->m_visible = p_visible;

    if (Source::hasSelection(d->m_viewManager) && p_visible) { // Now visible!
        d->m_handleWidget->installEventFilter(this);
        updatePositioning();
    } else { // Now hidden!
        d->m_handleWidget->removeEventFilter(this);

        for (QAbstractButton *button : d->m_buttons) {
            button->hide();
        }
        d->m_handleWidget->hide();

        d->m_pressed = false;
    }
}

void KisSelectionActionsPanel::setEnabled(bool enabled)
{

    bool configurationChanged = enabled != d->m_enabled;

    d->configure_action->setVisible(enabled && Source::hasSelection(d->m_viewManager));

    d->m_enabled = enabled;
    if (configurationChanged) {
        // Reset visibility when configuration changes
        setVisible(enabled);
    }
}

bool KisSelectionActionsPanel::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        return handlePress(event, mouseEventPos(mouseEvent), mouseEvent->button());
    }
    case QEvent::TabletPress: {
        const QTabletEvent *tabletEvent = static_cast<QTabletEvent *>(event);
        return handlePress(event, tabletEventPos(tabletEvent));
    }
    case QEvent::TouchBegin: {
        const QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        QPoint pos;
        if (touchEventPos(touchEvent, pos)) {
            return handlePress(event, pos);
        }
        break;
    }

    case QEvent::MouseMove:
        if (d->m_pressed) {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            return handleMove(event, mouseEventPos(mouseEvent));
        }
        break;
    case QEvent::TabletMove:
        if (d->m_pressed) {
            const QTabletEvent *tabletEvent = static_cast<QTabletEvent *>(event);
            return handleMove(event, tabletEventPos(tabletEvent));
        }
        break;
    case QEvent::TouchUpdate:
        if (d->m_pressed) {
            const QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
            QPoint pos;
            if (touchEventPos(touchEvent, pos)) {
                return handleMove(event, pos);
            }
        }
        break;

    case QEvent::MouseButtonRelease:
    case QEvent::TabletRelease:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        if (d->m_pressed) {
            return handleRelease(event);
        }
        break;

    default:
        break;
    }
    return false;
}


void KisSelectionActionsPanel::canvasWidgetChanged(KisCanvasWidgetBase* canvas)
{
    QWidget *canvasWidget = Source::canvasWidget(canvas);

    Q_FOREACH(QWidget* btn, d->m_buttons)  {
        btn->setParent(canvasWidget);
        if (d->m_visible) {
            btn->show();
        }
    }

    d->m_handleWidget->setParent(canvasWidget);
    if (d->m_visible) {
        d->m_handleWidget->show();
    }
}

void KisSelectionActionsPanel::updatePositioning()
{
    if (d->m_visible) {
        d->m_dragHandle.position = currentTopLeftPosition();
        movePanelWidgets();
    }
}

QPoint KisSelectionActionsPanel::clipPositionToCanvasBoundaries(QPoint position, QWidget *canvasWidget) const
{
    QRect canvasBounds = canvasWidget->rect();

    const int ACTION_BAR_WIDTH = d->m_actionBarWidth;
    const int ACTION_BAR_HEIGHT = d->m_actionBarHeight;

    int pos_x_min = canvasBounds.left() + BUFFER_SPACE;
    int pos_x_max = canvasBounds.right() - ACTION_BAR_WIDTH - BUFFER_SPACE;

    int pos_y_min = canvasBounds.top() + BUFFER_SPACE;
    int pos_y_max = canvasBounds.bottom() - ACTION_BAR_HEIGHT - BUFFER_SPACE;

    //Ensure that max is always bigger than min
    //If the window is small enough max could be smaller than min
    if (pos_x_max < pos_x_min) {
        pos_x_max = pos_x_min;
    }

    //It is pretty implausible for it to happen vertically but better safe than sorry
    if (pos_y_max < pos_y_min) {
        pos_y_max = pos_y_min;
    }

    position.setX(qBound(pos_x_min, position.x(), pos_x_max));
    position.setY(qBound(pos_y_min, position.y(), pos_y_max));

    return position;
}

QPoint KisSelectionActionsPanel::horizontalFreeFloatingTopLeftPosition(bool calculateOnlyAnchor) const
{
    Position position = d->position;
    QRect selection = getWidgetSelectionRect().toRect();
    QPoint anchor = selection.center();
    QPoint offsetted = selection.center();
    qreal barHorizontalOffset = d->m_actionBarWidth;
    qreal barVerticalOffset = d->m_actionBarHeight;

    switch (position) {
        case Position::Top:
        case Position::Bottom:
            anchor = QPoint(selection.center().x(), 0);
            offsetted = anchor - QPoint(barHorizontalOffset/2, 0);
        break;
        case Position::TopLeft:
        case Position::BottomLeft:
            anchor = QPoint(selection.left(), 0);
            offsetted = anchor;
        break;
        case Position::TopRight:
        case Position::BottomRight:
            anchor = QPoint(selection.right(), 0);
            offsetted = anchor - QPoint(barHorizontalOffset, 0);
        break;


        case Position::Left:
            anchor = QPoint(selection.left(), 0);
            offsetted = anchor - QPoint(barHorizontalOffset + BIG_BUFFER_SPACE_X, 0);
        break;

        case Position::Right:
            anchor = QPoint(selection.right(), 0);
            offsetted = anchor + QPoint(BIG_BUFFER_SPACE_X, 0);
        break;
    }

    switch (position) {
        case Position::Top:
        case Position::TopLeft:
        case Position::TopRight:
            anchor += QPoint(0, selection.top());
            offsetted += QPoint(0, selection.top() - BIG_BUFFER_SPACE_Y - barVerticalOffset);
        break;
        case Position::Bottom:
        case Position::BottomLeft:
        case Position::BottomRight:
            anchor += QPoint(0, selection.bottom());
            offsetted += QPoint(0, selection.bottom() + BIG_BUFFER_SPACE_Y);
        break;

        case Position::Left:
        case Position::Right:
            anchor += QPoint(0, selection.center().y());
            offsetted += QPoint(0, selection.center().y() - barVerticalOffset/2);
        break;

    }

    if (calculateOnlyAnchor) {
        return anchor;
    }
    return offsetted;

}

QPoint KisSelectionActionsPanel::verticalFreeFloatingTopLeftPosition(bool calculateOnlyAnchor) const
{
    Position position = d->position;
    QRect selection = getWidgetSelectionRect().toRect();
    QPoint anchor = selection.center();
    QPoint offsetted = selection.center();
    qreal barVerticalOffset = d->m_actionBarHeight;
    qreal barHorizontalOffset = d->m_actionBarWidth;

    switch (position) {
        case Position::Left:
        case Position::Right:
            anchor = QPoint(0, selection.center().y());
            offsetted = -QPoint(0, barVerticalOffset/2);
        break;
        case Position::TopLeft:
        case Position::TopRight:
            anchor = QPoint(0, selection.top());
            offsetted = QPoint();
        break;
        case Position::BottomLeft:
        case Position::BottomRight:
            anchor = QPoint(0, selection.bottom());
            offsetted = -QPoint(0, barVerticalOffset);
        break;

        case Position::Top:
            anchor = QPoint(0, selection.top());
            offsetted = -QPoint(0, barVerticalOffset + BIG_BUFFER_SPACE_Y);
        break;

        case Position::Bottom:
            anchor = QPoint(0, selection.bottom());
            offsetted = QPoint(0, BIG_BUFFER_SPACE_Y);
        break;
    }

    switch (position) {
        case Position::Left:
        case Position::TopLeft:
        case Position::BottomLeft:
            anchor += QPoint(selection.left(), 0);
            offsetted += QPoint(- BIG_BUFFER_SPACE_X - barHorizontalOffset, 0);
        break;
        case Position::Right:
        case Position::TopRight:
        case Position::BottomRight:
            anchor += QPoint(selection.right(), 0);
            offsetted += QPoint(BIG_BUFFER_SPACE_X, 0);
        break;

        case Position::Top:
        case Position::Bottom:
            anchor += QPoint(selection.center().x(), 0);
            offsetted += QPoint(- barHorizontalOffset/2, 0);
        break;
    }

    if (calculateOnlyAnchor) {
        return anchor;
    }
    return anchor + offsetted;
}

QRectF KisSelectionActionsPanel::getWidgetSelectionRect() const
{
    return d->m_viewManager ? Source::selectionRect(d->m_viewManager) : QRectF();
}

QPoint KisSelectionActionsPanel::freeFloatingInitialTopLeftPosition(bool calculateOnlyAnchor) const
{
    if (d->orientation == Orientation::Horizontal) {
        return horizontalFreeFloatingTopLeftPosition(calculateOnlyAnchor);
    } else {
        return verticalFreeFloatingTopLeftPosition(calculateOnlyAnchor);
    }
}

QPoint KisSelectionActionsPanel::currentTopLeftPosition() const
{
    if (d->behavior == Behavior::Fixed) {
        return getFixedPosition();
    }

    QPoint widgetTopLeftPosition = freeFloatingInitialTopLeftPosition();
    widgetTopLeftPosition += d->m_dragHandle.dragOffset;

    return clipPositionToCanvasBoundaries(widgetTopLeftPosition,
                                           Source::canvasWidget(d->m_viewManager));
}

void KisSelectionActionsPanel::drawAnchorWhileMoving(QPainter &painter) const
{
    painter.save();

    QPen pen = painter.pen();
    pen.setStyle(Qt::CustomDashLine);
    pen.setDashPattern({6, 6});
    pen.setColor(Qt::darkGray);
    painter.setPen(pen);
    QPoint initial = freeFloatingInitialTopLeftPosition(true);
    QPoint offset = QPoint(d->m_actionBarWidth/2, d->m_actionBarHeight/2);
    painter.drawLine(QLine(initial, d->m_dragHandle.position + offset));

    int xSize = 8;
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);

    painter.drawLine(initial + QPoint(xSize, xSize), initial - QPoint(xSize, xSize));
    painter.drawLine(initial + QPoint(xSize, -xSize), initial + QPoint(-xSize, xSize));

    painter.restore();
}

void KisSelectionActionsPanel::drawActionBarBackground(QPainter &painter, const KoColorDisplayRendererInterface *displayRendererInterface) const
{
    Source::drawBackground(painter,
                           displayRendererInterface,
                           d->m_dragHandle.position,
                           QSize(d->m_actionBarWidth, d->m_actionBarHeight),
                           QSize(d->m_innerActionBarWidth, d->m_innerActionBarHeight));
}

bool KisSelectionActionsPanel::handlePress(QEvent *event, const QPoint &pos, Qt::MouseButton button)
{
    if (d->m_pressed) {
        event->accept();
        return true;
    }

    if (button == Qt::LeftButton) {
        d->m_pressed = true;
        d->m_dragHandle.dragOrigin = pos - d->m_dragHandle.dragOffset;
        Source::setHandleHeld(d->m_handleWidget, true);

        event->accept();
        return true;
    }

    return false;
}

bool KisSelectionActionsPanel::handleMove(QEvent *event, const QPoint &pos)
{
    QWidget *canvasWidget = Source::canvasWidget(d->m_viewManager);
    QPoint newPos = pos - d->m_dragHandle.dragOrigin;
    d->m_dragHandle.dragOffset = newPos;

    updatePositioning();

    canvasWidget->update();
    event->accept();
    return true;
}

bool KisSelectionActionsPanel::handleRelease(QEvent *event)
{
    Source::setHandleHeld(d->m_handleWidget, false);
    d->m_pressed = false;
    event->accept();

    Source::writeDragOffset(d->m_dragHandle.dragOffset);

    return true;
}

void KisSelectionActionsPanel::movePanelWidgets()
{
    //This function gets called on panel creation, when dragHandle is not initialized, so we need to handle that
    if (!d->m_handleWidget)
        return;

    // don't show the widgets if the panel is hidden
    if (!d->m_visible)
        return;

    if (d->orientation == Orientation::Vertical) {
        d->m_handleWidget->move(d->m_dragHandle.position.x(),
                                d->m_dragHandle.position.y() + d->m_buttons.size() * BUTTON_SIZE);
    } else if (d->orientation == Orientation::Horizontal) {
        d->m_handleWidget->move(d->m_dragHandle.position.x() + d->m_buttons.size() * BUTTON_SIZE,
                                d->m_dragHandle.position.y());
    }
    d->m_handleWidget->show();

    int i = 0;
    Q_FOREACH (QAbstractButton *button, d->m_buttons) {
        int buttonPosition = i * BUTTON_SIZE;

        if (d->orientation == Orientation::Vertical) {
            button->move(d->m_dragHandle.position.x(), d->m_dragHandle.position.y() + buttonPosition);
        } else if (d->orientation == Orientation::Horizontal) {
            button->move(d->m_dragHandle.position.x() + buttonPosition, d->m_dragHandle.position.y());
        }
        button->show();

        i++;
    }
}

QPoint KisSelectionActionsPanel::mouseEventPos(const QMouseEvent *mouseEvent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return transformHandleCoords(mouseEvent->position().toPoint());
#else
    return transformHandleCoords(mouseEvent->pos());
#endif
}

QPoint KisSelectionActionsPanel::tabletEventPos(const QTabletEvent *tabletEvent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return transformHandleCoords(tabletEvent->position().toPoint());
#else
    return transformHandleCoords(tabletEvent->pos());
#endif
}

bool KisSelectionActionsPanel::touchEventPos(const QTouchEvent *touchEvent, QPoint &outPos)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (touchEvent->pointCount() < 1) {
        return false;
    } else {
        outPos = transformHandleCoords(touchEvent->points().first().position().toPoint());
        return true;
    }
#else
    const QList<QTouchEvent::TouchPoint> &touchPoints = touchEvent->touchPoints();
    if (touchPoints.isEmpty()) {
        return false;
    } else {
        outPos = transformHandleCoords(touchPoints.first().pos().toPoint());
        return true;
    }
#endif
}

QPoint KisSelectionActionsPanel::transformHandleCoords(QPoint pos) {
    return d->m_dragHandle.position + pos;
}

void KisSelectionActionsPanel::showContextMenu(const QPoint &pos)
{
    QMenu menu = QMenu();
    menu.addAction(d->disable_action);
    menu.addAction(d->configure_action);
    menu.exec(pos);
}

void KisSelectionActionsPanel::disableSelectionActionsPanel()
{
    Source::disablePanel();
}

void KisSelectionActionsPanel::configureSelectionActionsPanel()
{
    Source::configurePanel(d->m_viewManager);
}

void KisSelectionActionsPanel::configChanged(bool skipResettingOffset)
{
    const Source::PanelConfig config = Source::panelConfig();
    setHandleEnabled(config.behavior != Behavior::Fixed);

    bool resetOffset = false;
    if (d->orientation != config.orientation) {
        resetOffset = true;
        setOrientation(config.orientation);
    }
    d->behavior = config.behavior;
    if (d->position != config.position) {
        resetOffset = true;
        d->position = config.position;
    }

    if (resetOffset && !skipResettingOffset) {
        d->m_dragHandle.dragOffset = QPoint();
        Source::writeDragOffset(QPoint());
    }

    if (d->sapPinButtonIndex >= 0 && d->m_buttons[d->sapPinButtonIndex] && d->m_buttons[d->sapPinButtonIndex]->isCheckable()) {
        d->m_buttons[d->sapPinButtonIndex]->setChecked(config.behavior == Behavior::Fixed);
    }

    updatePositioning();
}

void KisSelectionActionsPanel::canvasStateChanged()
{
    updatePositioning();
}

void KisSelectionActionsPanel::themeChanged()
{
    Q_FOREACH(QAbstractButton *button, d->m_buttons) {
        Source::updateTheme(button);
    }
    Source::updateHandleTheme(d->m_handleWidget);
}



void KisSelectionActionsPanel::drawDebugRectangle(QPainter &painter, Position position)
{
    d->position = position;
    QBrush brush(Qt::cyan);
    QPoint p = freeFloatingInitialTopLeftPosition();
    brush.setColor(QColor(100 + 20*(int)position, 100, 100, 150));
    painter.setBrush(brush);
    painter.drawRect(QRect(p, QSize(d->m_actionBarWidth, d->m_actionBarHeight)));
    painter.drawEllipse(freeFloatingInitialTopLeftPosition(true), 10, 10);
}


void KisSelectionActionsPanel::drawDebugRectanglesForFreeFloatingBehaviour(QPainter &painter)
{
    painter.save();
    Position remember = d->position;

    for (int position = 0; position < 8; position++) {
        drawDebugRectangle(painter, (Position)position);
    }

    painter.restore();
    d->position = remember;

}
