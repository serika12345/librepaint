/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisVulkanFrameWindow.h"
#include <QApplication>
#include <QMouseEvent>
#include <QMutex>
#include <QSignalSpy>
#include <QTest>
#include <QVulkanInstance>
#include <QVBoxLayout>
#include <memory>

class KisVulkanPresentationContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void framesSurviveResizeAndReopen()
    {
        QStringList diagnostics;
        QMutex diagnosticsMutex;
        QVulkanInstance instance;
        instance.setApiVersion(QVersionNumber(1, 1, 0));
        instance.setExtensions({"VK_KHR_get_physical_device_properties2"});
        QVERIFY(instance.supportedLayers().contains("VK_LAYER_KHRONOS_validation"));
        instance.setLayers({"VK_LAYER_KHRONOS_validation"});
        instance.installDebugOutputFilter([&](auto severity, auto, const void *data) {
            const auto *message = static_cast<const VkDebugUtilsMessengerCallbackDataEXT *>(data);
            if (severity & (QVulkanInstance::WarningSeverity | QVulkanInstance::ErrorSeverity)) {
                const QMutexLocker lock(&diagnosticsMutex);
                diagnostics.append(QString::fromUtf8(message->pMessage));
            }
            return false;
        });
        QVERIFY2(instance.create(), qPrintable(QString::number(instance.errorCode())));
        {
            QWidget host;
            auto *window = new KisVulkanFrameWindow(&instance);
            auto *layout = new QVBoxLayout(&host);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->addWidget(QWidget::createWindowContainer(window, &host));
            host.resize(256, 192);
            QSignalSpy submitted(window, &KisVulkanFrameWindow::frameSubmitted);
            QSignalSpy failed(window, &KisVulkanFrameWindow::renderFailed);
            host.show();
            QVERIFY(QTest::qWaitForWindowExposed(&host));
            QTRY_VERIFY(window->isValid());
            QVERIFY(window->supportsGrab());
            for (int iteration = 0; iteration < 20; ++iteration) {
                qInfo() << "Image comparison iteration" << iteration;
                const QSize requestedSize(256 + iteration * 2, 192 + iteration * 2);
                host.resize(requestedSize);
                QTRY_COMPARE(window->size(), requestedSize);
                window->requestUpdate();
                QTRY_COMPARE(window->swapChainImageSize(), requestedSize * window->devicePixelRatio());
                QImage expected(window->swapChainImageSize(), QImage::Format_RGBA8888);
                for (int y = 0; y < expected.height(); ++y) {
                    for (int x = 0; x < expected.width(); ++x) {
                        expected.setPixelColor(x, y, QColor((x * 7 + iteration) % 256, (y * 9) % 256, 53, 255));
                    }
                }
                const int count = submitted.count();
                window->setFrame(expected);
                QTRY_VERIFY(submitted.count() > count);
                window->setFrame(QImage());
                const QImage actual = window->grab();
                QCOMPARE(actual.size(), expected.size());
                for (int y = 0; y < expected.height(); ++y) {
                    for (int x = 0; x < expected.width(); ++x) {
                        const QColor a = actual.pixelColor(x, y), e = expected.pixelColor(x, y);
                        QVERIFY(qAbs(a.red() - e.red()) <= 1);
                        QVERIFY(qAbs(a.green() - e.green()) <= 1);
                        QVERIFY(qAbs(a.blue() - e.blue()) <= 1);
                        QCOMPARE(a.alpha(), e.alpha());
                    }
                }
                host.hide();
                QTest::qWait(10);
                host.show();
                QVERIFY(QTest::qWaitForWindowExposed(&host));
            }
            QVERIFY2(failed.isEmpty(), failed.isEmpty() ? "" : qPrintable(failed.first().first().toString()));
        }
        instance.destroy();
        const QMutexLocker lock(&diagnosticsMutex);
        QVERIFY2(diagnostics.isEmpty(), qPrintable(diagnostics.join('\n')));
    }

    void inputIsDeliveredOnce()
    {
        struct Receiver : QWidget {
            QList<QPointF> points;
            bool event(QEvent *event) override {
                if (event->type() == QEvent::MouseButtonPress) {
                    points.append(static_cast<QMouseEvent *>(event)->position());
                    return true;
                }
                return QWidget::event(event);
            }
        } receiver;
        QVulkanInstance instance;
        instance.setApiVersion(QVersionNumber(1, 1, 0));
        instance.setExtensions({"VK_KHR_get_physical_device_properties2"});
        KisVulkanFrameWindow window(&instance, &receiver);
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(11, 17), QPointF(11, 17),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&window, &press);
        QCOMPARE(receiver.points, QList<QPointF>{QPointF(11, 17)});
    }
};

QTEST_MAIN(KisVulkanPresentationContractTest)
#include "KisVulkanPresentationContractTest.moc"
