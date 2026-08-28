/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "selection/kis_selection_actions_panel_source_p.h"

#include "KoColor.h"
#include "KoColorDisplayRendererInterface.h"
#include "application/kis_config.h"
#include "application/ui/orchestration/KisPart.h"
#include "application/ui/orchestration/kis_action.h"
#include "application/ui/orchestration/kis_action_manager.h"
#include "application/ui/workspace/KisMainWindow.h"
#include "application/ui/workspace/KisViewManager.h"
#include "canvas/kis_canvas_widget_base.h"
#include "dialogs/kis_dlg_preferences.h"
#include "kis_canvas2.h"
#include "kis_config_notifier.h"
#include "kis_coordinates_converter.h"
#include "kis_icon_utils.h"
#include "kis_painting_tweaks.h"
#include "kis_selection.h"
#include "selection/kis_selection_actions_panel_button.h"
#include "selection/kis_selection_actions_panel_handle.h"
#include "selection/kis_selection_manager.h"

#include <KoCanvasController.h>

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QWidget>
#include <kconfiggroup.h>
#include <klocalizedstring.h>

namespace KisSelectionActionsPanelSource
{

namespace
{

const QString dragOffsetConfigName = QStringLiteral("selectionActionBarDragOffset");

KisSelectionActionsPanelButton *panelButton(QAbstractButton *button)
{
    return static_cast<KisSelectionActionsPanelButton *>(button);
}

KisSelectionActionsPanelHandle *panelHandle(QWidget *handle)
{
    return static_cast<KisSelectionActionsPanelHandle *>(handle);
}

}

QVector<ActionButtonData> actionButtons()
{
    return {{QStringLiteral("select-all"), i18n("Select All"), Action::SelectAll},
            {QStringLiteral("select-invert"), i18n("Invert Selection"), Action::Invert},
            {QStringLiteral("select-clear"), i18n("Deselect"), Action::Deselect},
            {QStringLiteral("krita_tool_color_fill"),
             i18n("Fill Selection with Color"),
             Action::FillForegroundColor},
            {QStringLiteral("draw-eraser"), i18n("Clear Selection"), Action::Clear},
            {QStringLiteral("duplicatelayer"),
             i18n("Copy To New Layer"),
             Action::CopyToNewLayer},
            {QStringLiteral("tool_crop"),
             i18n("Crop to Selection"),
             Action::CropToSelection},
            {QStringLiteral("krita_tool_reference_images"),
             i18n("Toggle pin selection actions bar"),
             Action::TogglePin}};
}

QWidget *canvasWidget(KisViewManager *viewManager)
{
    return dynamic_cast<QWidget *>(viewManager->canvas());
}

QWidget *canvasWidget(KisCanvasWidgetBase *canvas)
{
    return canvas ? canvas->widget() : nullptr;
}

bool hasSelection(KisViewManager *viewManager)
{
    return bool(viewManager->selection());
}

QRectF selectionRect(KisViewManager *viewManager)
{
    KisSelectionSP selection = viewManager->selection();
    auto *canvas = dynamic_cast<KisCanvasWidgetBase *>(viewManager->canvas());
    if (!canvas || !selection) {
        return QRectF();
    }

    const KisCoordinatesConverter *converter = canvas->coordinatesConverter();
    if (!converter) {
        return QRectF();
    }

    return converter->imageToWidget(selection->outlineCache()).boundingRect();
}

QAbstractButton *createActionButton(const ActionButtonData &data,
                                    int size,
                                    QWidget *parent)
{
    return new KisSelectionActionsPanelButton(data.iconName, data.tooltip, size, parent);
}

void connectActionButton(QAbstractButton *button,
                         KisViewManager *viewManager,
                         Action action)
{
    KisSelectionManager *manager = viewManager->selectionManager();
    switch (action) {
    case Action::SelectAll:
        QObject::connect(button, &QAbstractButton::clicked, manager,
                         &KisSelectionManager::selectAll);
        break;
    case Action::Invert:
        QObject::connect(button, &QAbstractButton::clicked, manager,
                         &KisSelectionManager::invert);
        break;
    case Action::Deselect:
        QObject::connect(button, &QAbstractButton::clicked, manager,
                         &KisSelectionManager::deselect);
        break;
    case Action::FillForegroundColor:
        QObject::connect(button, &QAbstractButton::clicked, manager,
                         &KisSelectionManager::fillForegroundColor);
        break;
    case Action::Clear:
        QObject::connect(button, &QAbstractButton::clicked, manager,
                         &KisSelectionManager::clear);
        break;
    case Action::CopyToNewLayer:
        QObject::connect(button, &QAbstractButton::clicked, manager,
                         &KisSelectionManager::copySelectionToNewLayer);
        break;
    case Action::CropToSelection:
        QObject::connect(button, &QAbstractButton::clicked, manager,
                         &KisSelectionManager::imageResizeToSelection);
        break;
    case Action::TogglePin:
        QObject::connect(button, &QAbstractButton::clicked, manager,
                         &KisSelectionManager::toggleSAPpin);
        break;
    }
}

QWidget *createHandle(int size, Orientation orientation, QWidget *parent)
{
    return new KisSelectionActionsPanelHandle(size, orientation, parent);
}

QAction *createDisableAction(QObject *parent)
{
    auto *action = new KisAction(i18n("Disable selection actions bar"));
    action->setParent(parent);
    return action;
}

QAction *configureAction(KisViewManager *viewManager)
{
    return viewManager->actionManager()->actionByName(QStringLiteral("configure_sap"));
}

void connectEnvironment(KisViewManager *viewManager, QObject *panel)
{
    QObject::connect(KisConfigNotifier::instance(), SIGNAL(configChanged()),
                     panel, SLOT(configChanged()));
    QObject::connect(viewManager->canvasBase()->canvasController()->proxyObject,
                     SIGNAL(canvasStateChanged()), panel, SLOT(canvasStateChanged()));
    QObject::connect(KisPart::instance()->currentMainwindow(), SIGNAL(themeChanged()),
                     panel, SLOT(themeChanged()));
}

void initializeLegacyPosition()
{
    KisConfig config(true);
    config.setSelectionActionBarOrientation(Orientation::Horizontal);
    config.setSelectionActionBarPosition(Position::Bottom);
}

PanelConfig panelConfig()
{
    KisConfig config(true);
    return {config.selectionActionBarOrientation(),
            config.selectionActionBarPosition(),
            config.selectionActionBarBehavior()};
}

QPoint dragOffset()
{
    return KSharedConfig::openConfig()->group("").readEntry(dragOffsetConfigName,
                                                             QPoint());
}

void writeDragOffset(const QPoint &offset)
{
    KSharedConfig::openConfig()->group("").writeEntry(dragOffsetConfigName, offset);
}

void disablePanel()
{
    KisConfig config(false);
    config.setSelectionActionBar(false);
    KisConfigNotifier::instance()->notifyConfigChanged();
}

void configurePanel(KisViewManager *viewManager)
{
    KisAction *action = viewManager->actionManager()->actionByName(
        QStringLiteral("options_configure"));
    action->setData(QList<QVariant>({KisDlgPreferences::Page::General,
                                    KisDlgPreferences::GeneralTabs::Tools}));
    action->trigger();
}

void drawBackground(QPainter &painter,
                    const KoColorDisplayRendererInterface *displayRendererInterface,
                    const QPoint &position,
                    const QSize &outerSize,
                    const QSize &innerSize)
{
    const int cornerRadius = 4;
    QColor outlineColor = qApp->palette().window().color();
    QColor backgroundColor = outlineColor;
    if (!KisIconUtils::useDarkIcons()) {
        backgroundColor = backgroundColor.lighter(120);
    }
    KisPaintingTweaks::dragColor(&backgroundColor, outlineColor, 0.25);

    KoColor color;
    color.fromQColor(backgroundColor);
    backgroundColor = displayRendererInterface->convertColorToDisplayColorSpace(color);
    color.fromQColor(outlineColor);
    outlineColor = displayRendererInterface->convertColorToDisplayColorSpace(color);

    QColor transparentBackground = backgroundColor;
    transparentBackground.setAlpha(80);
    const int outlineWidth = 4;
    const QRectF contrastOutline(position - QPoint(outlineWidth + 1, outlineWidth + 1),
                                 outerSize + QSize(outlineWidth + 1,
                                                   outlineWidth + 1) * 2);
    const QRectF midOutline(position - QPoint(outlineWidth, outlineWidth),
                            outerSize + QSize(outlineWidth, outlineWidth) * 2);
    const QRectF centerBackground(position - QPoint(outlineWidth, outlineWidth) / 2,
                                  innerSize + QSize(outlineWidth, outlineWidth));
    QPainterPath backgroundPath;
    QPainterPath outlinePath;
    QPainterPath contrastOutlinePath;
    backgroundPath.addRoundedRect(centerBackground, cornerRadius, cornerRadius);
    outlinePath.addRoundedRect(midOutline, cornerRadius, cornerRadius);
    contrastOutlinePath.addRoundedRect(contrastOutline, cornerRadius, cornerRadius);
    painter.fillPath(contrastOutlinePath, transparentBackground);
    painter.fillPath(outlinePath, outlineColor);
    painter.fillPath(backgroundPath, backgroundColor);
}

void drawActionButton(QAbstractButton *button,
                      QPainter &painter,
                      const KoColorDisplayRendererInterface *displayRendererInterface)
{
    panelButton(button)->draw(painter, displayRendererInterface);
}

void drawHandle(QWidget *handle,
                QPainter &painter,
                const KoColorDisplayRendererInterface *displayRendererInterface)
{
    panelHandle(handle)->draw(painter, displayRendererInterface);
}

void setHandleHeld(QWidget *handle, bool held)
{
    panelHandle(handle)->set_held(held);
}

void setHandleOrientation(QWidget *handle, Orientation orientation)
{
    panelHandle(handle)->setOrientation(orientation);
}

void updateTheme(QAbstractButton *button)
{
    KisIconUtils::updateIcon(button);
}

void updateHandleTheme(QWidget *handle)
{
    panelHandle(handle)->themeChanged();
}

}
