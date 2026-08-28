/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QAbstractButton>
#include <QAction>
#include <QEvent>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QTest>
#include <QWidget>

#include <array>
#include <memory>
#include <type_traits>

#include <kis_shared.h>

#include "selection/kis_selection_actions_panel.h"
#include "selection/kis_selection_actions_panel_source_p.h"

namespace Source = KisSelectionActionsPanelSource;

namespace
{

struct TestState
{
    QWidget canvas;
    QWidget replacementCanvas;
    QAction configureAction;
    QRectF selectionRect {100.0, 100.0, 200.0, 100.0};
    Source::PanelConfig config;
    QPoint dragOffset;
    QList<Source::Action> triggeredActions;
    QList<QPoint> writtenOffsets;
    int initializationCount {0};
    int backgroundDrawCount {0};
    int buttonDrawCount {0};
    int handleDrawCount {0};
    int disableCount {0};
    int configureCount {0};
    int buttonThemeCount {0};
    int handleThemeCount {0};
    QList<bool> heldStates;
    QList<Orientation> handleOrientations;
    bool selectionPresent {true};

    TestState()
    {
        canvas.resize(600, 400);
        replacementCanvas.resize(800, 600);
    }
};

TestState *state = nullptr;

KisViewManager *viewManager()
{
    return reinterpret_cast<KisViewManager *>(quintptr(1));
}

KisCanvasWidgetBase *replacementCanvas()
{
    return reinterpret_cast<KisCanvasWidgetBase *>(quintptr(2));
}

std::unique_ptr<QMouseEvent> mouseEvent(QEvent::Type type,
                                        const QPointF &position,
                                        Qt::MouseButton button,
                                        Qt::MouseButtons buttons)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return std::make_unique<QMouseEvent>(type,
                                         position,
                                         position,
                                         button,
                                         buttons,
                                         Qt::NoModifier);
#else
    return std::make_unique<QMouseEvent>(type,
                                         position,
                                         button,
                                         buttons,
                                         Qt::NoModifier);
#endif
}

QList<QAbstractButton *> buttons(QWidget &parent)
{
    return parent.findChildren<QAbstractButton *>(QString(),
                                                   Qt::FindDirectChildrenOnly);
}

QWidget *handle(QWidget &parent)
{
    return parent.findChild<QWidget *>(QStringLiteral("selection-actions-handle"),
                                       Qt::FindDirectChildrenOnly);
}

class Fixture
{
public:
    Fixture()
    {
        state = &data;
    }

    ~Fixture()
    {
        state = nullptr;
    }

    TestState data;
};

}

namespace KisSelectionActionsPanelSource
{

QVector<ActionButtonData> actionButtons()
{
    return {{QStringLiteral("select-all"), QStringLiteral("Select All"), Action::SelectAll},
            {QStringLiteral("select-invert"), QStringLiteral("Invert Selection"), Action::Invert},
            {QStringLiteral("select-clear"), QStringLiteral("Deselect"), Action::Deselect},
            {QStringLiteral("krita_tool_color_fill"), QStringLiteral("Fill Selection with Color"), Action::FillForegroundColor},
            {QStringLiteral("draw-eraser"), QStringLiteral("Clear Selection"), Action::Clear},
            {QStringLiteral("duplicatelayer"), QStringLiteral("Copy To New Layer"), Action::CopyToNewLayer},
            {QStringLiteral("tool_crop"), QStringLiteral("Crop to Selection"), Action::CropToSelection},
            {QStringLiteral("krita_tool_reference_images"), QStringLiteral("Toggle pin selection actions bar"), Action::TogglePin}};
}

QWidget *canvasWidget(KisViewManager *)
{
    return &state->canvas;
}

QWidget *canvasWidget(KisCanvasWidgetBase *canvas)
{
    return canvas ? &state->replacementCanvas : nullptr;
}

bool hasSelection(KisViewManager *)
{
    return state->selectionPresent;
}

QRectF selectionRect(KisViewManager *)
{
    return state->selectionRect;
}

QAbstractButton *createActionButton(const ActionButtonData &data,
                                    int size,
                                    QWidget *parent)
{
    auto *button = new QPushButton(parent);
    button->setToolTip(data.tooltip);
    button->setFixedSize(size, size);
    return button;
}

void connectActionButton(QAbstractButton *button,
                         KisViewManager *,
                         Action action)
{
    QObject::connect(button, &QAbstractButton::clicked, [action]() {
        state->triggeredActions.append(action);
    });
}

QWidget *createHandle(int size, Orientation, QWidget *parent)
{
    auto *widget = new QWidget(parent);
    widget->setFixedSize(size, size);
    return widget;
}

QAction *createDisableAction(QObject *parent)
{
    return new QAction(QStringLiteral("Disable"), parent);
}

QAction *configureAction(KisViewManager *)
{
    return &state->configureAction;
}

void connectEnvironment(KisViewManager *, QObject *)
{
}

void initializeLegacyPosition()
{
    ++state->initializationCount;
}

PanelConfig panelConfig()
{
    return state->config;
}

QPoint dragOffset()
{
    return state->dragOffset;
}

void writeDragOffset(const QPoint &offset)
{
    state->writtenOffsets.append(offset);
}

void disablePanel()
{
    ++state->disableCount;
}

void configurePanel(KisViewManager *)
{
    ++state->configureCount;
}

void drawBackground(QPainter &,
                    const KoColorDisplayRendererInterface *,
                    const QPoint &,
                    const QSize &,
                    const QSize &)
{
    ++state->backgroundDrawCount;
}

void drawActionButton(QAbstractButton *,
                      QPainter &,
                      const KoColorDisplayRendererInterface *)
{
    ++state->buttonDrawCount;
}

void drawHandle(QWidget *,
                QPainter &,
                const KoColorDisplayRendererInterface *)
{
    ++state->handleDrawCount;
}

void setHandleHeld(QWidget *, bool held)
{
    state->heldStates.append(held);
}

void setHandleOrientation(QWidget *, Orientation orientation)
{
    state->handleOrientations.append(orientation);
}

void updateTheme(QAbstractButton *)
{
    ++state->buttonThemeCount;
}

void updateHandleTheme(QWidget *)
{
    ++state->handleThemeCount;
}

}

class KisSelectionActionsPanelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void publicTypesAndConstructionRegisterStableActions();
    void visibilityAndEnablementFollowSelectionState();
    void orientationHandleAndCanvasChangesRelayoutWidgets();
    void drawDispatchesOnlyForVisibleEnabledSelections();
    void pointerEventsMoveAndPersistThePanel();
};

void KisSelectionActionsPanelContractTest::publicTypesAndConstructionRegisterStableActions()
{
    static_assert(std::is_base_of_v<QObject, KisSelectionActionsPanel>);
    static_assert(!std::is_default_constructible_v<KisSelectionActionsPanel>);
    static_assert(std::is_same_v<Orientation, KisConfig::SelectionActionsBarOrientation>);
    static_assert(std::is_same_v<Position, KisConfig::SelectionActionsBarPosition>);
    static_assert(std::is_same_v<Behavior, KisConfig::SelectionActionsBarBehavior>);
    static_assert(std::is_same_v<KisSelectionActionsPanelSP,
                                 KisSharedPtr<KisSelectionActionsPanel>>);
    static_assert(!std::is_base_of_v<KisShared, KisSelectionActionsPanel>);

    Fixture fixture;
    QPointer<KisSelectionActionsPanel> guard;
    {
        KisSelectionActionsPanel panel(viewManager());
        guard = &panel;
        QCOMPARE(panel.parent(), nullptr);
        QCOMPARE(fixture.data.initializationCount, 1);

        const QList<QAbstractButton *> actionButtons = buttons(fixture.data.canvas);
        QCOMPARE(actionButtons.size(), 8);
        const std::array<QString, 8> names {{
            QStringLiteral("select-all"),
            QStringLiteral("select-invert"),
            QStringLiteral("select-clear"),
            QStringLiteral("krita_tool_color_fill"),
            QStringLiteral("draw-eraser"),
            QStringLiteral("duplicatelayer"),
            QStringLiteral("tool_crop"),
            QStringLiteral("krita_tool_reference_images")
        }};
        for (qsizetype i = 0; i < actionButtons.size(); ++i) {
            QCOMPARE(actionButtons[i]->objectName(), names[std::size_t(i)]);
            QCOMPARE(actionButtons[i]->size(), QSize(30, 30));
            actionButtons[i]->click();
        }
        QCOMPARE(fixture.data.triggeredActions.size(), 8);
        QCOMPARE(fixture.data.triggeredActions.front(), Source::Action::SelectAll);
        QCOMPARE(fixture.data.triggeredActions.back(), Source::Action::TogglePin);
        QVERIFY(!actionButtons.front()->isCheckable());
        QVERIFY(actionButtons.back()->isCheckable());
        QVERIFY(handle(fixture.data.canvas));
    }
    QVERIFY(guard.isNull());
}

void KisSelectionActionsPanelContractTest::visibilityAndEnablementFollowSelectionState()
{
    Fixture fixture;
    KisSelectionActionsPanel panel(viewManager());
    const QList<QAbstractButton *> actionButtons = buttons(fixture.data.canvas);
    QWidget *handleWidget = handle(fixture.data.canvas);

    panel.setVisible(true);
    for (QAbstractButton *button : actionButtons) {
        QVERIFY(!button->isHidden());
    }
    QVERIFY(!handleWidget->isHidden());
    QVERIFY(fixture.data.configureAction.isVisible());

    panel.setEnabled(false);
    for (QAbstractButton *button : actionButtons) {
        QVERIFY(button->isHidden());
    }
    QVERIFY(handleWidget->isHidden());
    QVERIFY(!fixture.data.configureAction.isVisible());

    panel.setEnabled(true);
    for (QAbstractButton *button : actionButtons) {
        QVERIFY(!button->isHidden());
    }
    QVERIFY(!handleWidget->isHidden());

    fixture.data.selectionPresent = false;
    panel.setEnabled(false);
    panel.setEnabled(true);
    for (QAbstractButton *button : actionButtons) {
        QVERIFY(button->isHidden());
    }
    QVERIFY(handleWidget->isHidden());
    QVERIFY(!fixture.data.configureAction.isVisible());
}

