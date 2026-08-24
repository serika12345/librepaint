/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2015 Michael Abrahams <miabraha@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KIS_TOOL_SELECT_UI_BASE_H
#define KIS_TOOL_SELECT_UI_BASE_H

#include <kis_assert.h>
#include <kis_selection_options.h>
#include <kis_selection_tool_config_widget_helper.h>
#include <kis_selection_tool_helper.h>
#include <kis_signal_auto_connection.h>
#include <kis_tool_select_base.h>
#include <kritaui_export.h>

/**
 * Selection-tool option presentation and action wiring.
 *
 * Selection interaction remains in the lower-level KisToolSelectBase.
 */
template <class BaseClass>
class KRITAUI_EXPORT KisToolSelectUiBase : public KisToolSelectBase<BaseClass>
{
    using InteractionBase = KisToolSelectBase<BaseClass>;

public:
    KisToolSelectUiBase(KoCanvasBase *canvas, const QString &toolName)
        : InteractionBase(canvas)
        , m_widgetHelper(toolName)
    {
    }

    KisToolSelectUiBase(KoCanvasBase *canvas,
                        const QCursor &cursor,
                        const QString &toolName)
        : InteractionBase(canvas, cursor)
        , m_widgetHelper(toolName)
    {
    }

    KisToolSelectUiBase(KoCanvasBase *canvas,
                        const QCursor &cursor,
                        const QString &toolName,
                        KoToolBase *delegateTool)
        : InteractionBase(canvas, cursor, delegateTool)
        , m_widgetHelper(toolName)
    {
    }

    enum SampleLayersMode {
        SampleAllLayers,
        SampleCurrentLayer,
        SampleColorLabeledLayers
    };

    void updateActionShortcutToolTips()
    {
        KisSelectionOptions *widget = m_widgetHelper.optionWidget();
        if (!widget) {
            return;
        }

        widget->updateActionButtonToolTip(
            SELECTION_REPLACE,
            this->action("selection_tool_mode_replace")->shortcut());
        widget->updateActionButtonToolTip(
            SELECTION_ADD,
            this->action("selection_tool_mode_add")->shortcut());
        widget->updateActionButtonToolTip(
            SELECTION_SUBTRACT,
            this->action("selection_tool_mode_subtract")->shortcut());
        widget->updateActionButtonToolTip(
            SELECTION_INTERSECT,
            this->action("selection_tool_mode_intersect")->shortcut());
    }

    void activate(const QSet<KoShape *> &shapes) override
    {
        InteractionBase::activate(shapes);

        m_modeConnections.addUniqueConnection(
            this->action("selection_tool_mode_replace"),
            SIGNAL(triggered()),
            &m_widgetHelper,
            SLOT(slotReplaceModeRequested()));
        m_modeConnections.addUniqueConnection(
            this->action("selection_tool_mode_add"),
            SIGNAL(triggered()),
            &m_widgetHelper,
            SLOT(slotAddModeRequested()));
        m_modeConnections.addUniqueConnection(
            this->action("selection_tool_mode_subtract"),
            SIGNAL(triggered()),
            &m_widgetHelper,
            SLOT(slotSubtractModeRequested()));
        m_modeConnections.addUniqueConnection(
            this->action("selection_tool_mode_intersect"),
            SIGNAL(triggered()),
            &m_widgetHelper,
            SLOT(slotIntersectModeRequested()));

        updateActionShortcutToolTips();
        updateOptionSectionVisibility();
    }

    void deactivate() override
    {
        InteractionBase::deactivate();
        m_modeConnections.clear();
    }

    QWidget *createOptionWidget() override
    {
        m_widgetHelper.createOptionWidget(this->toolId());
        m_widgetHelper.setConfigGroupForExactTool(this->toolId());

        this->connect(this,
                      SIGNAL(isActiveChanged(bool)),
                      &m_widgetHelper,
                      SLOT(slotToolActivatedChanged(bool)));
        this->connect(&m_widgetHelper,
                      SIGNAL(selectionActionChanged(SelectionAction)),
                      this,
                      SLOT(resetCursorStyle()));

        updateActionShortcutToolTips();
        if (m_widgetHelper.optionWidget()) {
            m_widgetHelper.optionWidget()->setContentsMargins(0, 10, 0, 10);
        }
        updateOptionSectionVisibility();
        return m_widgetHelper.optionWidget();
    }

    SelectionMode selectionMode() const
    {
        return m_widgetHelper.selectionMode();
    }

    bool antiAliasSelection() const
    {
        return m_widgetHelper.antiAliasSelection();
    }

    int growSelection() const
    {
        return m_widgetHelper.growSelection();
    }

    bool stopGrowingAtDarkestPixel() const
    {
        return m_widgetHelper.stopGrowingAtDarkestPixel();
    }

    int featherSelection() const
    {
        return m_widgetHelper.featherSelection();
    }

    QList<int> colorLabelsSelected() const
    {
        return m_widgetHelper.selectedColorLabels();
    }

    SampleLayersMode sampleLayersMode() const
    {
        switch (m_widgetHelper.referenceLayers()) {
        case KisSelectionOptions::AllLayers:
            return SampleAllLayers;
        case KisSelectionOptions::CurrentLayer:
            return SampleCurrentLayer;
        case KisSelectionOptions::ColorLabeledLayers:
            return SampleColorLabeledLayers;
        }
        KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(false, SampleAllLayers);
    }

    KisSelectionOptions *selectionOptionWidget()
    {
        return m_widgetHelper.optionWidget();
    }

    QMenu *popupActionsMenu() override
    {
        if (this->isSelecting()) {
            return BaseClass::popupActionsMenu();
        }

        return KisSelectionToolHelper::getSelectionContextMenu(this->canvas());
    }

protected:
    KisSelectionToolConfigWidgetHelper m_widgetHelper;

    virtual bool isPixelOnly() const
    {
        return false;
    }

    virtual bool usesColorLabels() const
    {
        return false;
    }

    SelectionAction configuredSelectionAction() const override
    {
        return m_widgetHelper.selectionAction();
    }

private:
    void updateOptionSectionVisibility()
    {
        KisSelectionOptions *widget = m_widgetHelper.optionWidget();
        if (!widget) {
            return;
        }
        if (isPixelOnly()) {
            widget->setModeSectionVisible(false);
            widget->setAdjustmentsSectionVisible(true);
        }
        widget->setReferenceSectionVisible(usesColorLabels());
    }

    KisSignalAutoConnectionsStore m_modeConnections;
};

struct FakeBaseTool : KisTool
{
    explicit FakeBaseTool(KoCanvasBase *canvas)
        : KisTool(canvas, QCursor())
    {
    }

    FakeBaseTool(KoCanvasBase *canvas, const QCursor &cursor)
        : KisTool(canvas, cursor)
    {
    }
};

using KisToolSelect = KisToolSelectUiBase<FakeBaseTool>;

#endif
