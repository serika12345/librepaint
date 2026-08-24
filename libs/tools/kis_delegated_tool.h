/*
 *  SPDX-FileCopyrightText: 2013 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __KIS_DELEGATED_TOOL_H
#define __KIS_DELEGATED_TOOL_H

#include <KoPointerEvent.h>
#include <KoShape.h>

#include <QLayout>
#include <QPointer>

#include <KisOptionCollectionWidget.h>
#include <KisToolCanvas.h>
#include <kis_delegated_tool_policies.h>
#include <kis_tool.h>
#include <kritatools_export.h>

#define PRESS_CONDITION_OM(_event, _mode, _button, _modifier)           \
    (this->mode() == (_mode) && (_event)->button() == (_button) &&      \
     ((_event)->modifiers() & (_modifier) ||                            \
      (_event)->modifiers() == Qt::NoModifier))

template <class BaseClass, class DelegateTool, class ActivationPolicy = NoopActivationPolicy>
class KRITATOOLS_EXPORT KisDelegatedTool : public BaseClass
{
public:
    KisDelegatedTool(KoCanvasBase *canvas,
                     const QCursor &cursor,
                     DelegateTool *delegateTool)
        : BaseClass(canvas, cursor)
        , m_localTool(delegateTool)
    {
    }

    DelegateTool *localTool() const
    {
        return m_localTool.data();
    }

    void activate(const QSet<KoShape *> &shapes) override
    {
        BaseClass::activate(shapes);
        m_localTool->activate(shapes);
        ActivationPolicy::onActivate(BaseClass::canvas());

        toolCanvas()->attachPriorityEventFilterForTool(this);
    }

    void deactivate() override
    {
        m_localTool->deactivate();
        BaseClass::deactivate();

        toolCanvas()->detachPriorityEventFilterForTool(this);
    }

    void mousePressEvent(KoPointerEvent *event) override
    {
        if (PRESS_CONDITION_OM(event,
                               KisTool::HOVER_MODE,
                               Qt::LeftButton,
                               Qt::ShiftModifier | Qt::ControlModifier |
                                   Qt::AltModifier)) {
            this->setMode(KisTool::PAINT_MODE);

            Q_ASSERT(m_localTool);
            m_localTool->mousePressEvent(event);
        } else {
            BaseClass::mousePressEvent(event);
        }
    }

    void mouseDoubleClickEvent(KoPointerEvent *event) override
    {
        if (PRESS_CONDITION_OM(event,
                               KisTool::HOVER_MODE,
                               Qt::LeftButton,
                               Qt::ShiftModifier | Qt::ControlModifier |
                                   Qt::AltModifier)) {
            Q_ASSERT(m_localTool);
            m_localTool->mouseDoubleClickEvent(event);
        } else {
            BaseClass::mouseDoubleClickEvent(event);
        }
    }

    void mouseMoveEvent(KoPointerEvent *event) override
    {
        Q_ASSERT(m_localTool);
        m_localTool->mouseMoveEvent(event);

        BaseClass::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(KoPointerEvent *event) override
    {
        if (this->mode() == KisTool::PAINT_MODE &&
            event->button() == Qt::LeftButton) {
            this->setMode(KisTool::HOVER_MODE);

            Q_ASSERT(m_localTool);
            m_localTool->mouseReleaseEvent(event);
        } else {
            BaseClass::mouseReleaseEvent(event);
        }
    }

    void paint(QPainter &painter, const KoViewConverter &converter) override
    {
        Q_ASSERT(m_localTool);
        m_localTool->paint(painter, converter);
    }

    QList<QPointer<QWidget>> createOptionWidgets() override
    {
        QList<QPointer<QWidget>> baseWidgetList = BaseClass::createOptionWidgets();
        QList<QPointer<QWidget>> localWidgetList =
            m_localTool->createOptionWidgets();

        KisOptionCollectionWidget *baseOptionsWidget =
            baseWidgetList.isEmpty()
            ? nullptr
            : dynamic_cast<KisOptionCollectionWidget *>(
                  baseWidgetList.first().data());

        if (baseOptionsWidget) {
            for (int i = 0; i < localWidgetList.size(); ++i) {
                QWidget *widget = localWidgetList[i];
                KisOptionCollectionWidgetWithHeader *section =
                    new KisOptionCollectionWidgetWithHeader(
                        widget->windowTitle());
                const QString sectionName =
                    QStringLiteral("section") + QString::number(i);
                section->appendWidget(sectionName + QStringLiteral("Widget"),
                                      widget);
                baseOptionsWidget->appendWidget(sectionName, section);
            }
        } else {
            baseWidgetList.append(localWidgetList);
        }
        return baseWidgetList;
    }

protected:
    QScopedPointer<DelegateTool> m_localTool;

private:
    KisToolCanvas *toolCanvas() const
    {
        KisToolCanvas *result =
            dynamic_cast<KisToolCanvas *>(BaseClass::canvas());
        Q_ASSERT(result);
        return result;
    }
};

#endif /* __KIS_DELEGATED_TOOL_H */
