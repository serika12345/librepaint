/*
 * SPDX-FileCopyrightText: 2026 The Krita iPadOS Port Contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "iostouchui.h"

#include <KisDockerHud.h>
#include <KisMainWindow.h>
#include <KisPopupWidgetInterface.h>
#include <KisResourceModel.h>
#include <KisResourceTypes.h>
#include <KisViewManager.h>
#include <KoCanvasResourceProvider.h>
#include <KoCanvasResourcesIds.h>
#include <KoColor.h>
#include <KoInputDevice.h>
#include <KoResource.h>
#include <KoToolManager.h>
#include <brushengine/kis_paintop_preset.h>
#include <kactioncollection.h>
#include <kis_action.h>
#include <kis_canvas2.h>
#include <kis_canvas_resource_provider.h>
#include <kis_config.h>
#include <kis_icon_utils.h>
#include <kis_image_config.h>
#include <input/kis_input_manager.h>
#include <kis_paintop_box.h>
#include <kis_paintop_presets_chooser_popup.h>

#include <QAction>
#include <QEvent>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMargins>
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPointer>
#include <QSignalBlocker>
#include <QSlider>
#include <QStringList>
#include <QTimer>
#include <QToolButton>
#include <QVariant>
#include <QVBoxLayout>
#include <QWindow>

#include <cmath>
#include <functional>
#include <utility>

#include <klocalizedstring.h>
#include <kpluginfactory.h>

namespace
{
constexpr int TopBarHeight = 60;
constexpr int SideBarWidth = 68;
constexpr int OverlayMargin = 8;
constexpr int ToolButtonExtent = 48;
constexpr int SliderResolution = 1000;
constexpr qreal MinimumBrushSize = 0.01;

const QString CanvasOnlyStartupModeKey = QStringLiteral("iostouchui/canvasOnlyStartupMode");
const QString LastControlsVisibleKey = QStringLiteral("iostouchui/lastControlsVisible");
const QString StartupRememberLast = QStringLiteral("remember-last");
const QString StartupShowControls = QStringLiteral("show-controls");
const QString StartupHideControls = QStringLiteral("hide-controls");

const QString PanelStyle = QStringLiteral(
    "QFrame#KisIOSTouchTopBar, QFrame#KisIOSTouchSideBar, "
    "QFrame#KisIOSTouchBrushPanel, QFrame#KisIOSTouchLayerPanel {"
    "  background-color: rgba(42, 42, 42, 242);"
    "  border: 1px solid rgba(255, 255, 255, 35);"
    "  border-radius: 12px;"
    "}"
    "QToolButton {"
    "  color: #f2f2f2;"
    "  background: transparent;"
    "  border: 0;"
    "  border-radius: 9px;"
    "  padding: 5px;"
    "}"
    "QToolButton:pressed, QToolButton:checked {"
    "  background-color: #0a84ff;"
    "}"
    "QToolButton#KisIOSTouchRestoreButton {"
    "  background-color: rgba(42, 42, 42, 242);"
    "  border: 1px solid rgba(255, 255, 255, 35);"
    "  border-radius: 12px;"
    "}"
    "QToolButton#KisIOSTouchRestoreButton:pressed {"
    "  background-color: #0a84ff;"
    "}"
    "QToolButton:disabled { color: rgba(242, 242, 242, 90); }"
    "QLabel { color: #f2f2f2; }"
    "QSlider::groove:vertical {"
    "  background: rgba(255, 255, 255, 55);"
    "  width: 8px;"
    "  border-radius: 4px;"
    "}"
    "QSlider::handle:vertical {"
    "  background: #d9d9d9;"
    "  height: 28px;"
    "  margin: 0 -8px;"
    "  border-radius: 12px;"
    "}"
);
}

class KisIOSTouchPopupAdapter final : public QObject, public KisPopupWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(KisPopupWidgetInterface)

public:
    using VisibilityCallback = std::function<bool()>;
    using DismissCallback = std::function<void()>;

    KisIOSTouchPopupAdapter(VisibilityCallback visibilityCallback,
                            DismissCallback dismissCallback,
                            QObject *parent)
        : QObject(parent)
        , m_visibilityCallback(std::move(visibilityCallback))
        , m_dismissCallback(std::move(dismissCallback))
    {
    }

    void registerWith(KisInputManager *inputManager)
    {
        if (m_registered && m_inputManager == inputManager) {
            return;
        }

        if (!inputManager) {
            return;
        }

        if (m_registered) {
            finish();
        }

        m_registered = true;
        m_inputManager = inputManager;
        inputManager->registerPopupWidget(this);
    }

    void popup(const QPoint &) override
    {
        // The touch UI positions and shows its panels before registering them.
    }

    bool onScreen() override
    {
        return m_visibilityCallback && m_visibilityCallback();
    }

    void dismiss() override
    {
        m_registered = false;
        m_inputManager = nullptr;
        if (m_dismissCallback) {
            m_dismissCallback();
        }
    }

    void finish()
    {
        if (!m_registered) {
            return;
        }

        m_registered = false;
        m_inputManager = nullptr;
        Q_EMIT finished();
    }

Q_SIGNALS:
    void finished() override;

private:
    VisibilityCallback m_visibilityCallback;
    DismissCallback m_dismissCallback;
    QPointer<KisInputManager> m_inputManager;
    bool m_registered {false};
};

K_PLUGIN_FACTORY_WITH_JSON(KritaIOSTouchUIFactory, "kritaiostouchui.json", registerPlugin<KisIOSTouchUI>();)

class KisIOSTouchUI::Private final : public QObject
{
public:
    Private(KisIOSTouchUI *qq, KisViewManager *viewManager, QAction *touchUiAction)
        : QObject(qq)
        , viewManager(viewManager)
        , touchUiAction(touchUiAction)
    {
        if (!viewManager || !viewManager->mainWindowAsQWidget()) {
            return;
        }

        mainWindow = viewManager->mainWindowAsQWidget();
        mainWindow->installEventFilter(this);

#ifdef Q_OS_IOS
        if (qGuiApp) {
            connect(qGuiApp,
                    &QGuiApplication::applicationStateChanged,
                    this,
                    [this](Qt::ApplicationState state) {
                        invalidatePendingLayout();
                        if (state == Qt::ApplicationActive) {
                            scheduleLayout();
                        } else {
                            hideAllOverlays();
                        }
                    });
        }
#endif

        popupAdapter = new KisIOSTouchPopupAdapter(
            [this] {
                return (brushPanel && brushPanel->isVisible())
                    || (layerPanel && layerPanel->isVisible());
            },
            [this] {
                hidePanels(false);
            },
            this);

        if (touchUiAction) {
            touchUiAction->setCheckable(true);
            {
                KisConfig cfg(true);
                QSignalBlocker blocker(touchUiAction);
                touchUiAction->setChecked(
                    cfg.readEntry<bool>(LastControlsVisibleKey, true));
            }
            connect(touchUiAction, &QAction::toggled, this, [this](bool checked) {
                KisConfig cfg(false);
                cfg.writeEntry(LastControlsVisibleKey, checked);
                if (!checked) {
                    hideAllOverlays();
                }
                scheduleLayout();
            });
        }

        canvasOnlyAction = findAction(QStringLiteral("view_show_canvas_only"));
        if (canvasOnlyAction) {
            connect(canvasOnlyAction, &QAction::toggled, this, [this](bool checked) {
                if (!checked) {
                    hideAllOverlays();
                }
                scheduleLayout();
            });
        }

        connect(viewManager, &KisViewManager::viewChanged, this, [this] {
            hideAllOverlays();
            scheduleLayout();
        });

        connect(viewManager->mainWindow(), &KisMainWindow::restoringDone, this, [this] {
            initialize();
        });
        connect(viewManager->mainWindow(), &KisMainWindow::guiLoadingFinished, this, [this] {
            initialize();
        });
        connect(viewManager->mainWindow(), &KisMainWindow::themeChanged, this, [this] {
            updateColorButton(this->viewManager->canvasResourceProvider()->fgColor());
        });

        QTimer::singleShot(0, this, [this] {
            initialize();
        });
    }

    ~Private() override
    {
        hideAllOverlays();
        if (canvas) {
            canvas->removeEventFilter(this);
        }
        if (window) {
            window->removeEventFilter(this);
        }
        if (mainWindow) {
            mainWindow->removeEventFilter(this);
        }

        delete restoreControlsButton.data();
        delete brushPanel.data();
        delete layerPanel.data();
        delete sideBar.data();
        delete topBar.data();
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if ((watched == mainWindow || watched == canvas || watched == window)
            && (event->type() == QEvent::Resize
                || event->type() == QEvent::Move
                || event->type() == QEvent::Show
                || event->type() == QEvent::LayoutRequest
                || event->type() == QEvent::ParentChange
                || event->type() == QEvent::WindowStateChange
                || event->type() == QEvent::SafeAreaMarginsChange)) {
            scheduleLayout();
        }

        return QObject::eventFilter(watched, event);
    }

private:
    void initialize()
    {
        if (initialized || !mainWindow) {
            scheduleLayout();
            return;
        }

        initialized = true;
        createTopBar();
        createSideBar();
        createRestoreButton();
        connectResourceState();
        scheduleLayout();
    }

    QAction *findAction(const QString &id) const
    {
        return viewManager && viewManager->actionCollection()
            ? viewManager->actionCollection()->action(id)
            : nullptr;
    }

    void triggerAction(const QString &id)
    {
        QAction *action = findAction(id);
        if (action && action->isEnabled()) {
            action->trigger();
        }
    }

    QToolButton *createActionButton(QWidget *parent,
                                    const QString &actionId,
                                    const QString &fallbackIcon,
                                    const QString &fallbackToolTip,
                                    bool brushRelated = false)
    {
        auto *button = new QToolButton(parent);
        button->setAutoRaise(true);
        button->setFixedSize(ToolButtonExtent, ToolButtonExtent);
        button->setIconSize(QSize(30, 30));
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);

        QAction *action = findAction(actionId);
        if (action) {
            button->setDefaultAction(action);
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
            if (action->menu()) {
                button->setMenu(action->menu());
                button->setPopupMode(QToolButton::InstantPopup);
            }
            connect(button, &QToolButton::pressed, this, [this] {
                hidePanels();
            });
        } else {
            button->setEnabled(false);
            button->setToolTip(fallbackToolTip);
        }

        if (button->icon().isNull() && !fallbackIcon.isEmpty()) {
            button->setIcon(KisIconUtils::loadIcon(fallbackIcon));
        }
        if (button->toolTip().isEmpty()) {
            button->setToolTip(fallbackToolTip);
        }
        if (brushRelated) {
            connect(button, &QToolButton::pressed, this, [this] {
                ensureStylusDevice();
            });
        }

        return button;
    }

    QToolButton *createCustomButton(QWidget *parent,
                                    const QString &iconName,
                                    const QString &toolTip)
    {
        auto *button = new QToolButton(parent);
        button->setAutoRaise(true);
        button->setFixedSize(ToolButtonExtent, ToolButtonExtent);
        button->setIconSize(QSize(30, 30));
        button->setIcon(KisIconUtils::loadIcon(iconName));
        button->setToolTip(toolTip);
        return button;
    }

    void createTopBar()
    {
        topBar = new QFrame(mainWindow);
        topBar->setObjectName(QStringLiteral("KisIOSTouchTopBar"));
        topBar->setAttribute(Qt::WA_StyledBackground, true);
        topBar->setStyleSheet(PanelStyle);

        auto *layout = new QHBoxLayout(topBar);
        layout->setContentsMargins(8, 5, 8, 5);
        layout->setSpacing(3);

        galleryButton = createCustomButton(topBar,
                                           QStringLiteral("document-open-recent"),
                                           i18n("Close document"));
        galleryButton->setText(i18n("Gallery"));
        galleryButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        galleryButton->setFixedWidth(94);
        connect(galleryButton, &QToolButton::clicked, this, [this] {
            hidePanels();
            triggerAction(QStringLiteral("file_close"));
        });
        layout->addWidget(galleryButton);

        hideControlsButton = createCustomButton(topBar,
                                                QStringLiteral("novisible"),
                                                i18n("Hide iPad Drawing Controls"));
        connect(hideControlsButton, &QToolButton::clicked, this, [this] {
            if (touchUiAction) {
                touchUiAction->setChecked(false);
            }
        });
        layout->addWidget(hideControlsButton);

        const auto addSecondaryAction = [this, layout](const QString &id,
                                                        const QString &icon,
                                                        const QString &toolTip) {
            QToolButton *button = createActionButton(topBar, id, icon, toolTip);
            secondaryButtons.append(button);
            layout->addWidget(button);
        };

        addSecondaryAction(QStringLiteral("options_configure"),
                           QStringLiteral("configure"),
                           i18n("Configure LibrePaint"));
        addSecondaryAction(QStringLiteral("adjust_filters"),
                           QStringLiteral("filter-color-curves"),
                           i18n("Adjustments"));
        addSecondaryAction(QStringLiteral("KisToolSelectOutline"),
                           QStringLiteral("tool_outline_selection"),
                           i18n("Selection"));
        addSecondaryAction(QStringLiteral("KisToolTransform"),
                           QStringLiteral("krita_tool_transform"),
                           i18n("Transform"));

        layout->addStretch(1);

        brushButton = createCustomButton(topBar,
                                         QStringLiteral("preset-switcher"),
                                         i18n("Brush Library"));
        brushButton->setCheckable(true);
        connect(brushButton, &QToolButton::clicked, this, [this] {
            activateBrush();
            toggleBrushPanel();
            syncToolButtons(viewManager->canvasResourceProvider()->currentPreset());
        });
        layout->addWidget(brushButton);

        smudgeButton = createCustomButton(topBar,
                                          QStringLiteral("colorsmudge"),
                                          i18n("Color Smudge"));
        smudgeButton->setCheckable(true);
        connect(smudgeButton, &QToolButton::clicked, this, [this] {
            hidePanels();
            activateSmudge();
            syncToolButtons(viewManager->canvasResourceProvider()->currentPreset());
        });
        layout->addWidget(smudgeButton);

        eraserAction = findAction(QStringLiteral("eraser_preset_action"));
        eraserButton = createActionButton(topBar,
                                          QStringLiteral("eraser_select_preset_action"),
                                          QStringLiteral("draw-eraser"),
                                          i18n("Eraser"),
                                          true);
        if (eraserAction) {
            connect(eraserAction, &QAction::toggled, this, [this] {
                syncToolButtons(viewManager->canvasResourceProvider()->currentPreset());
            });
        }
        layout->addWidget(eraserButton);

        layerButton = createCustomButton(topBar,
                                         QStringLiteral("view-list-details"),
                                         i18n("Layers"));
        layerButton->setCheckable(true);
        connect(layerButton, &QToolButton::clicked, this, [this] {
            toggleLayerPanel();
        });
        layout->addWidget(layerButton);

        colorButton = createCustomButton(topBar, QString(), i18n("Foreground Color"));
        connect(colorButton, &QToolButton::clicked, this, [this] {
            hidePanels();
            ensureStylusDevice();
            triggerAction(QStringLiteral("chooseForegroundColor"));
        });
        layout->addWidget(colorButton);

        topBar->hide();
    }

    void createRestoreButton()
    {
        restoreControlsButton = createCustomButton(mainWindow,
                                                   QStringLiteral("visible"),
                                                   i18n("Show iPad Drawing Controls"));
        restoreControlsButton->setObjectName(QStringLiteral("KisIOSTouchRestoreButton"));
        restoreControlsButton->setFocusPolicy(Qt::NoFocus);
        restoreControlsButton->setStyleSheet(PanelStyle);
        connect(restoreControlsButton, &QToolButton::clicked, this, [this] {
            if (touchUiAction) {
                touchUiAction->setChecked(true);
            }
        });
        restoreControlsButton->hide();
    }

    void createSideBar()
    {
        sideBar = new QFrame(mainWindow);
        sideBar->setObjectName(QStringLiteral("KisIOSTouchSideBar"));
        sideBar->setAttribute(Qt::WA_StyledBackground, true);
        sideBar->setStyleSheet(PanelStyle);

        auto *layout = new QVBoxLayout(sideBar);
        layout->setContentsMargins(7, 10, 7, 8);
        layout->setSpacing(5);

        sizeLabel = new QLabel(sideBar);
        sizeLabel->setAlignment(Qt::AlignCenter);
        sizeLabel->setToolTip(i18n("Brush Size"));
        layout->addWidget(sizeLabel);

        sizeSlider = new QSlider(Qt::Vertical, sideBar);
        sizeSlider->setRange(0, SliderResolution);
        sizeSlider->setTracking(true);
        sizeSlider->setInvertedAppearance(false);
        sizeSlider->setToolTip(i18n("Brush Size"));
        layout->addWidget(sizeSlider, 1, Qt::AlignHCenter);

        opacityLabel = new QLabel(sideBar);
        opacityLabel->setAlignment(Qt::AlignCenter);
        opacityLabel->setToolTip(i18n("Brush Opacity"));
        layout->addWidget(opacityLabel);

        opacitySlider = new QSlider(Qt::Vertical, sideBar);
        opacitySlider->setRange(0, 100);
        opacitySlider->setTracking(true);
        opacitySlider->setInvertedAppearance(false);
        opacitySlider->setToolTip(i18n("Brush Opacity"));
        layout->addWidget(opacitySlider, 1, Qt::AlignHCenter);

        layout->addSpacing(3);
        layout->addWidget(createActionButton(sideBar,
                                             QStringLiteral("edit_undo"),
                                             QStringLiteral("edit-undo"),
                                             i18n("Undo")),
                          0,
                          Qt::AlignHCenter);
        layout->addWidget(createActionButton(sideBar,
                                             QStringLiteral("edit_redo"),
                                             QStringLiteral("edit-redo"),
                                             i18n("Redo")),
                          0,
                          Qt::AlignHCenter);

        connect(sizeSlider, &QSlider::valueChanged, this, [this](int value) {
            ensureStylusDevice();
            const qreal size = sliderToBrushSize(value);
            viewManager->canvasResourceProvider()->setSize(size);
            updateSizeLabel(size);
        });
        connect(opacitySlider, &QSlider::valueChanged, this, [this](int value) {
            ensureStylusDevice();
            viewManager->canvasResourceProvider()->setOpacity(qreal(value) / 100.0);
            updateOpacityLabel(qreal(value) / 100.0);
        });

        sideBar->hide();
    }

    void connectResourceState()
    {
        KisCanvasResourceProvider *provider = viewManager->canvasResourceProvider();
        connect(provider, &KisCanvasResourceProvider::sigOpacityChanged, this, [this](qreal value) {
            QSignalBlocker blocker(opacitySlider);
            opacitySlider->setValue(qRound(value * 100.0));
            updateOpacityLabel(value);
        });
        connect(provider, &KisCanvasResourceProvider::sigFGColorChanged, this, [this](const KoColor &color) {
            updateColorButton(color);
        });
        connect(provider, &KisCanvasResourceProvider::sigPaintOpPresetChanged, this, [this](const KisPaintOpPresetSP preset) {
            if (brushChooser) {
                brushChooser->canvasResourceChanged(preset);
            }
            rememberPreset(preset);
            syncToolButtons(preset);
        });

        if (provider->resourceManager()) {
            connect(provider->resourceManager(),
                    &KoCanvasResourceProvider::canvasResourceChanged,
                    this,
                    [this](int key, const QVariant &value) {
                        if (key == KoCanvasResource::Size) {
                            const qreal size = value.toReal();
                            QSignalBlocker blocker(sizeSlider);
                            sizeSlider->setValue(brushSizeToSlider(size));
                            updateSizeLabel(size);
                        }
                    });
        }

        {
            QSignalBlocker blocker(sizeSlider);
            sizeSlider->setValue(brushSizeToSlider(provider->size()));
        }
        {
            QSignalBlocker blocker(opacitySlider);
            opacitySlider->setValue(qRound(provider->opacity() * 100.0));
        }
        updateSizeLabel(provider->size());
        updateOpacityLabel(provider->opacity());
        updateColorButton(provider->fgColor());
        rememberPreset(provider->currentPreset());
        syncToolButtons(provider->currentPreset());
    }

    void ensureBrushPanel()
    {
        if (brushPanel || !mainWindow) {
            return;
        }

        brushPanel = new QFrame(mainWindow);
        brushPanel->setObjectName(QStringLiteral("KisIOSTouchBrushPanel"));
        brushPanel->setAttribute(Qt::WA_StyledBackground, true);
        brushPanel->setStyleSheet(PanelStyle);

        auto *layout = new QVBoxLayout(brushPanel);
        layout->setContentsMargins(8, 8, 8, 8);
        brushChooser = new KisPaintOpPresetsChooserPopup(brushPanel);
        brushChooser->setResponsiveness(true);
        layout->addWidget(brushChooser);

        connect(brushChooser,
                &KisPaintOpPresetsChooserPopup::resourceSelected,
                this,
                [this](KoResourceSP resource) {
                    selectPreset(resource);
                });
        connect(brushChooser,
                &KisPaintOpPresetsChooserPopup::resourceClicked,
                this,
                [this](KoResourceSP resource) {
                    selectPreset(resource);
                });

        if (KisPaintOpPresetSP preset = viewManager->canvasResourceProvider()->currentPreset()) {
            brushChooser->canvasResourceChanged(preset);
        }
        brushPanel->hide();
    }

    void ensureLayerPanel()
    {
        if (layerPanel || !mainWindow) {
            return;
        }

        KisConfig cfg(false);
        const QList<QString> layerDockers {QStringLiteral("KisLayerBox")};
        cfg.writeList(QStringLiteral("iostouchui-layer/dockerList"), layerDockers);
        cfg.writeEntry(QStringLiteral("iostouchui-layer/currentDocker"), QStringLiteral("KisLayerBox"));

        layerPanel = new QFrame(mainWindow);
        layerPanel->setObjectName(QStringLiteral("KisIOSTouchLayerPanel"));
        layerPanel->setAttribute(Qt::WA_StyledBackground, true);
        layerPanel->setStyleSheet(PanelStyle);

        auto *layout = new QVBoxLayout(layerPanel);
        layout->setContentsMargins(8, 8, 8, 8);
        layerHud = new KisDockerHud(i18n("iPad Touch UI"), QStringLiteral("iostouchui-layer"));
        layout->addWidget(layerHud);
        layerPanel->hide();
    }

    void toggleBrushPanel()
    {
        ensureBrushPanel();
        if (!brushPanel) {
            return;
        }

        if (brushPanel->isVisible()) {
            hidePanels();
            return;
        }

        hidePanels();
        brushPanel->show();
        popupAdapter->registerWith(viewManager->inputManager());
        scheduleLayout();
    }

    void toggleLayerPanel()
    {
        ensureLayerPanel();
        if (!layerPanel) {
            return;
        }

        if (layerPanel->isVisible()) {
            hidePanels();
            return;
        }

        hidePanels();
        layerPanel->show();
        layerButton->setChecked(true);
        popupAdapter->registerWith(viewManager->inputManager());
        scheduleLayout();
    }

    void hidePanels(bool finishPopup = true)
    {
        if (brushPanel) {
            brushPanel->hide();
        }
        if (layerPanel) {
            layerPanel->hide();
        }
        if (layerButton) {
            layerButton->setChecked(false);
        }
        if (finishPopup && popupAdapter) {
            popupAdapter->finish();
        }
    }

    void hideAllOverlays()
    {
        hidePanels();
        if (sideBar) {
            sideBar->hide();
        }
        if (topBar) {
            topBar->hide();
        }
        if (restoreControlsButton) {
            restoreControlsButton->hide();
        }
    }

    void applyCanvasOnlyStartupMode()
    {
        if (!touchUiAction) {
            return;
        }

        KisConfig cfg(true);
        const QString startupMode = cfg.readEntry<QString>(CanvasOnlyStartupModeKey,
                                                           StartupShowControls);

        bool showControls = true;
        if (startupMode == StartupRememberLast) {
            showControls = cfg.readEntry<bool>(LastControlsVisibleKey, true);
        } else if (startupMode == StartupHideControls) {
            showControls = false;
        }

        QSignalBlocker blocker(touchUiAction);
        touchUiAction->setChecked(showControls);
        if (!showControls) {
            hideAllOverlays();
        }
    }

    void selectPreset(const KoResourceSP &resource)
    {
        if (!resource || !viewManager->paintOpBox()) {
            return;
        }

        ensureStylusDevice();
        leaveEraserMode();
        KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
        viewManager->paintOpBox()->resourceSelected(resource);
    }

    void activateBrush()
    {
        ensureStylusDevice();
        leaveEraserMode();
        KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));

        KisPaintOpPresetSP current = viewManager->canvasResourceProvider()->currentPreset();
        if (current && current->paintOp().id() == QStringLiteral("colorsmudge") && lastBrushPreset) {
            viewManager->paintOpBox()->resourceSelected(lastBrushPreset);
        }
    }

    void activateSmudge()
    {
        ensureStylusDevice();
        leaveEraserMode();

        KisPaintOpPresetSP current = viewManager->canvasResourceProvider()->currentPreset();
        if (current && current->paintOp().id() != QStringLiteral("colorsmudge")) {
            lastBrushPreset = current;
        }

        if (!smudgePreset) {
            if (!resourceModel) {
                resourceModel = new KisResourceModel(ResourceType::PaintOpPresets, this);
            }
            for (int row = 0; row < resourceModel->rowCount(); ++row) {
                KisPaintOpPresetSP candidate = resourceModel
                    ->resourceForIndex(resourceModel->index(row, 0))
                    .dynamicCast<KisPaintOpPreset>();
                if (candidate && candidate->paintOp().id() == QStringLiteral("colorsmudge")) {
                    smudgePreset = candidate;
                    break;
                }
            }
        }

        if (!smudgePreset) {
            viewManager->showFloatingMessage(i18n("No Color Smudge preset is available"), QIcon(), 2500);
            ensureBrushPanel();
            if (brushPanel) {
                brushPanel->show();
                scheduleLayout();
            }
            return;
        }

        KoToolManager::instance()->switchToolRequested(QStringLiteral("KritaShape/KisToolBrush"));
        viewManager->paintOpBox()->resourceSelected(smudgePreset);
    }

    void leaveEraserMode()
    {
        if (eraserAction && eraserAction->isChecked() && eraserAction->isEnabled()) {
            triggerAction(QStringLiteral("brush_select_preset_action"));
        }
    }

    void ensureStylusDevice()
    {
        KoToolManager *toolManager = KoToolManager::instance();
        if (toolManager->currentInputDevice().isMouse()) {
            toolManager->switchInputDeviceRequested(KoInputDevice::stylus());
        }
    }

    void rememberPreset(const KisPaintOpPresetSP &preset)
    {
        if (!preset) {
            return;
        }

        if (preset->paintOp().id() == QStringLiteral("colorsmudge")) {
            smudgePreset = preset;
        } else if (!eraserAction || !eraserAction->isChecked()) {
            lastBrushPreset = preset;
        }
    }

    void syncToolButtons(const KisPaintOpPresetSP &preset)
    {
        if (!brushButton || !smudgeButton) {
            return;
        }

        const bool erasing = eraserAction && eraserAction->isChecked();
        const bool smudging = !erasing && preset
            && preset->paintOp().id() == QStringLiteral("colorsmudge");
        QSignalBlocker brushBlocker(brushButton);
        QSignalBlocker smudgeBlocker(smudgeButton);
        brushButton->setChecked(!erasing && !smudging);
        smudgeButton->setChecked(smudging);
    }

    qreal maximumBrushSize() const
    {
        return qMax<qreal>(1.0, KisImageConfig(true).maxBrushSize());
    }

    qreal sliderToBrushSize(int value) const
    {
        const qreal maximum = maximumBrushSize();
        const qreal ratio = maximum / MinimumBrushSize;
        const qreal position = qreal(value) / qreal(SliderResolution);
        return MinimumBrushSize * std::pow(ratio, position);
    }

    int brushSizeToSlider(qreal size) const
    {
        const qreal maximum = maximumBrushSize();
        const qreal clamped = qBound(MinimumBrushSize, size, maximum);
        const qreal position = std::log(clamped / MinimumBrushSize)
            / std::log(maximum / MinimumBrushSize);
        return qRound(position * SliderResolution);
    }

    void updateSizeLabel(qreal size)
    {
        if (!sizeLabel) {
            return;
        }
        sizeLabel->setText(size < 10.0
                               ? i18nc("Brush size in pixels", "%1 px", QString::number(size, 'f', 1))
                               : i18nc("Brush size in pixels", "%1 px", qRound(size)));
    }

    void updateOpacityLabel(qreal opacity)
    {
        if (opacityLabel) {
            opacityLabel->setText(i18nc("Brush opacity percentage", "%1%", qRound(opacity * 100.0)));
        }
    }

    void updateColorButton(const KoColor &color)
    {
        if (!colorButton) {
            return;
        }

        QPixmap swatch(34, 34);
        swatch.fill(Qt::transparent);
        QPainter painter(&swatch);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor(235, 235, 235), 2));
        painter.setBrush(color.toQColor());
        painter.drawEllipse(QRectF(3, 3, 28, 28));
        colorButton->setIcon(QIcon(swatch));
    }

    void scheduleLayout()
    {
        if (!applicationAllowsLayout()) {
            return;
        }

        if (layoutPending) {
            return;
        }

        layoutPending = true;
        const quint64 generation = layoutGeneration;
        QTimer::singleShot(0, this, [this, generation] {
            if (generation != layoutGeneration) {
                return;
            }

            layoutPending = false;
            if (!applicationAllowsLayout()) {
                return;
            }

            refreshCanvasAndLayout();
        });
    }

    void refreshCanvasAndLayout()
    {
        if (!applicationAllowsLayout()) {
            return;
        }

        QWidget *newCanvas = viewManager ? viewManager->canvas() : nullptr;
        if (newCanvas != canvas) {
            if (canvas) {
                canvas->removeEventFilter(this);
            }
            canvas = newCanvas;
            if (canvas) {
                canvas->installEventFilter(this);
            }

            QObject::disconnect(canvasEngineConnection);
            if (viewManager && viewManager->canvasBase()) {
                canvasEngineConnection = connect(viewManager->canvasBase(),
                                                 &KisCanvas2::sigCanvasEngineChanged,
                                                 this,
                                                 [this] {
                                                     scheduleLayout();
                                                 });
            }
        }

        QWindow *newWindow = mainWindow ? mainWindow->windowHandle() : nullptr;
        if (newWindow != window) {
            if (window) {
                window->removeEventFilter(this);
            }
            window = newWindow;
            if (window) {
                window->installEventFilter(this);
            }
        }

        const bool inCanvasOnly = canvasOnlyAction && canvasOnlyAction->isChecked();
        if (inCanvasOnly && !wasCanvasOnly) {
            applyCanvasOnlyStartupMode();
        } else if (!inCanvasOnly && wasCanvasOnly) {
            hideAllOverlays();
        }
        wasCanvasOnly = inCanvasOnly;

        const bool shouldShow = initialized && canvas && canvas->isVisible() && inCanvasOnly;
        if (!shouldShow) {
            hideAllOverlays();
            return;
        }

        QRect contentRect(canvas->mapTo(mainWindow, QPoint(0, 0)), canvas->size());
        if (window) {
            contentRect = contentRect.intersected(mainWindow->rect().marginsRemoved(window->safeAreaMargins()));
        }
        if (contentRect.width() <= 0 || contentRect.height() <= 0) {
            hideAllOverlays();
            return;
        }

        const QRect inner = contentRect.adjusted(OverlayMargin,
                                                 OverlayMargin,
                                                 -OverlayMargin,
                                                 -OverlayMargin);
        if (inner.width() < ToolButtonExtent || inner.height() < ToolButtonExtent) {
            hideAllOverlays();
            return;
        }

        const bool controlsVisible = !touchUiAction || touchUiAction->isChecked();
        if (!controlsVisible) {
            hidePanels();
            if (sideBar) {
                sideBar->hide();
            }
            if (topBar) {
                topBar->hide();
            }
            if (restoreControlsButton) {
                restoreControlsButton->setGeometry(inner.x(),
                                                   inner.y(),
                                                   ToolButtonExtent,
                                                   ToolButtonExtent);
                restoreControlsButton->show();
                restoreControlsButton->raise();
            }
            return;
        }

        if (restoreControlsButton) {
            restoreControlsButton->hide();
        }
        topBar->setGeometry(inner.x(), inner.y(), inner.width(), TopBarHeight);

        const bool showSecondary = inner.width() >= 720;
        galleryButton->show();
        for (const QPointer<QToolButton> &button : std::as_const(secondaryButtons)) {
            button->setVisible(showSecondary);
        }

        const int sideTop = inner.y() + TopBarHeight + OverlayMargin;
        const int sideHeight = inner.bottom() - sideTop + 1;
        sideBar->setGeometry(inner.x(), sideTop, SideBarWidth, qMax(0, sideHeight));
        sideBar->setVisible(sideHeight >= 260);

        if (sideHeight < 240) {
            hidePanels();
        }

        if (brushPanel) {
            const int panelWidth = qMin(inner.width(), qMin(620, qMax(300, inner.width() * 3 / 5)));
            const int panelHeight = qMin(650, qMax(0, sideHeight));
            brushPanel->setGeometry(inner.right() - panelWidth + 1,
                                    sideTop,
                                    panelWidth,
                                    panelHeight);
        }
        if (layerPanel) {
            const int panelWidth = qMin(inner.width(), qMin(480, qMax(300, inner.width() / 2)));
            const int panelHeight = qMin(650, qMax(0, sideHeight));
            layerPanel->setGeometry(inner.right() - panelWidth + 1,
                                    sideTop,
                                    panelWidth,
                                    panelHeight);
        }

        topBar->show();
        topBar->raise();
        if (sideBar->isVisible()) {
            sideBar->raise();
        }
        if (brushPanel && brushPanel->isVisible()) {
            brushPanel->raise();
        }
        if (layerPanel && layerPanel->isVisible()) {
            layerPanel->raise();
        }
    }

    bool applicationAllowsLayout() const
    {
#ifdef Q_OS_IOS
        return QGuiApplication::applicationState() == Qt::ApplicationActive;
#else
        return true;
#endif
    }

    void invalidatePendingLayout()
    {
        ++layoutGeneration;
        layoutPending = false;
    }

private:
    QPointer<KisViewManager> viewManager;
    QPointer<QWidget> mainWindow;
    QPointer<QWidget> canvas;
    QPointer<QWindow> window;
    QPointer<QAction> canvasOnlyAction;
    QPointer<QAction> touchUiAction;
    QPointer<QAction> eraserAction;
    QPointer<KisIOSTouchPopupAdapter> popupAdapter;

    QPointer<QFrame> topBar;
    QPointer<QFrame> sideBar;
    QPointer<QFrame> brushPanel;
    QPointer<QFrame> layerPanel;
    QPointer<KisPaintOpPresetsChooserPopup> brushChooser;
    QPointer<KisDockerHud> layerHud;

    QPointer<QToolButton> galleryButton;
    QPointer<QToolButton> hideControlsButton;
    QPointer<QToolButton> restoreControlsButton;
    QPointer<QToolButton> brushButton;
    QPointer<QToolButton> smudgeButton;
    QPointer<QToolButton> eraserButton;
    QPointer<QToolButton> layerButton;
    QPointer<QToolButton> colorButton;
    QList<QPointer<QToolButton>> secondaryButtons;

    QPointer<QLabel> sizeLabel;
    QPointer<QLabel> opacityLabel;
    QPointer<QSlider> sizeSlider;
    QPointer<QSlider> opacitySlider;

    QPointer<KisResourceModel> resourceModel;
    KisPaintOpPresetSP lastBrushPreset;
    KisPaintOpPresetSP smudgePreset;
    QMetaObject::Connection canvasEngineConnection;

    bool initialized {false};
    bool layoutPending {false};
    bool wasCanvasOnly {false};
    quint64 layoutGeneration {0};
};

KisIOSTouchUI::KisIOSTouchUI(QObject *parent, const QVariantList &args)
    : KisActionPlugin(parent)
    , d(std::make_unique<Private>(this,
                                  viewManager(),
                                  createAction(QStringLiteral("view_show_ios_touch_ui"))))
{
    Q_UNUSED(args)
}

KisIOSTouchUI::~KisIOSTouchUI() = default;

#include "iostouchui.moc"
