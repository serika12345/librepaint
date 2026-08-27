/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QCursor>
#include <QMouseEvent>
#include <QPainter>
#include <QPointingDevice>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTabletEvent>
#include <QTest>
#include <QTouchEvent>
#include <QWidget>
#include <QtGui/private/qeventpoint_p.h>

#include <memory>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KisInputActionGroup.h>
#include <KisToolCanvas.h>
#include <KoCanvasBase.h>
#include <KoCanvasController.h>
#include <KoPointerEvent.h>
#include <KoSelectedShapesProxySimple.h>
#include <KoShapeControllerBase.h>
#include <KoShapeManager.h>
#include <KoToolProxy_p.h>
#include <KoUnit.h>
#include <kactioncollection.h>
#include <kis_config_notifier.h>
#include <kis_coordinates_converter.h>
#include <kis_tool.h>
#include <kis_tool_proxy.h>

namespace
{
class TestShapeController final : public KoShapeControllerBase
{
public:
    QRectF documentRectInPixels() const override
    {
        return QRectF(0.0, 0.0, 500.0, 500.0);
    }

    qreal pixelsPerInch() const override
    {
        return 100.0;
    }
};

class TestInputActionGroupsMaskState final : public KisInputActionGroupsMaskInterface
{
public:
    KisInputActionGroupsMask inputActionGroupsMask() const override
    {
        return m_mask;
    }

    void setInputActionGroupsMask(KisInputActionGroupsMask value) override
    {
        m_mask = value;
    }

private:
    KisInputActionGroupsMask m_mask{AllActionGroup};
};

class TestToolCanvas final : public KoCanvasBase, public KisToolCanvas
{
public:
    explicit TestToolCanvas(KoShapeControllerBase *shapeController)
        : KoCanvasBase(shapeController)
        , m_shapeManager(new KoShapeManager(this))
        , m_selectedShapesProxy(new KoSelectedShapesProxySimple(m_shapeManager.data()))
        , m_inputActionGroupsMask(QSharedPointer<TestInputActionGroupsMaskState>::create())
    {
        m_converter.setResolution(100.0, 100.0);
        m_converter.setZoom(1.0);
        m_converter.setDocumentOffset(QPoint(20, 30));
        m_converter.setCanvasWidgetSize(QSize(500, 500));
    }

    void setToolProxy(KisToolProxy *proxy)
    {
        m_toolProxy = proxy;
    }

    void gridSize(QPointF *offset, QSizeF *spacing) const override
    {
        *offset = QPointF();
        *spacing = QSizeF(10.0, 10.0);
    }

    bool snapToGrid() const override
    {
        return false;
    }

    void setCursor(const QCursor &) override
    {
    }

    void addCommand(KUndo2Command *) override
    {
    }

    KoShapeManager *shapeManager() const override
    {
        return m_shapeManager.data();
    }

    KoSelectedShapesProxy *selectedShapesProxy() const override
    {
        return m_selectedShapesProxy.data();
    }

    void updateCanvas(const QRectF &) override
    {
    }

    KoToolProxy *toolProxy() const override
    {
        return m_toolProxy;
    }

    const KoViewConverter *viewConverter() const override
    {
        return &m_converter;
    }

    KoViewConverter *viewConverter() override
    {
        return &m_converter;
    }

    QWidget *canvasWidget() override
    {
        return &m_widget;
    }

    const QWidget *canvasWidget() const override
    {
        return &m_widget;
    }

    KoUnit unit() const override
    {
        return KoUnit(KoUnit::Point);
    }

    const KisCoordinatesConverter *coordinatesConverter() const override
    {
        return &m_converter;
    }

    KisImageWSP currentImage() const override
    {
        return {};
    }

    KisSelectionSP currentSelectionForTool() const override
    {
        return {};
    }

    KisNodeList selectedNodesForTool() const override
    {
        return {};
    }

    void attachPriorityEventFilterForTool(QObject *, int) override
    {
    }

    void detachPriorityEventFilterForTool(QObject *) override
    {
    }