void KisSelectionActionsPanelContractTest::orientationHandleAndCanvasChangesRelayoutWidgets()
{
    Fixture fixture;
    KisSelectionActionsPanel panel(viewManager());
    panel.setVisible(true);

    QList<QAbstractButton *> actionButtons = buttons(fixture.data.canvas);
    QWidget *handleWidget = handle(fixture.data.canvas);
    QCOMPARE(actionButtons[1]->pos() - actionButtons[0]->pos(), QPoint(30, 0));
    QCOMPARE(handleWidget->pos() - actionButtons[7]->pos(), QPoint(30, 0));

    panel.setOrientation(Orientation::Vertical);
    QCOMPARE(fixture.data.handleOrientations.back(), Orientation::Vertical);
    QCOMPARE(actionButtons[1]->pos() - actionButtons[0]->pos(), QPoint(0, 30));
    QCOMPARE(handleWidget->pos() - actionButtons[7]->pos(), QPoint(0, 30));

    panel.setHandleEnabled(false);
    QVERIFY(!handleWidget->isEnabled());
    panel.updatePositioning();

    panel.canvasWidgetChanged(replacementCanvas());
    actionButtons = buttons(fixture.data.replacementCanvas);
    handleWidget = handle(fixture.data.replacementCanvas);
    QCOMPARE(actionButtons.size(), 8);
    QVERIFY(handleWidget);
    for (QAbstractButton *button : actionButtons) {
        QVERIFY(!button->isHidden());
    }
}

void KisSelectionActionsPanelContractTest::drawDispatchesOnlyForVisibleEnabledSelections()
{
    Fixture fixture;
    KisSelectionActionsPanel panel(viewManager());
    QImage image(600, 400, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&image);

    panel.draw(painter, nullptr);
    QCOMPARE(fixture.data.backgroundDrawCount, 0);

    panel.setVisible(true);
    panel.draw(painter, nullptr);
    QCOMPARE(fixture.data.backgroundDrawCount, 1);
    QCOMPARE(fixture.data.buttonDrawCount, 8);
    QCOMPARE(fixture.data.handleDrawCount, 1);

    panel.setHandleEnabled(false);
    panel.draw(painter, nullptr);
    QCOMPARE(fixture.data.backgroundDrawCount, 2);
    QCOMPARE(fixture.data.buttonDrawCount, 16);
    QCOMPARE(fixture.data.handleDrawCount, 1);

    panel.setEnabled(false);
    panel.draw(painter, nullptr);
    QCOMPARE(fixture.data.backgroundDrawCount, 2);
}

void KisSelectionActionsPanelContractTest::pointerEventsMoveAndPersistThePanel()
{
    Fixture fixture;
    KisSelectionActionsPanel panel(viewManager());
    panel.setVisible(true);
    QWidget *handleWidget = handle(fixture.data.canvas);
    const QPoint initialPosition = handleWidget->pos();

    QEvent otherEvent(QEvent::User);
    QVERIFY(!panel.eventFilter(handleWidget, &otherEvent));

    auto rightPress = mouseEvent(QEvent::MouseButtonPress,
                                 QPointF(5.0, 5.0),
                                 Qt::RightButton,
                                 Qt::RightButton);
    QVERIFY(!panel.eventFilter(handleWidget, rightPress.get()));

    auto press = mouseEvent(QEvent::MouseButtonPress,
                            QPointF(5.0, 5.0),
                            Qt::LeftButton,
                            Qt::LeftButton);
    QVERIFY(panel.eventFilter(handleWidget, press.get()));
    QVERIFY(press->isAccepted());

    auto move = mouseEvent(QEvent::MouseMove,
                           QPointF(20.0, 15.0),
                           Qt::NoButton,
                           Qt::LeftButton);
    QVERIFY(panel.eventFilter(handleWidget, move.get()));
    QVERIFY(move->isAccepted());
    QVERIFY(handleWidget->pos() != initialPosition);

    auto release = mouseEvent(QEvent::MouseButtonRelease,
                              QPointF(20.0, 15.0),
                              Qt::LeftButton,
                              Qt::NoButton);
    QVERIFY(panel.eventFilter(handleWidget, release.get()));
    QVERIFY(release->isAccepted());
    QCOMPARE(fixture.data.heldStates, QList<bool>({true, false}));
    QCOMPARE(fixture.data.writtenOffsets.size(), 1);
    QVERIFY(!fixture.data.writtenOffsets.front().isNull());
}

QTEST_MAIN(KisSelectionActionsPanelContractTest)

#include "KisSelectionActionsPanelContractTest.moc"
