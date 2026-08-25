/*
 *  SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2018 Emmet & Eoin O'Neill <emmetoneill.pdx@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tool_utils.h"

#include <kis_transaction.h>
#include <kis_properties_configuration.h>
#include <kconfiggroup.h>
#include <ksharedconfig.h>
#include "kis_layer_utils.h"
#include "kis_selection.h"
#include "kis_command_utils.h"
#include "kis_processing_applicator.h"

#include <algorithm>

#include "KisAnimAutoKey.h"

#include <QApplication>

namespace KisToolUtils {

    bool clearImage(KisImageSP image, KisNodeList nodes, KisSelectionSP selection)
    {
        KisNodeList masks;

        Q_FOREACH (KisNodeSP node, nodes) {
            if (node->inherits("KisMask")) {
                masks.append(node);
            }
        }

        // To prevent deleting same layer multiple times
        KisLayerUtils::filterMergeableNodes(nodes);
        nodes.append(masks);

        if (nodes.isEmpty()) {
            return false;
        }

        KisProcessingApplicator applicator(image, 0, KisProcessingApplicator::NONE,
                                           KisImageSignalVector(), kundo2_i18n("Clear"));

        Q_FOREACH (KisNodeSP node, nodes) {
            KisLayerUtils::recursiveApplyNodes(node, [&applicator, selection, masks] (KisNodeSP node) {

                // applied on masks if selected explicitly
                if (node->inherits("KisMask") && !masks.contains(node)) {
                    return;
                }

                if(node->hasEditablePaintDevice()) {
                    KUndo2Command *cmd =
                        new KisCommandUtils::LambdaCommand(kundo2_i18n("Clear"),
                            [node, selection] () {
                                KisPaintDeviceSP device = node->paintDevice();

                                std::unique_ptr<KisCommandUtils::CompositeCommand> parentCommand(
                                    new KisCommandUtils::CompositeCommand());

                                KUndo2Command *autoKeyframeCommand = KisAutoKey::tryAutoCreateDuplicatedFrame(device);
                                if (autoKeyframeCommand) {
                                    parentCommand->addCommand(autoKeyframeCommand);
                                }

                                KisTransaction transaction(kundo2_noi18n("internal-clear-command"), device);

                                QRect dirtyRect;
                                if (selection) {
                                    dirtyRect = selection->selectedRect();
                                    device->clearSelection(selection);
                                } else {
                                    dirtyRect = device->extent();
                                    device->clear();
                                }

                                device->setDirty(dirtyRect);
                                parentCommand->addCommand(transaction.endAndTake());

                                return parentCommand.release();
                            });
                    applicator.applyCommand(cmd, KisStrokeJobData::CONCURRENT);
                }
            });
        }

        applicator.end();

        return true;
    }

    const QString ColorSamplerConfig::CONFIG_GROUP_NAME = "tool_color_sampler";

    ColorSamplerConfig::ColorSamplerConfig()
        : toForegroundColor(true)
        , updateColor(true)
        , addColorToCurrentPalette(false)
        , normaliseValues(false)
        , sampleMerged(true)
        , radius(1)
        , blend(100)
    {
    }

    void ColorSamplerConfig::save() const
    {
        KisPropertiesConfiguration props;
        props.setProperty("toForegroundColor", toForegroundColor);
        props.setProperty("updateColor", updateColor);
        props.setProperty("addPalette", addColorToCurrentPalette);
        props.setProperty("normaliseValues", normaliseValues);
        props.setProperty("sampleMerged", sampleMerged);
        props.setProperty("radius", radius);
        props.setProperty("blend", blend);

        KConfigGroup config =  KSharedConfig::openConfig()->group(CONFIG_GROUP_NAME);

        config.writeEntry("ColorSamplerDefaultActivation", props.toXML());
    }

    void ColorSamplerConfig::load()
    {
        KisPropertiesConfiguration props;

        KConfigGroup config =  KSharedConfig::openConfig()->group(CONFIG_GROUP_NAME);
        props.fromXML(config.readEntry("ColorSamplerDefaultActivation"));

        toForegroundColor = props.getBool("toForegroundColor", true);
        updateColor = props.getBool("updateColor", true);
        addColorToCurrentPalette = props.getBool("addPalette", false);
        normaliseValues = props.getBool("normaliseValues", false);
        sampleMerged = props.getBool("sampleMerged", true);
        radius = props.getInt("radius", 1);
        blend = props.getInt("blend", 100);
    }

    void setCursorPos(const QPoint &point)
    {
        // https://bugreports.qt.io/browse/QTBUG-99009
        QScreen *screen = qApp->screenAt(point);
        if (!screen) {
            screen = qApp->primaryScreen();
        }
        QCursor::setPos(screen, point);
    }

    QString nodeEditableMessage(KisNodeSP node, bool blockedNoIndirectPainting)
    {
        QString message;
        if (!node->isEditable(true) || blockedNoIndirectPainting) {
            if (!node->visible() && node->userLocked()) {
                message = i18n("Layer is locked and invisible.");
            } else if (node->userLocked()) {
                message = i18n("Layer is locked.");
            } else if(!node->visible()) {
                message = i18n("Layer is invisible.");
            } else if (blockedNoIndirectPainting) {
                message = i18n("Layer can be painted in Wash Mode only.");
            } else {
                message = i18n("Group not editable.");
            }
        }
        return message;
    }


    StandardBrushSizes::StandardBrushSizes(int minSize, int maxSize)
    {
        int brushSize = minSize;
        do {
            m_sizes.push_back(brushSize);
            int increment = qMax(1, int(std::ceil(qreal(brushSize) / 15)));
            brushSize += increment;
        } while (brushSize < maxSize);
        m_sizes.push_back(maxSize);
    }

    int StandardBrushSizes::increaseBrushSize(qreal size)
    {
        std::vector<int>::iterator result = std::upper_bound(m_sizes.begin(), m_sizes.end(), qRound(size));
        return result != m_sizes.end() ? *result : m_sizes.back();
    }

    int StandardBrushSizes::decreaseBrushSize(qreal size)
    {
        std::vector<int>::reverse_iterator result =
            std::upper_bound(m_sizes.rbegin(), m_sizes.rend(), qRound(size), std::greater<int>());
        return result != m_sizes.rend() ? *result : m_sizes.front();
    }


}