    void requestStrokeEndForTool() override
    {
    }

    void requestStrokeCancellationForTool() override
    {
    }

    bool blockUntilOperationsFinishedForTool(KisImageSP) override
    {
        return true;
    }

    void blockUntilOperationsFinishedForToolForced(KisImageSP) override
    {
    }

    bool selectionEditableForTool() const override
    {
        return true;
    }

    bool selectionModifierMappingSwapsCtrlAndAltForTool() const override
    {
        return false;
    }

    QCursor moveSelectionCursorForTool() const override
    {
        return QCursor(Qt::SizeAllCursor);
    }

    void showToolMessage(const QString &, const QString &) override
    {
    }

    void drawToolOutline(QPainter *, const KisOptimizedBrushOutline &, int) override
    {
    }

    QObject *toolConfigNotifier() const override
    {
        return KisConfigNotifier::instance();
    }

    void setInputEventFilterConnection(std::function<void(QObject *, bool, int)>) override
    {
    }

    void setInputCanvasWidgetChangedCallback(std::function<void()>) override
    {
    }

    KisInputActionGroupsMaskInterface::SharedInterface inputActionGroupsMaskInterface() override
    {
        return m_inputActionGroupsMask;
    }

private:
    KisCoordinatesConverter m_converter;
    QScopedPointer<KoShapeManager> m_shapeManager;
    QScopedPointer<KoSelectedShapesProxySimple> m_selectedShapesProxy;
    KisInputActionGroupsMaskInterface::SharedInterface m_inputActionGroupsMask;
    QWidget m_widget;
    KisToolProxy *m_toolProxy{nullptr};
};

class TestCanvasController final : public KoCanvasController
{
public:
    TestCanvasController(KoCanvasBase *canvas, KisKActionCollection *actionCollection)
        : KoCanvasController(actionCollection)
        , m_canvas(canvas)
    {
        m_canvas->setCanvasController(this);
    }

    void setCanvas(KoCanvasBase *canvas) override
    {
        m_canvas = canvas;
        m_canvas->setCanvasController(this);
    }

    KoCanvasBase *canvas() const override
    {
        return m_canvas;
    }

    void ensureVisibleDoc(const QRectF &, bool) override
    {
    }

    void zoomIn(const KoViewTransformStillPoint &) override
    {
    }

    void zoomIn() override
    {
    }

    void zoomOut(const KoViewTransformStillPoint &) override
    {
    }

    void zoomOut() override
    {
    }

    void zoomTo(const QRect &) override
    {
    }

    void setZoom(KoZoomMode::Mode, qreal) override
    {
    }

    void setPreferredCenter(const QPointF &point) override
    {
        m_preferredCenter = point;
    }

    QPointF preferredCenter() const override
    {
        return m_preferredCenter;
    }

    void pan(const QPoint &) override
    {
    }

    void panUp() override
    {
    }

    void panDown() override
    {
    }

    void panLeft() override
    {
    }

    void panRight() override
    {
    }

    QPoint scrollBarValue() const override
    {
        return {};
    }

    void setScrollBarValue(const QPoint &) override
    {
    }

    void resetScrollBars() override
    {
    }

    QPointF currentCursorPosition() const override
    {
        return {};
    }

    KoZoomState zoomState() const override
    {
        return {};
    }

private:
    KoCanvasBase *m_canvas;
    QPointF m_preferredCenter;
};

struct PointerSnapshot {
    QString phase;
    QPointF documentPoint;
    QPoint localPoint;
    QPoint globalPoint;
    Qt::MouseButton button;
    Qt::MouseButtons buttons;
    Qt::KeyboardModifiers modifiers;
    ulong timestamp;
    qreal pressure;
    qreal xTilt;
    qreal yTilt;
    qreal tangentialPressure;
    qreal rotation;
    int z;
    bool tablet;
    bool touch;
};

class RecordingTool final : public KisTool
{
public:
    explicit RecordingTool(KoCanvasBase *canvas)
        : KisTool(canvas, QCursor(Qt::ArrowCursor))
    {
    }

