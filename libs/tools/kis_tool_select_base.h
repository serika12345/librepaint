/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2015 Michael Abrahams <miabraha@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KIS_TOOL_SELECT_BASE_H
#define KIS_TOOL_SELECT_BASE_H

#include <KoCanvasBase.h>
#include <KoPointerEvent.h>

#include <QKeyEvent>
#include <QPainterPath>
#include <QTimer>

#include <KisToolCanvas.h>
#include <kis_coordinates_converter.h>
#include <kis_assert.h>
#include <kis_image.h>
#include <kis_selection.h>
#include <kis_selection_modifier_mapping.h>
#include <kis_tool.h>
#include <kritatools_export.h>
#include <strokes/move_stroke_strategy.h>

/**
 * Selection-tool interaction shared by concrete pixel and shape tools.
 *
 * Presentation and persisted option widgets are supplied by the UI-owned
 * subclass through configuredSelectionAction().
 */
template <class BaseClass>
class KRITATOOLS_EXPORT KisToolSelectBase : public BaseClass
{
public:
    explicit KisToolSelectBase(KoCanvasBase *canvas)
        : BaseClass(canvas)
    {
    }

    KisToolSelectBase(KoCanvasBase *canvas, const QCursor &cursor)
        : BaseClass(canvas, cursor)
    {
    }

    KisToolSelectBase(KoCanvasBase *canvas,
                      const QCursor &cursor,
                      KoToolBase *delegateTool)
        : BaseClass(canvas, cursor, delegateTool)
    {
    }

    SelectionAction selectionAction() const
    {
        return alternateSelectionAction() == SELECTION_DEFAULT
            ? configuredSelectionAction()
            : alternateSelectionAction();
    }

    SelectionAction alternateSelectionAction() const
    {
        return m_selectionActionAlternate;
    }

    virtual void setAlternateSelectionAction(SelectionAction action)
    {
        m_selectionActionAlternate = action;
    }

    void activateAlternateAction(KisTool::AlternateAction action) override
    {
        Q_UNUSED(action);
        BaseClass::activatePrimaryAction();
    }

    void deactivateAlternateAction(KisTool::AlternateAction action) override
    {
        Q_UNUSED(action);
        BaseClass::deactivatePrimaryAction();
    }

    void beginAlternateAction(KoPointerEvent *event,
                              KisTool::AlternateAction action) override
    {
        Q_UNUSED(action);
        beginPrimaryAction(event);
    }

    void continueAlternateAction(KoPointerEvent *event,
                                 KisTool::AlternateAction action) override
    {
        Q_UNUSED(action);
        continuePrimaryAction(event);
    }

    void endAlternateAction(KoPointerEvent *event,
                            KisTool::AlternateAction action) override
    {
        Q_UNUSED(action);
        endPrimaryAction(event);
    }

    KisNodeSP locateSelectionMaskUnderCursor(
        const QPointF &pos,
        Qt::KeyboardModifiers modifiers) const
    {
        if (modifiers != Qt::NoModifier) {
            return KisNodeSP();
        }

        KisToolCanvas *canvasInterface = toolCanvas();
        KisSelectionSP selection = canvasInterface->currentSelectionForTool();
        if (selection && selection->outlineCacheValid()) {
            const qreal handleRadius =
                qreal(this->handleRadius()) /
                canvasInterface->coordinatesConverter()->effectiveZoom();
            QPainterPath samplePath;
            samplePath.addEllipse(pos, handleRadius, handleRadius);

            const QPainterPath selectionPath = selection->outlineCache();
            if (selectionPath.intersects(samplePath) &&
                !selectionPath.contains(samplePath)) {
                KisNodeSP parent = selection->parentNode();
                if (parent && parent->isEditable()) {
                    return parent;
                }
            }
        }

        return KisNodeSP();
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        const Qt::Key key = normalizedSelectionModifierKey(event);
        m_currentModifiers = Qt::NoModifier;
        if (key == Qt::Key_Control ||
            event->modifiers().testFlag(Qt::ControlModifier)) {
            m_currentModifiers.setFlag(Qt::ControlModifier);
        }
        if (key == Qt::Key_Shift ||
            event->modifiers().testFlag(Qt::ShiftModifier)) {
            m_currentModifiers.setFlag(Qt::ShiftModifier);
        }
        if (key == Qt::Key_Alt ||
            event->modifiers().testFlag(Qt::AltModifier)) {
            m_currentModifiers.setFlag(Qt::AltModifier);
        }

        if (isSelecting()) {
            BaseClass::keyPressEvent(event);
            return;
        }
        if (isMovingSelection()) {
            return;
        }

        setAlternateSelectionAction(mappedSelectionAction());
        this->resetCursorStyle();
    }

    void keyReleaseEvent(QKeyEvent *event) override
    {
        const Qt::Key key = normalizedSelectionModifierKey(event);
        m_currentModifiers =
            Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier;
        if (key == Qt::Key_Control ||
            !event->modifiers().testFlag(Qt::ControlModifier)) {
            m_currentModifiers.setFlag(Qt::ControlModifier, false);
        }
        if (key == Qt::Key_Shift ||
            !event->modifiers().testFlag(Qt::ShiftModifier)) {
            m_currentModifiers.setFlag(Qt::ShiftModifier, false);
        }
        if (key == Qt::Key_Alt ||
            !event->modifiers().testFlag(Qt::AltModifier)) {
            m_currentModifiers.setFlag(Qt::AltModifier, false);
        }

        if (isSelecting()) {
            BaseClass::keyReleaseEvent(event);
            return;
        }
        if (isMovingSelection()) {
            return;
        }

        setAlternateSelectionAction(mappedSelectionAction());
        if (m_currentModifiers == Qt::NoModifier) {
            updateCursorForCurrentPosition();
        } else {
            this->resetCursorStyle();
        }
    }

