/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisPopupButton.h>

#include <QApplication>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPointer>
#include <QProxyStyle>
#include <QScreen>
#include <QSignalSpy>
#include <QStyleFactory>
#include <QTest>

class PopupButtonRecordingStyle : public QProxyStyle
{
public:
    PopupButtonRecordingStyle()
        : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion")))
    {
    }

    void reset() const
    {
        arrowDraws = 0;
    }

    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = nullptr) const override
    {
        if (element == QStyle::PE_IndicatorArrowDown) {
            ++arrowDraws;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    mutable int arrowDraws = 0;
};

QWidget *installPopup(KisPopupButton &button, const QSize &size = QSize(80, 40))
{
    auto *popup = new QWidget;
    popup->setFixedSize(size);
    button.setPopupWidget(popup);
    return popup;
}

class KisPopupButtonContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void constructorUsesParentAndDestroysOwnedPopup();
    void popupVisibilityAndSignalFollowCommands();
    void popupWidthControlsFrame();
    void detachedModeChangesFrameWindowType();
    void adjustPositionKeepsFrameOnScreen();
    void arrowVisibilityControlsIndicatorPainting();
};

void KisPopupButtonContractTest::initTestCase()
{
    QApplication::setStyle(new PopupButtonRecordingStyle);
}

void KisPopupButtonContractTest::constructorUsesParentAndDestroysOwnedPopup()
{
    QWidget parent;
    QPointer<KisPopupButton> button = new KisPopupButton(&parent);
    QPointer<QWidget> popup = installPopup(*button);

    QCOMPARE(button->parentWidget(), &parent);
    QCOMPARE(button->objectName(), QStringLiteral("KisPopupButton"));
    QVERIFY(popup->parentWidget());

    delete button.data();
    QVERIFY(button.isNull());
    QVERIFY(popup.isNull());
}

void KisPopupButtonContractTest::popupVisibilityAndSignalFollowCommands()
{
    KisPopupButton button(nullptr);
    installPopup(button);
    QSignalSpy shownSpy(&button, &KisPopupButton::sigPopupWidgetShown);

    QVERIFY(!button.isPopupWidgetVisible());
    button.showPopupWidget();
    QVERIFY(button.isPopupWidgetVisible());
    QCOMPARE(shownSpy.size(), 1);

    button.hidePopupWidget();
    QVERIFY(!button.isPopupWidgetVisible());

    button.setPopupWidgetVisible(true);
    QVERIFY(button.isPopupWidgetVisible());
    QCOMPARE(shownSpy.size(), 2);

    button.setPopupWidgetVisible(false);
    QVERIFY(!button.isPopupWidgetVisible());

    QVERIFY(QMetaObject::invokeMethod(&button, "released", Qt::DirectConnection));
    QVERIFY(button.isPopupWidgetVisible());
    QCOMPARE(shownSpy.size(), 3);
    button.hidePopupWidget();
}

void KisPopupButtonContractTest::popupWidthControlsFrame()
{
    KisPopupButton button(nullptr);
    QWidget *popup = installPopup(button);
    QWidget *frame = popup->parentWidget();
    QVERIFY(frame);

    button.setPopupWidgetWidth(137);

    QCOMPARE(frame->width(), 137);
}

void KisPopupButtonContractTest::detachedModeChangesFrameWindowType()
{
    KisPopupButton button(nullptr);
    QWidget *popup = installPopup(button);
    QWidget *frame = popup->parentWidget();
    QVERIFY(frame);
    QCOMPARE(frame->windowType(), Qt::Popup);

    button.setPopupWidgetDetached(true);
    QCOMPARE(frame->windowType(), Qt::Dialog);

    button.setPopupWidgetDetached(false);
    QCOMPARE(frame->windowType(), Qt::Popup);
}

void KisPopupButtonContractTest::adjustPositionKeepsFrameOnScreen()
{
    KisPopupButton button(nullptr);
    QWidget *popup = installPopup(button, QSize(120, 60));
    QWidget *frame = popup->parentWidget();
    QVERIFY(frame);
    button.setPopupWidgetVisible(true);

    button.adjustPosition();

    QScreen *screen = QGuiApplication::screenAt(frame->geometry().center());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    QVERIFY(screen);
    QVERIFY(screen->availableGeometry().contains(frame->geometry()));
    button.hidePopupWidget();
}

void KisPopupButtonContractTest::arrowVisibilityControlsIndicatorPainting()
{
    auto *style = dynamic_cast<PopupButtonRecordingStyle *>(QApplication::style());
    QVERIFY(style);
    KisPopupButton button(nullptr);
    button.resize(40, 40);
    QImage image(button.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    style->reset();
    button.render(&image);
    QCOMPARE(style->arrowDraws, 1);

    button.setArrowVisible(false);
    style->reset();
    image.fill(Qt::transparent);
    button.render(&image);
    QCOMPARE(style->arrowDraws, 0);

    button.setArrowVisible(true);
    style->reset();
    image.fill(Qt::transparent);
    button.render(&image);
    QCOMPARE(style->arrowDraws, 1);
}

QTEST_MAIN(KisPopupButtonContractTest)

#include "KisPopupButtonContractTest.moc"