    void paint(QPainter &, const KoViewConverter &) override
    {
    }

    void beginPrimaryAction(KoPointerEvent *event) override
    {
        record(QStringLiteral("begin"), event);
        if (ignoreBegin) {
            event->ignore();
        }
    }

    void continuePrimaryAction(KoPointerEvent *event) override
    {
        record(QStringLiteral("continue"), event);
    }

    void endPrimaryAction(KoPointerEvent *event) override
    {
        record(QStringLiteral("end"), event);
    }

    QVector<PointerSnapshot> snapshots;
    bool ignoreBegin{false};

private:
    void record(const QString &phase, KoPointerEvent *event)
    {
        snapshots.append({phase,
                          event->point,
                          event->pos(),
                          event->globalPos(),
                          event->button(),
                          event->buttons(),
                          event->modifiers(),
                          event->time(),
                          event->pressure(),
                          event->xTilt(),
                          event->yTilt(),
                          event->tangentialPressure(),
                          event->rotation(),
                          event->z(),
                          event->isTabletEvent(),
                          event->isTouchEvent()});
    }
};

std::unique_ptr<QMouseEvent> mouseEvent(QEvent::Type type,
                                        const QPointF &localPoint,
                                        const QPointF &globalPoint,
                                        Qt::MouseButton button,
                                        Qt::MouseButtons buttons,
                                        ulong timestamp)
{
    auto event =
        std::make_unique<QMouseEvent>(type, localPoint, localPoint, globalPoint, button, buttons, Qt::ShiftModifier);
    event->setTimestamp(timestamp);
    return event;
}

std::unique_ptr<QTabletEvent> tabletEvent(QEvent::Type type,
                                          const QPointingDevice *device,
                                          const QPointF &localPoint,
                                          const QPointF &globalPoint,
                                          qreal pressure,
                                          float xTilt,
                                          float yTilt,
                                          float tangentialPressure,
                                          qreal rotation,
                                          float z,
                                          Qt::MouseButton button,
                                          Qt::MouseButtons buttons,
                                          ulong timestamp)
{
    auto event = std::make_unique<QTabletEvent>(type,
                                                device,
                                                localPoint,
                                                globalPoint,
                                                pressure,
                                                xTilt,
                                                yTilt,
                                                tangentialPressure,
                                                rotation,
                                                z,
                                                Qt::ShiftModifier,
                                                button,
                                                buttons);
    event->setTimestamp(timestamp);
    return event;
}

std::unique_ptr<QTouchEvent> touchEvent(QEvent::Type type,
                                        const QPointingDevice *device,
                                        QEventPoint::State state,
                                        const QPointF &localPoint,
                                        const QPointF &globalPoint,
                                        qreal pressure,
                                        qreal rotation,
                                        ulong timestamp)
{
    QEventPoint point(7, state, localPoint, globalPoint);
    QMutableEventPoint::setDevice(point, device);
    QMutableEventPoint::setPosition(point, localPoint);
    QMutableEventPoint::setPressure(point, pressure);
    QMutableEventPoint::setRotation(point, rotation);

    auto event = std::make_unique<QTouchEvent>(
        type, device, Qt::ShiftModifier, QList<QEventPoint>{point});
    event->setTimestamp(timestamp);
    return event;
}

void compareSnapshot(const PointerSnapshot &snapshot,
                     const QString &phase,
                     const QPointF &documentPoint,
                     const QPoint &localPoint,
                     const QPoint &globalPoint,
                     Qt::MouseButton button,
                     Qt::MouseButtons buttons,
                     ulong timestamp)
{
    QCOMPARE(snapshot.phase, phase);
    QCOMPARE(snapshot.documentPoint, documentPoint);
    QCOMPARE(snapshot.localPoint, localPoint);
    QCOMPARE(snapshot.globalPoint, globalPoint);
    QCOMPARE(snapshot.button, button);
    QCOMPARE(snapshot.buttons, buttons);
    QCOMPARE(snapshot.modifiers, Qt::KeyboardModifiers(Qt::ShiftModifier));
    QCOMPARE(snapshot.timestamp, timestamp);
    QCOMPARE(snapshot.pressure, 1.0);
    QCOMPARE(snapshot.xTilt, 0.0);
    QCOMPARE(snapshot.yTilt, 0.0);
    QCOMPARE(snapshot.tangentialPressure, 0.0);
    QCOMPARE(snapshot.rotation, 0.0);
    QCOMPARE(snapshot.z, 0);
    QVERIFY(!snapshot.tablet);
    QVERIFY(!snapshot.touch);
}

void compareTabletSnapshot(const PointerSnapshot &snapshot,
                           const QString &phase,
                           const QPointF &documentPoint,
                           const QPoint &localPoint,
                           const QPoint &globalPoint,
                           Qt::MouseButton button,
                           Qt::MouseButtons buttons,
                           ulong timestamp,
                           qreal pressure,
                           qreal xTilt,
                           qreal yTilt,
                           qreal tangentialPressure,
                           qreal rotation,
                           int z)
{
    QCOMPARE(snapshot.phase, phase);
    QCOMPARE(snapshot.documentPoint, documentPoint);
    QCOMPARE(snapshot.localPoint, localPoint);
    QCOMPARE(snapshot.globalPoint, globalPoint);
    QCOMPARE(snapshot.button, button);
    QCOMPARE(snapshot.buttons, buttons);
    QCOMPARE(snapshot.modifiers, Qt::KeyboardModifiers(Qt::ShiftModifier));
    QCOMPARE(snapshot.timestamp, timestamp);
    QVERIFY(qAbs(snapshot.pressure - pressure) < 0.000001);
    QCOMPARE(snapshot.xTilt, xTilt);
    QCOMPARE(snapshot.yTilt, yTilt);
    QVERIFY(qAbs(snapshot.tangentialPressure - tangentialPressure) < 0.000001);
    QCOMPARE(snapshot.rotation, rotation);
    QCOMPARE(snapshot.z, z);
    QVERIFY(snapshot.tablet);
    QVERIFY(!snapshot.touch);
}

void compareTouchSnapshot(const PointerSnapshot &snapshot,
                          const QString &phase,
                          const QPointF &documentPoint,
                          const QPoint &localPoint,
                          const QPoint &globalPoint,
                          ulong timestamp,
                          qreal pressure,
                          qreal rotation)
{
    QCOMPARE(snapshot.phase, phase);
    QCOMPARE(snapshot.documentPoint, documentPoint);
    QCOMPARE(snapshot.localPoint, localPoint);
    QCOMPARE(snapshot.globalPoint, globalPoint);
    QCOMPARE(snapshot.button, Qt::LeftButton);
    QCOMPARE(snapshot.buttons, Qt::MouseButtons(Qt::LeftButton));
    QCOMPARE(snapshot.modifiers, Qt::KeyboardModifiers(Qt::ShiftModifier));
    QCOMPARE(snapshot.timestamp, timestamp);
    QVERIFY(qAbs(snapshot.pressure - pressure) < 0.000001);
    QCOMPARE(snapshot.xTilt, 0.0);
    QCOMPARE(snapshot.yTilt, 0.0);
    QCOMPARE(snapshot.tangentialPressure, 0.0);
    QCOMPARE(snapshot.rotation, rotation);
    QCOMPARE(snapshot.z, 0);
    QVERIFY(!snapshot.tablet);
    QVERIFY(snapshot.touch);
}
} // namespace

class KisToolProxyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void forwardsMouseStrokeToCurrentTool();
    void forwardsTabletStrokeToCurrentTool();
    void forwardsSingleTouchStrokeToCurrentTool();
    void reportsIgnoredMousePress();

private:
    bool m_hadTouchPressureSetting{false};
    bool m_previousTouchPressureSetting{true};
};

void KisToolProxyContractTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    m_hadTouchPressureSetting = config.hasKey("useTouchPressureSensitivity");
    m_previousTouchPressureSetting = config.readEntry("useTouchPressureSensitivity", true);
    config.writeEntry("useTouchPressureSensitivity", true);
    config.sync();
    KisConfigNotifier::instance()->notifyConfigChanged();
}

void KisToolProxyContractTest::cleanupTestCase()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    if (m_hadTouchPressureSetting) {
        config.writeEntry("useTouchPressureSensitivity", m_previousTouchPressureSetting);
    } else {
        config.deleteEntry("useTouchPressureSensitivity");
    }
    config.sync();
    KisConfigNotifier::instance()->notifyConfigChanged();
}

void KisToolProxyContractTest::forwardsMouseStrokeToCurrentTool()
{
    TestShapeController shapeController;
    TestToolCanvas canvas(&shapeController);
    KisToolProxy proxy(&canvas);
    KisKActionCollection actionCollection(this);
    TestCanvasController controller(&canvas, &actionCollection);
    proxy.priv()->controller = &controller;
    canvas.setToolProxy(&proxy);
    RecordingTool tool(&canvas);
    proxy.setActiveTool(&tool);
    QSignalSpy activationSpy(&proxy, &KisToolProxy::toolPrimaryActionActivated);

    auto press = mouseEvent(QEvent::MouseButtonPress,
                            QPointF(100.0, 120.0),
                            QPointF(1000.0, 1200.0),
                            Qt::LeftButton,
                            Qt::LeftButton,
                            10);
    auto move =
        mouseEvent(QEvent::MouseMove, QPointF(140.0, 180.0), QPointF(1040.0, 1260.0), Qt::NoButton, Qt::LeftButton, 20);
    auto release = mouseEvent(QEvent::MouseButtonRelease,
                              QPointF(160.0, 210.0),
                              QPointF(1060.0, 1290.0),
                              Qt::LeftButton,
                              Qt::NoButton,
                              30);

    QVERIFY(proxy.forwardEvent(KisToolProxy::BEGIN, KisTool::Primary, press.get(), press.get()));
    QVERIFY(proxy.forwardEvent(KisToolProxy::CONTINUE, KisTool::Primary, move.get(), move.get()));
    QVERIFY(proxy.forwardEvent(KisToolProxy::END, KisTool::Primary, release.get(), release.get()));

    QCOMPARE(tool.snapshots.size(), 3);
    compareSnapshot(tool.snapshots.at(0),
                    QStringLiteral("begin"),
                    QPointF(0.5, 0.7),
                    QPoint(100, 120),
                    QPoint(1000, 1200),
                    Qt::LeftButton,
                    Qt::LeftButton,
                    10);
    compareSnapshot(tool.snapshots.at(1),
                    QStringLiteral("continue"),
                    QPointF(0.9, 1.3),
                    QPoint(140, 180),
                    QPoint(1040, 1260),
                    Qt::NoButton,
                    Qt::LeftButton,
                    20);
    compareSnapshot(tool.snapshots.at(2),
                    QStringLiteral("end"),
                    QPointF(1.1, 1.6),
                    QPoint(160, 210),
                    QPoint(1060, 1290),
                    Qt::LeftButton,
                    Qt::NoButton,
                    30);

    QCOMPARE(activationSpy.count(), 2);
    QCOMPARE(activationSpy.at(0).at(0).toBool(), true);
    QCOMPARE(activationSpy.at(1).at(0).toBool(), false);
}

