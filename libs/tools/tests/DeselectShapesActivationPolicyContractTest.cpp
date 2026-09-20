/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_delegated_tool_policies.h"

#include <KoSelection.h>
#include <KoShapeManager.h>
#include <MockShapes.h>

#include <QCoreApplication>
#include <QEvent>
#include <QTest>
#include <QWidget>

class PaintTrackingWidget : public QWidget
{
public:
    void clearPaintEvents()
    {
        m_paintEvents = 0;
    }

    int paintEvents() const
    {
        return m_paintEvents;
    }

protected:
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::Paint) {
            ++m_paintEvents;
        }
        return QWidget::event(event);
    }

private:
    int m_paintEvents = 0;
};

class PolicyCanvas : public MockCanvas
{
public:
    QWidget *canvasWidget() override
    {
        return &m_widget;
    }

    const QWidget *canvasWidget() const override
    {
        return &m_widget;
    }

    PaintTrackingWidget *trackingWidget()
    {
        return &m_widget;
    }

private:
    PaintTrackingWidget m_widget;
};

class DeselectShapesActivationPolicyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void activationClearsShapeSelectionAndSchedulesCanvasUpdate();
};

void DeselectShapesActivationPolicyContractTest::activationClearsShapeSelectionAndSchedulesCanvasUpdate()
{
    PolicyCanvas canvas;
    MockShape shape;
    KoSelection *selection = canvas.shapeManager()->selection();

    selection->select(&shape);
    QCOMPARE(selection->count(), 1);

    PaintTrackingWidget *widget = canvas.trackingWidget();
    widget->resize(32, 32);
    widget->show();
    QCoreApplication::processEvents();
    widget->clearPaintEvents();

    DeselectShapesActivationPolicy::onActivate(&canvas);

    QCOMPARE(selection->count(), 0);
    QTRY_VERIFY(widget->paintEvents() > 0);
}

QTEST_MAIN(DeselectShapesActivationPolicyContractTest)

#include "DeselectShapesActivationPolicyContractTest.moc"
