/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "WgpuCanvasPresenter.h"
#include "../kis_canvas_performance_measurement_p.h"

#include <QMouseEvent>
#include <QTest>
#include <QWindow>
#include <QWidget>
#include <QScopeGuard>

using Krita::Canvas::WgpuCanvasPresenter;

class WgpuCanvasPresenterTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void nativeWidgetResizesAndResumes_data();
    void nativeWidgetResizesAndResumes();
    void retainsQtMouseInputRoute();
};

void WgpuCanvasPresenterTest::nativeWidgetResizesAndResumes_data()
{
    QTest::addColumn<QByteArray>("presentMode");
    QTest::addColumn<QByteArray>("frameInterval");
    QTest::newRow("fifo-unpaced") << QByteArray("fifo") << QByteArray("0");
    QTest::newRow("fifo-paced") << QByteArray("fifo") << QByteArray("16");
    QTest::newRow("immediate-unpaced") << QByteArray("immediate") << QByteArray("0");
    QTest::newRow("immediate-paced") << QByteArray("immediate") << QByteArray("16");
}

void WgpuCanvasPresenterTest::initTestCase()
{
    const auto platform = QGuiApplication::platformName();
    if (platform == QStringLiteral("offscreen") || platform == QStringLiteral("minimal")) {
        QSKIP("Native surface contracts require a windowing session (cocoa, xcb or windows)");
    }
}

void WgpuCanvasPresenterTest::nativeWidgetResizesAndResumes()
{
    QFETCH(QByteArray, presentMode);
    QFETCH(QByteArray, frameInterval);
    const auto previousMode = qgetenv("LIBREPAINT_WGPU_PRESENT_MODE");
    const auto previousInterval = qgetenv("LIBREPAINT_WGPU_FRAME_INTERVAL_MS");
    qputenv("LIBREPAINT_WGPU_PRESENT_MODE", presentMode);
    qputenv("LIBREPAINT_WGPU_FRAME_INTERVAL_MS", frameInterval);
    auto restore = qScopeGuard([&] {
        if (previousMode.isNull()) qunsetenv("LIBREPAINT_WGPU_PRESENT_MODE");
        else qputenv("LIBREPAINT_WGPU_PRESENT_MODE", previousMode);
        if (previousInterval.isNull()) qunsetenv("LIBREPAINT_WGPU_FRAME_INTERVAL_MS");
        else qputenv("LIBREPAINT_WGPU_FRAME_INTERVAL_MS", previousInterval);
    });
    QWidget host;
    host.resize(268, 156);
    host.winId();
    QWindow *window = host.windowHandle();
    window->setProperty(Krita::Canvas::Performance::enabledProperty, true);
    WgpuCanvasPresenter presenter(*window);
    QImage image(67, 39, QImage::Format_RGBA8888);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            image.setPixelColor(x, y, QColor(x * 3, y * 5, x + y, 255));
        }
    }
    image.setDevicePixelRatio(2.0);
    QVERIFY2(presenter.setImage(image, image.rect()), qPrintable(presenter.error()));
    host.resize(268, 156);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(host.windowHandle()));
    QTRY_VERIFY_WITH_TIMEOUT(presenter.submittedFrames() > 0, 5000);
    const auto metrics = window->property(Krita::Canvas::Performance::samplesProperty).toMap();
    QVERIFY(metrics.value(QStringLiteral("surface_acquire")).toMap().value(QStringLiteral("count")).toLongLong() > 0);
    QImage expected = image;
    expected.setDevicePixelRatio(1.0);
    QCOMPARE(presenter.readback(), expected.scaled(window->size() * window->devicePixelRatio()));
    const quint64 initial = presenter.submittedFrames();
    const quint64 uploaded = presenter.uploadedBytes();
    host.resize(335, 195);
    QTRY_VERIFY_WITH_TIMEOUT(presenter.submittedFrames() > initial, 5000);
    QCOMPARE(presenter.readback(), expected.scaled(window->size() * window->devicePixelRatio()));
    QCOMPARE(presenter.uploadedBytes(), uploaded);
    host.hide();
    QTest::qWait(50);
    const quint64 hidden = presenter.submittedFrames();
    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(presenter.submittedFrames() > hidden, 5000);
    QCOMPARE(presenter.readback(), expected.scaled(window->size() * window->devicePixelRatio()));
    QVERIFY2(presenter.error().isEmpty(), qPrintable(presenter.error()));

    // Dirty rectangles use physical image pixels, even for a high-DPI snapshot.
    image.setDevicePixelRatio(2.0);
    const QRect dirty(5, 8, 13, 7);
    for (int y = dirty.top(); y <= dirty.bottom(); ++y) {
        for (int x = dirty.left(); x <= dirty.right(); ++x) image.setPixelColor(x, y, Qt::red);
    }
    QVERIFY(presenter.setImage(image, dirty));
    const QRect secondDirty(35, 10, 9, 5);
    for (int y = secondDirty.top(); y <= secondDirty.bottom(); ++y) {
        for (int x = secondDirty.left(); x <= secondDirty.right(); ++x) image.setPixelColor(x, y, Qt::blue);
    }
    QVERIFY(presenter.setImage(image, secondDirty));
    const quint64 changed = presenter.submittedFrames();
    QTRY_VERIFY_WITH_TIMEOUT(presenter.submittedFrames() > changed, 5000);
    expected = image;
    expected.setDevicePixelRatio(1.0);
    QCOMPARE(presenter.readback(), expected.scaled(window->size() * window->devicePixelRatio()));
    host.hide();
    window->destroy();
    const quint64 destroyed = presenter.submittedFrames();
    host.show();
    QTRY_VERIFY_WITH_TIMEOUT(presenter.submittedFrames() > destroyed, 5000);
    QCOMPARE(presenter.readback(), expected.scaled(window->size() * window->devicePixelRatio()));
}

void WgpuCanvasPresenterTest::retainsQtMouseInputRoute()
{
    class Receiver : public QWidget {
    public:
        QList<QEvent::Type> events;
        QList<QPointF> points;
        bool event(QEvent *event) override {
            if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonRelease) {
                events.append(event->type());
                points.append(static_cast<QMouseEvent *>(event)->position());
                event->accept();
                return true;
            }
            return QWidget::event(event);
        }
    } receiver;
    receiver.setMouseTracking(true);
    receiver.winId();
    WgpuCanvasPresenter presenter(*receiver.windowHandle());
    receiver.show();
    QVERIFY(QTest::qWaitForWindowExposed(receiver.windowHandle()));
    const QPoint position(13, 21);
    const QPoint moved(23, 31);
    receiver.events.clear();
    receiver.points.clear();
    for (auto type : {QEvent::MouseButtonPress, QEvent::MouseMove, QEvent::MouseButtonRelease}) {
        const QPoint local = type == QEvent::MouseButtonPress ? position : moved;
        QMouseEvent event(type, local, receiver.mapToGlobal(local),
                          type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                          type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(receiver.windowHandle(), &event);
    }
    QCOMPARE(receiver.events, QList<QEvent::Type>({QEvent::MouseButtonPress, QEvent::MouseMove, QEvent::MouseButtonRelease}));
    QCOMPARE(receiver.points, QList<QPointF>({position, moved, moved}));
}

QTEST_MAIN(WgpuCanvasPresenterTest)
#include "WgpuCanvasPresenterTest.moc"