void KisToolProxyContractTest::forwardsTabletStrokeToCurrentTool()
{
    TestShapeController shapeController;
    TestToolCanvas canvas(&shapeController);
    KisToolProxy proxy(&canvas);
    KisKActionCollection actionCollection(this);
    TestCanvasController controller(&canvas, &actionCollection);
    proxy.priv()->controller = &controller;
    canvas.setToolProxy(&proxy);
    RecordingTool tool(&canvas);
    proxy.setActiveTool(&tool);
    QSignalSpy activationSpy(&proxy, &KisToolProxy::toolPrimaryActionActivated);

    const QInputDevice::Capabilities capabilities = QInputDevice::Capability::Position
        | QInputDevice::Capability::Pressure | QInputDevice::Capability::Rotation
        | QInputDevice::Capability::XTilt | QInputDevice::Capability::YTilt
        | QInputDevice::Capability::TangentialPressure | QInputDevice::Capability::ZPosition;
    const QPointingDevice device(QStringLiteral("R2-G10 contract stylus"),
                                 17,
                                 QInputDevice::DeviceType::Stylus,
                                 QPointingDevice::PointerType::Pen,
                                 capabilities,
                                 1,
                                 3,
                                 QStringLiteral("contract seat"),
                                 QPointingDeviceUniqueId::fromNumericId(23));

    auto press = tabletEvent(QEvent::TabletPress,
                             &device,
                             QPointF(100.0, 120.0),
                             QPointF(1000.0, 1200.0),
                             0.25,
                             -15.0F,
                             20.0F,
                             -0.6F,
                             30.0,
                             3.0F,
                             Qt::LeftButton,
                             Qt::LeftButton,
                             110);
    auto move = tabletEvent(QEvent::TabletMove,
                            &device,
                            QPointF(140.0, 180.0),
                            QPointF(1040.0, 1260.0),
                            0.5,
                            -5.0F,
                            10.0F,
                            0.0F,
                            45.0,
                            4.0F,
                            Qt::NoButton,
                            Qt::LeftButton,
                            120);
    auto release = tabletEvent(QEvent::TabletRelease,
                               &device,
                               QPointF(160.0, 210.0),
                               QPointF(1060.0, 1290.0),
                               0.0,
                               12.0F,
                               -8.0F,
                               0.8F,
                               60.0,
                               5.0F,
                               Qt::LeftButton,
                               Qt::NoButton,
                               130);

    QVERIFY(proxy.forwardEvent(KisToolProxy::BEGIN, KisTool::Primary, press.get(), press.get()));
    QVERIFY(proxy.forwardEvent(KisToolProxy::CONTINUE, KisTool::Primary, move.get(), move.get()));
    QVERIFY(proxy.forwardEvent(KisToolProxy::END, KisTool::Primary, release.get(), release.get()));

    QCOMPARE(tool.snapshots.size(), 3);
    compareTabletSnapshot(tool.snapshots.at(0),
                          QStringLiteral("begin"),
                          QPointF(0.5, 0.7),
                          QPoint(100, 120),
                          QPoint(1000, 1200),
                          Qt::LeftButton,
                          Qt::LeftButton,
                          110,
                          0.25,
                          -15.0,
                          20.0,
                          0.2,
                          30.0,
                          3);
    compareTabletSnapshot(tool.snapshots.at(1),
                          QStringLiteral("continue"),
                          QPointF(0.9, 1.3),
                          QPoint(140, 180),
                          QPoint(1040, 1260),
                          Qt::NoButton,
                          Qt::LeftButton,
                          120,
                          0.5,
                          -5.0,
                          10.0,
                          0.5,
                          45.0,
                          4);
    compareTabletSnapshot(tool.snapshots.at(2),
                          QStringLiteral("end"),
                          QPointF(1.1, 1.6),
                          QPoint(160, 210),
                          QPoint(1060, 1290),
                          Qt::LeftButton,
                          Qt::NoButton,
                          130,
                          0.0,
                          12.0,
                          -8.0,
                          0.9,
                          60.0,
                          5);

    QCOMPARE(activationSpy.count(), 2);
    QCOMPARE(activationSpy.at(0).at(0).toBool(), true);
    QCOMPARE(activationSpy.at(1).at(0).toBool(), false);
}

