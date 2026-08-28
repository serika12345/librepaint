/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisPopupWidgetInterface.h>

#include <QPointer>
#include <QSignalSpy>
#include <QTest>

class RecordingPopupWidget final : public QObject, public KisPopupWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(KisPopupWidgetInterface)

public:
    ~RecordingPopupWidget() override
    {
        if (destroyed) {
            *destroyed = true;
        }
    }

    void popup(const QPoint &position) override
    {
        lastPosition = position;
        visible = true;
    }

    bool onScreen() override
    {
        return visible;
    }

    void dismiss() override
    {
        visible = false;
    }

    bool *destroyed {nullptr};
    QPoint lastPosition;
    bool visible {false};

Q_SIGNALS:
    void finished() override;
};

class KisPopupWidgetInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void transitionsVisibilityAtRequestedPosition();
    void emitsCompletionThroughInterface();
    void supportsQtInterfaceAndPolymorphicLifetime();
};

void KisPopupWidgetInterfaceContractTest::transitionsVisibilityAtRequestedPosition()
{
    RecordingPopupWidget widget;
    KisPopupWidgetInterface *interface = &widget;

    QVERIFY(!interface->onScreen());

    interface->popup(QPoint(23, 47));

    QVERIFY(interface->onScreen());
    QCOMPARE(widget.lastPosition, QPoint(23, 47));

    interface->dismiss();

    QVERIFY(!interface->onScreen());
}

void KisPopupWidgetInterfaceContractTest::emitsCompletionThroughInterface()
{
    RecordingPopupWidget widget;
    QSignalSpy finishedSpy(&widget, &RecordingPopupWidget::finished);
    KisPopupWidgetInterface *interface = &widget;

    interface->finished();

    QCOMPARE(finishedSpy.count(), 1);
}

void KisPopupWidgetInterfaceContractTest::supportsQtInterfaceAndPolymorphicLifetime()
{
    bool destroyed = false;
    auto *widget = new RecordingPopupWidget;
    widget->destroyed = &destroyed;
    QPointer<RecordingPopupWidget> guard(widget);
    KisPopupWidgetInterface *interface = qobject_cast<KisPopupWidgetInterface *>(widget);

    QCOMPARE(interface, static_cast<KisPopupWidgetInterface *>(widget));

    delete interface;

    QVERIFY(destroyed);
    QVERIFY(guard.isNull());
}

QTEST_GUILESS_MAIN(KisPopupWidgetInterfaceContractTest)

#include "KisPopupWidgetInterfaceContractTest.moc"
