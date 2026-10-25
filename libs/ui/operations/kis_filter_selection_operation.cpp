/*
 *  SPDX-FileCopyrightText: 2012 Dmitry Kazakov <dimula73@gmail.com>
 *  SPDX-FileCopyrightText: 2013 Sven Langkamp <sven.langkamp@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */


#include "kis_filter_selection_operation.h"
#include <commands_new/kis_transaction_based_command.h>
#include <application/ui/workspace/KisViewManager.h>
#include <kis_stroke_job_strategy.h>
#include <kis_selection.h>
#include <kis_selection_filters.h>
#include <kis_pixel_selection.h>
#include <kis_processing_applicator.h>
#include <kis_transaction.h>
#include <qpaintdevice.h>
#include <selection/kis_selection_manager.h>
#include <kis_command_utils.h>
#include "commands/KisDeselectActiveSelectionCommand.h"
#include "kis_types.h"
#include <kis_image.h>
#include "kundo2stack.h"
#include "operations/kis_operation_configuration.h"

void KisFilterSelectionOperation::runFilter(KisSelectionFilter* filter, KisViewManager* view, const KisOperationConfiguration& config)
{
    if (view->image() && view->image()->gpuImage()) {
        if (config.id() == QLatin1String("invertselection")) {
            delete filter;
            const QRect current = view->image()->gpuSelectionRect();
            if (current.isEmpty()) return;
            if (view->image()->gpuSelectionEllipse()) {
                view->showFloatingMessage(i18n("Inverting a non-rectangular GPU selection is not available."), QIcon());
                return;
            }
            if (!view->image()->setGpuSelectionRect(current, !view->image()->gpuSelectionInverted()))
                view->showFloatingMessage(i18n("The selection could not be inverted."), QIcon());
            return;
        }
        if (config.id() == QLatin1String("shrinkselection") ||
            config.id() == QLatin1String("growselection")) {
            const bool grow = config.id() == QLatin1String("growselection");
            const int xRadius = config.getInt("x-radius", 1);
            const int yRadius = config.getInt("y-radius", 1);
            const bool edgeLock = config.getBool("edgeLock", false);
            const QRect current = view->image()->gpuSelectionRect();
            delete filter;
            if (xRadius < 0 || yRadius < 0) {
                view->showFloatingMessage(i18n("The selection radius is invalid."), QIcon());
                return;
            }
            if (current.isEmpty() || (xRadius == 0 && yRadius == 0)) return;
            if (view->image()->gpuSelectionEllipse()) {
                view->showFloatingMessage(i18n("Growing or shrinking an elliptical GPU selection is not available."), QIcon());
                return;
            }
            const QRect imageBounds(QPoint(), view->image()->size());
            if (view->image()->gpuSelectionInverted()) {
                if (edgeLock) {
                    view->showFloatingMessage(i18n("Edge-locked shrinking of an inverted GPU selection is not available."), QIcon());
                    return;
                }
                const QRect hole = grow
                    ? current.adjusted(xRadius, yRadius, -xRadius, -yRadius)
                    : current.adjusted(-xRadius, -yRadius, xRadius, yRadius).intersected(imageBounds);
                if (hole.isEmpty()) view->image()->setGpuSelectionRect(imageBounds);
                else if (hole == imageBounds) view->image()->clearGpuSelection();
                else if (!view->image()->setGpuSelectionRect(hole, true))
                    view->showFloatingMessage(i18n("The selection could not be updated."), QIcon());
                return;
            }
            QRect updated;
            if (grow) {
                updated = current.adjusted(-xRadius, -yRadius, xRadius, yRadius)
                    .intersected(QRect(QPoint(), view->image()->size()));
            } else {
                int left = xRadius, right = xRadius, top = yRadius, bottom = yRadius;
                if (edgeLock) {
                    if (current.left() <= 0) left = 0;
                    if (current.top() <= 0) top = 0;
                    if (current.right() + 1 >= view->image()->width()) right = 0;
                    if (current.bottom() + 1 >= view->image()->height()) bottom = 0;
                }
                updated = current.adjusted(left, top, -right, -bottom);
            }
            if (updated.isEmpty()) view->image()->clearGpuSelection();
            else if (!view->image()->setGpuSelectionRect(updated))
                view->showFloatingMessage(i18n("The selection could not be updated."), QIcon());
            return;
        }
        view->showFloatingMessage(i18n("Only invert, grow and shrink are available for a rectangular GPU selection."), QIcon());
        delete filter;
        return;
    }
    KisSelectionSP selection = view->selection();
    if (!selection) return;

    struct FilterSelection : public KisTransactionBasedCommand {
        FilterSelection(KisImageSP image, KisSelectionSP sel, KisSelectionFilter *filter)
            : m_image(image), m_sel(sel), m_filter(filter) {}
        ~FilterSelection() override { delete m_filter;}
        KisImageSP m_image;
        KisSelectionSP m_sel;
        KisSelectionFilter *m_filter;

        KUndo2Command* paint() override {
            KisPixelSelectionSP mergedSelection = m_sel->pixelSelection();
            KisTransaction transaction(mergedSelection);
            QRect processingRect = m_filter->changeRect(mergedSelection->selectedExactRect(), mergedSelection->defaultBounds());
            m_filter->process(mergedSelection, processingRect);
            KUndo2Command *savedCommand = transaction.endAndTake();
            mergedSelection->setDirty(processingRect);
            if (m_sel->selectedExactRect().isEmpty() || m_sel->pixelSelection()->outline().isEmpty()) {
                KisCommandUtils::CompositeCommand *cmd = new KisCommandUtils::CompositeCommand();
                cmd->addCommand(savedCommand);
                cmd->addCommand(new KisDeselectActiveSelectionCommand(m_sel, m_image));
                savedCommand = cmd;
            }

            return savedCommand;
        }
    };

    KisProcessingApplicator *ap = beginAction(view, filter->name());
    ap->applyCommand(new FilterSelection(view->image(), selection, filter),
                     KisStrokeJobData::SEQUENTIAL, KisStrokeJobData::NORMAL);
    endAction(ap, config.toXML());
}