void KisToolProxyContractTest::forwardsSingleTouchStrokeToCurrentTool()
{
    TestShapeController shapeController;
    TestToolCanvas canvas(&shapeController);
    KisToolProxy proxy(&canvas);
    KisKActionCollection actionCollection(this);
    TestCanvasController controller(&canvas, &actionCollection);
    proxy.priv()->controller = &controller;
    canvas.setToolProxy(&proxy);
    RecordingTool tool(&canvas);
    proxy.setActiveTool(&tool);
    QSignalSpy activationSpy(&proxy, &KisToolProxy::toolPrimaryActionActivated);

    const QInputDevice::Capabilities capabilities =
        QInputDevice::Capability::Position | QInputDevice::Capability::Pressure
        | QInputDevice::Capability::Rotation;
    const QPointingDevice device(QStringLiteral("R2-G11 contract touchscreen"),
                                 31,
                                 QInputDevice::DeviceType::TouchScreen,
                                 QPointingDevice::PointerType::Finger,
                                 capabilities,
                                 1,
                                 0,
                                 QStringLiteral("contract seat"));

    auto begin = touchEvent(QEvent::TouchBegin,
                            &device,
                            QEventPoint::State::Pressed,
                            QPointF(100.0, 120.0),
                            QPointF(1000.0, 1200.0),
                            0.2,
                            5.0,
                            210);
    auto update = touchEvent(QEvent::TouchUpdate,
                             &device,
                             QEventPoint::State::Updated,
                             QPointF(140.0, 180.0),
                             QPointF(1040.0, 1260.0),
                             0.6,
                             25.0,
                             220);
    auto end = touchEvent(QEvent::TouchEnd,
                          &device,
                          QEventPoint::State::Released,
                          QPointF(160.0, 210.0),
                          QPointF(1060.0, 1290.0),
                          0.0,
                          45.0,
                          230);

    QVERIFY(proxy.forwardEvent(KisToolProxy::BEGIN, KisTool::Primary, begin.get(), begin.get()));
    QVERIFY(proxy.forwardEvent(KisToolProxy::CONTINUE, KisTool::Primary, update.get(), update.get()));
    QVERIFY(proxy.forwardEvent(KisToolProxy::END, KisTool::Primary, end.get(), end.get()));

    QCOMPARE(tool.snapshots.size(), 3);
    compareTouchSnapshot(tool.snapshots.at(0),
                         QStringLiteral("begin"),
                         QPointF(0.5, 0.7),
                         QPoint(100, 120),
                         QPoint(1000, 1200),
                         210,
                         0.2,
                         5.0);
    compareTouchSnapshot(tool.snapshots.at(1),
                         QStringLiteral("continue"),
                         QPointF(0.9, 1.3),
                         QPoint(140, 180),
                         QPoint(1040, 1260),
                         220,
                         0.6,
                         25.0);
    compareTouchSnapshot(tool.snapshots.at(2),
                         QStringLiteral("end"),
                         QPointF(1.1, 1.6),
                         QPoint(160, 210),
                         QPoint(1060, 1290),
                         230,
                         0.0,
                         45.0);

    QCOMPARE(activationSpy.count(), 2);
    QCOMPARE(activationSpy.at(0).at(0).toBool(), true);
    QCOMPARE(activationSpy.at(1).at(0).toBool(), false);
}

void KisToolProxyContractTest::reportsIgnoredMousePress()
{
    TestShapeController shapeController;
    TestToolCanvas canvas(&shapeController);
    KisToolProxy proxy(&canvas);
    KisKActionCollection actionCollection(this);
    TestCanvasController controller(&canvas, &actionCollection);
    proxy.priv()->controller = &controller;
    canvas.setToolProxy(&proxy);
    RecordingTool tool(&canvas);
    tool.ignoreBegin = true;
    proxy.setActiveTool(&tool);

    auto press = mouseEvent(QEvent::MouseButtonPress,
                            QPointF(100.0, 120.0),
                            QPointF(1000.0, 1200.0),
                            Qt::LeftButton,
                            Qt::LeftButton,
                            10);

    QVERIFY(!proxy.forwardEvent(KisToolProxy::BEGIN, KisTool::Primary, press.get(), press.get()));
    QCOMPARE(tool.snapshots.size(), 1);
    QCOMPARE(tool.snapshots.constFirst().phase, QStringLiteral("begin"));
}

QTEST_MAIN(KisToolProxyContractTest)

#include "KisToolProxyContractTest.moc"
