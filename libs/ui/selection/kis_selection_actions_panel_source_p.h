/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_SELECTION_ACTIONS_PANEL_SOURCE_P_H
#define KIS_SELECTION_ACTIONS_PANEL_SOURCE_P_H

#include "selection/kis_selection_actions_panel.h"

#include <QPoint>
#include <QRectF>
#include <QString>
#include <QVector>

class QAction;
class QAbstractButton;
class QPainter;
class QSize;
class QWidget;

namespace KisSelectionActionsPanelSource
{

enum class Action
{
    SelectAll,
    Invert,
    Deselect,
    FillForegroundColor,
    Clear,
    CopyToNewLayer,
    CropToSelection,
    TogglePin
};

struct ActionButtonData
{
    QString iconName;
    QString tooltip;
    Action action;
};

struct PanelConfig
{
    Orientation orientation {Orientation::Horizontal};
    Position position {Position::Bottom};
    Behavior behavior {Behavior::FreeFloating};
};

QVector<ActionButtonData> actionButtons();
QWidget *canvasWidget(KisViewManager *viewManager);
QWidget *canvasWidget(KisCanvasWidgetBase *canvas);
bool hasSelection(KisViewManager *viewManager);
QRectF selectionRect(KisViewManager *viewManager);

QAbstractButton *createActionButton(const ActionButtonData &data,
                                    int size,
                                    QWidget *parent);
void connectActionButton(QAbstractButton *button,
                         KisViewManager *viewManager,
                         Action action);
QWidget *createHandle(int size, Orientation orientation, QWidget *parent);
QAction *createDisableAction(QObject *parent);
QAction *configureAction(KisViewManager *viewManager);
void connectEnvironment(KisViewManager *viewManager, QObject *panel);

void initializeLegacyPosition();
PanelConfig panelConfig();
QPoint dragOffset();
void writeDragOffset(const QPoint &offset);
void disablePanel();
void configurePanel(KisViewManager *viewManager);

void drawBackground(QPainter &painter,
                    const KoColorDisplayRendererInterface *displayRendererInterface,
                    const QPoint &position,
                    const QSize &outerSize,
                    const QSize &innerSize);
void drawActionButton(QAbstractButton *button,
                      QPainter &painter,
                      const KoColorDisplayRendererInterface *displayRendererInterface);
void drawHandle(QWidget *handle,
                QPainter &painter,
                const KoColorDisplayRendererInterface *displayRendererInterface);
void setHandleHeld(QWidget *handle, bool held);
void setHandleOrientation(QWidget *handle, Orientation orientation);
void updateTheme(QAbstractButton *button);
void updateHandleTheme(QWidget *handle);

}

#endif