    void mouseMoveEvent(KoPointerEvent *event) override
    {
        m_currentPos = this->convertToPixelCoord(event->point);

        if (isSelecting()) {
            BaseClass::mouseMoveEvent(event);
            return;
        }
        if (isMovingSelection()) {
            return;
        }

        KisNodeSP selectionMask =
            locateSelectionMaskUnderCursor(m_currentPos, event->modifiers());
        if (selectionMask) {
            this->useCursor(toolCanvas()->moveSelectionCursorForTool());
        } else {
            setAlternateSelectionAction(mappedSelectionAction());
            this->resetCursorStyle();
        }
    }

    void beginPrimaryAction(KoPointerEvent *event) override
    {
        if (isSelecting()) {
            BaseClass::beginPrimaryAction(event);
            return;
        }
        if (isMovingSelection()) {
            return;
        }

        const QPointF pos = this->convertToPixelCoord(event->point);
        KisNodeSP selectionMask =
            locateSelectionMaskUnderCursor(pos, event->modifiers());
        if (selectionMask && beginMoveSelectionInteraction()) {
            KisStrokeStrategy *strategy = new MoveStrokeStrategy(
                {selectionMask}, this->image().data(), this->image().data());
            m_moveStrokeId = this->image()->startStroke(strategy);
            m_dragStartPos = pos;
            m_didMove = true;
            return;
        }

        m_didMove = false;
        BaseClass::beginPrimaryAction(event);
    }

    void continuePrimaryAction(KoPointerEvent *event) override
    {
        if (isMovingSelection()) {
            const QPointF pos = this->convertToPixelCoord(event->point);
            const QPoint offset((pos - m_dragStartPos).toPoint());
            this->image()->addJob(m_moveStrokeId,
                                  new MoveStrokeStrategy::Data(offset));
            return;
        }

        BaseClass::continuePrimaryAction(event);
    }

    void endPrimaryAction(KoPointerEvent *event) override
    {
        if (isMovingSelection()) {
            this->image()->endStroke(m_moveStrokeId);
            m_moveStrokeId.clear();
            endMoveSelectionInteraction();
            return;
        }

        BaseClass::endPrimaryAction(event);
    }

    bool selectionDidMove() const
    {
        return m_didMove;
    }

    KisPopupWidgetInterface *popupWidget() override
    {
        return isSelecting() ? BaseClass::popupWidget() : nullptr;
    }

    bool beginMoveSelectionInteraction()
    {
        if (m_currentInteraction != Interaction_None) {
            return false;
        }
        m_currentInteraction = Interaction_MoveSelection;
        return true;
    }

    bool endMoveSelectionInteraction()
    {
        if (!isMovingSelection()) {
            return false;
        }
        m_currentInteraction = Interaction_None;
        updateCursorDelayed();
        return true;
    }

    bool beginSelectInteraction()
    {
        if (m_currentInteraction != Interaction_None) {
            return false;
        }
        m_currentInteraction = Interaction_Select;
        return true;
    }

    bool endSelectInteraction()
    {
        if (!isSelecting()) {
            return false;
        }
        m_currentInteraction = Interaction_None;
        updateCursorDelayed();
        return true;
    }

    bool isMovingSelection() const
    {
        return m_currentInteraction == Interaction_MoveSelection;
    }

    bool isSelecting() const
    {
        return m_currentInteraction == Interaction_Select;
    }

protected:
    using BaseClass::canvas;

    KisToolCanvas *toolCanvas() const
    {
        KisToolCanvas *result =
            dynamic_cast<KisToolCanvas *>(this->canvas());
        KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(result, nullptr);
        return result;
    }

    virtual SelectionAction configuredSelectionAction() const = 0;

private:
    enum Interaction {
        Interaction_None,
        Interaction_Select,
        Interaction_MoveSelection
    };

    SelectionAction mappedSelectionAction() const
    {
        return mapSelectionToolModifiers(
            m_currentModifiers,
            toolCanvas()->selectionModifierMappingSwapsCtrlAndAltForTool());
    }

    void updateCursorForCurrentPosition()
    {
        KisNodeSP selectionMask = locateSelectionMaskUnderCursor(
            m_currentPos,
            m_currentModifiers);
        if (selectionMask) {
            this->useCursor(toolCanvas()->moveSelectionCursorForTool());
        } else {
            this->resetCursorStyle();
        }
    }

    void updateCursorDelayed()
    {
        setAlternateSelectionAction(mappedSelectionAction());
        QTimer::singleShot(100,
                           Qt::CoarseTimer,
                           this,
                           [this]() { updateCursorForCurrentPosition(); });
    }

    Interaction m_currentInteraction {Interaction_None};
    Qt::KeyboardModifiers m_currentModifiers;
    QPointF m_dragStartPos;
    QPointF m_currentPos;
    KisStrokeId m_moveStrokeId;
    SelectionAction m_selectionActionAlternate {SELECTION_DEFAULT};
    bool m_didMove {false};
};

#endif
