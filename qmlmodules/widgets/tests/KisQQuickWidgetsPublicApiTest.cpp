/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QColorSpace>
#endif
#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QTest>

#include <KisQQuickPopupWidget.h>
#include <KisQQuickWidget.h>

class KisQQuickWidgetsPublicApiTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void widgetConfiguresQuickRendering();
    void minimumDimensionsFollowRootObject();
    void widgetParentedLifetimeIsSafe();
    void popupLoadsRootAndManagesMargins();
    void popupParentedLifetimeIsSafe();
};

void KisQQuickWidgetsPublicApiTest::widgetConfiguresQuickRendering()
{
    QWidget parent;
    KisQQuickWidget widget(&parent);

    QCOMPARE(widget.parentWidget(), &parent);
    QCOMPARE(widget.resizeMode(), QQuickWidget::SizeRootObjectToView);
    QCOMPARE(widget.engine()->rootContext()->contextProperty(QStringLiteral("mainWindow")).value<QObject *>(), &parent);
    QVERIFY(widget.engine()->rootContext()->contextObject());
    QCOMPARE(widget.format().redBufferSize(), 8);
    QCOMPARE(widget.format().greenBufferSize(), 8);
    QCOMPARE(widget.format().blueBufferSize(), 8);
    QCOMPARE(widget.format().alphaBufferSize(), 8);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QCOMPARE(widget.format().colorSpace(), QColorSpace(QColorSpace::SRgb));
#else
    QCOMPARE(widget.format().colorSpace(), QSurfaceFormat::sRGBColorSpace);
#endif
}

void KisQQuickWidgetsPublicApiTest::minimumDimensionsFollowRootObject()
{
    KisQQuickWidget widget;

    widget.connectMinimumWidthToRootObject();
    widget.connectMinimumHeightToRootObject();
    QCOMPARE(widget.minimumSize(), QSize(0, 0));

    QQmlComponent component(widget.engine());
    component.setData(R"(
        import QtQuick
        Item {
            implicitWidth: 64
            implicitHeight: 48
        }
    )",
                      QUrl(QStringLiteral("inline:/MinimumSizeRoot.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(component.isReady() || component.isError(), 1000);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QObject *createdObject = component.create();
    QVERIFY2(createdObject, qPrintable(component.errorString()));
    widget.setContent(component.url(), &component, createdObject);
    QCOMPARE(widget.status(), QQuickWidget::Ready);

    widget.connectMinimumWidthToRootObject();
    widget.connectMinimumHeightToRootObject();
    QCOMPARE(widget.minimumSize(), QSize(64, 48));

    QQuickItem *root = widget.rootObject();
    QVERIFY(root);
    root->setImplicitWidth(96);
    root->setImplicitHeight(72);
    QCOMPARE(widget.minimumSize(), QSize(96, 72));
}

void KisQQuickWidgetsPublicApiTest::widgetParentedLifetimeIsSafe()
{
    QWidget parent;
    auto *widget = new KisQQuickWidget(&parent);
    QPointer<KisQQuickWidget> guard(widget);
    QPointer<QObject> localizedContext(widget->engine()->rootContext()->contextObject());

    QVERIFY(localizedContext);
    QCOMPARE(localizedContext->parent(), widget);

    delete widget;

    QVERIFY(guard.isNull());
    QVERIFY(localizedContext.isNull());
}

void KisQQuickWidgetsPublicApiTest::popupLoadsRootAndManagesMargins()
{
    QWidget parent;
    KisQQuickPopupWidget popup(&parent);

    QCOMPARE(popup.parentWidget(), &parent);
    QVERIFY(popup.windowFlags().testFlag(Qt::Popup));
    QTRY_VERIFY_WITH_TIMEOUT(popup.rootObject(), 1000);
    QCOMPARE(popup.layoutContentMargins(), QMargins());

    const QMargins margins(1, 2, 3, 4);
    popup.setLayoutMargins(margins);
    QCOMPARE(popup.layoutContentMargins(), margins);
}

void KisQQuickWidgetsPublicApiTest::popupParentedLifetimeIsSafe()
{
    QWidget parent;
    auto *popup = new KisQQuickPopupWidget(&parent);
    QPointer<KisQQuickPopupWidget> guard(popup);

    delete popup;

    QVERIFY(guard.isNull());
}

QTEST_MAIN(KisQQuickWidgetsPublicApiTest)

#include "KisQQuickWidgetsPublicApiTest.moc"
