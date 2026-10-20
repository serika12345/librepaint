/*
 *  SPDX-FileCopyrightText: 2005 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2011 José Luis Vergara <pentalis@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_dlg_layer_properties.h"
#include <limits.h>

#include <QLayout>
#include <QString>
#include <QGroupBox>
#include <QVBoxLayout>

#include <klocalizedstring.h>

#include <KoColorSpace.h>
#include <KoChannelInfo.h>

#include <KisSpinBoxI18nHelper.h>
#include "KoDialog.h"
#include "application/ui/workspace/KisViewManager.h"

#include <kis_cmb_composite.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <QtGlobal>
#include <qlist.h>
#include <qnamespace.h>
#include <qobjectdefs.h>
#include <qpaintdevice.h>
#include <qpointer.h>
#include <qsharedpointer.h>
#include "KoColorProfile.h"
#include "kis_base_node.h"
#include "kis_pointer_utils.h"
#include "kis_types.h"
#include "kundo2magicstring.h"
#include "nodes/kis_multinode_property.h"
#include "kis_layer_utils.h"
#include "kis_image.h"
#include "kis_signal_compressor.h"
#include "commands_new/kis_saved_commands.h"
#include "kis_post_execution_undo_adapter.h"
#include <gpu/KisGpuPaintImage.h>
#include <KoCompositeOpRegistry.h>
#include <kis_group_layer.h>
#include <kis_layer_properties_icons.h>
#include <algorithm>


struct KisDlgLayerProperties::Private
{
    Private() : updatesCompressor(500, KisSignalCompressor::POSTPONE) {}

    KisNodeList nodes;
    const KoColorSpace *colorSpace {0};
    KisViewManager *view {0};
    LibrePaint::Image::Gpu::PaintImageSP gpuOwner;
    LibrePaint::Image::Gpu::Snapshot gpuBefore;
    bool gpuCancelled = false;
    WdgLayerProperties *page {0};

    QSharedPointer<KisMultinodeCompositeOpProperty> compositeOpProperty;
    QSharedPointer<KisMultinodeOpacityProperty> opacityProperty;
    QSharedPointer<KisMultinodeNameProperty> nameProperty;
    QSharedPointer<KisMultinodeColorLabelProperty> colorLabelProperty;

    QList<KisMultinodePropertyInterfaceSP> layerProperties;
    QList<QString> layerPropertyIds;
    QList<QPointer<QCheckBox> > layerPropCheckboxes;

    QList<KisMultinodePropertyInterfaceSP> channelFlagsProps;
    QList<quint8> channelFlagMasks;
    QList<QPointer<QCheckBox> > channelFlagsCheckboxes;

    KisSignalCompressor updatesCompressor;

    QList<LibrePaint::Image::Gpu::LayerPropertiesSelection> selectedGpuProperties(bool survivingOnly = false) const {
        namespace Gpu = LibrePaint::Image::Gpu;
        const auto after = gpuOwner->operations().snapshot();
        QList<Gpu::LayerPropertiesSelection> selection;
        for (const auto &node : nodes) {
            Gpu::LayerPropertiesSelection fields;
            for (const auto &layer : after->layers)
                if (layer.uuid == node->uuid()) fields.layer = layer.id;
            if (survivingOnly) {
                const auto original = std::find_if(gpuBefore->layers.cbegin(), gpuBefore->layers.cend(),
                    [&](const auto &layer) { return layer.id == fields.layer && layer.uuid == node->uuid(); });
                if (original == gpuBefore->layers.cend()) continue;
            }
            fields.name = !nameProperty->isIgnored();
            fields.opacity = !opacityProperty->isIgnored();
            fields.colorLabel = !colorLabelProperty->isIgnored();
            fields.blendMode = !compositeOpProperty->isIgnored();
            const auto properties = node->sectionModelProperties();
            for (qsizetype index = 0; index < layerProperties.size(); ++index) {
                if (layerProperties[index]->isIgnored()) continue;
                const auto &id = layerPropertyIds[index];
                if (!std::any_of(properties.cbegin(), properties.cend(),
                    [&](const auto &property) { return property.id == id; })) continue;
                if (id == KisLayerPropertiesIcons::visible.id()) fields.visible = true;
                else if (id == KisLayerPropertiesIcons::locked.id()) fields.locked = true;
                else if (id == KisLayerPropertiesIcons::alphaLocked.id()) fields.protectedChannels |= 8;
                else if (id == KisLayerPropertiesIcons::inheritAlpha.id()) fields.projectionChannels |= 8;
                else if (id == KisLayerPropertiesIcons::passThrough.id()) fields.passThrough = true;
            }
            for (qsizetype index = 0; index < channelFlagsProps.size(); ++index)
                if (!channelFlagsProps[index]->isIgnored()) fields.projectionChannels |= channelFlagMasks[index];
            selection.append(fields);
        }
        return selection;
    }

    void cancelUnsupportedGpuPreviews() {
        compositeOpProperty->setIgnored(true);
    }

    QList<KisMultinodePropertyInterfaceSP> allProperties() const {
        QList<KisMultinodePropertyInterfaceSP> props;
        props << compositeOpProperty;
        props << opacityProperty;
        props << nameProperty;
        props << layerProperties;
        props << channelFlagsProps;
        props << colorLabelProperty;
        return props;
    }
};

KisDlgLayerProperties::KisDlgLayerProperties(KisNodeList nodes, KisViewManager *view, QWidget *parent, const char *name, Qt::WindowFlags f)
    : KoDialog(parent)
    , d(new Private())
{
    nodes = KisLayerUtils::sortMergeableNodes(view->image()->root(), nodes);
    d->nodes = nodes;

    Q_UNUSED(f);
    setCaption(i18n("Layer Properties"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);
    setModal(false);

    setObjectName(name);
    d->page = new WdgLayerProperties(this);
    setMainWidget(d->page);
    d->view = view;
    d->gpuOwner = view->image()->gpuImage();
    if (d->gpuOwner) d->gpuBefore = d->gpuOwner->operations().snapshot();
    d->colorSpace = d->nodes.first()->colorSpace();

    d->page->editName->setFocus();
    d->nameProperty.reset(new KisMultinodeNameProperty(nodes));
    d->nameProperty->connectIgnoreCheckBox(d->page->chkName);
    d->nameProperty->connectAutoEnableWidget(d->page->editName);
    d->nameProperty->connectValueChangedSignal(this, SLOT(slotNameValueChangedInternally()));
    connect(d->page->editName, SIGNAL(textChanged(QString)), SLOT(slotNameValueChangedExternally()));

    d->page->intOpacity->setRange(0, 100);
    KisSpinBoxI18nHelper::setText(d->page->intOpacity, i18nc("{n} is the number value, % is the percent sign", "{n}%"));
    d->opacityProperty.reset(new KisMultinodeOpacityProperty(nodes));
    d->opacityProperty->connectIgnoreCheckBox(d->page->chkOpacity);
    d->opacityProperty->connectAutoEnableWidget(d->page->intOpacity);
    d->opacityProperty->connectValueChangedSignal(this, SLOT(slotOpacityValueChangedInternally()));
    d->opacityProperty->connectValueChangedSignal(&d->updatesCompressor, SLOT(start()));
    connect(d->page->intOpacity, SIGNAL(valueChanged(int)), SLOT(slotOpacityValueChangedExternally()));

    d->compositeOpProperty.reset(new KisMultinodeCompositeOpProperty(nodes));
    d->compositeOpProperty->connectIgnoreCheckBox(d->page->chkCompositeOp);
    d->compositeOpProperty->connectAutoEnableWidget(d->page->cmbComposite);
    d->compositeOpProperty->connectValueChangedSignal(this, SLOT(slotCompositeOpValueChangedInternally()));
    d->compositeOpProperty->connectValueChangedSignal(&d->updatesCompressor, SLOT(start()));
    connect(d->page->cmbComposite, SIGNAL(currentIndexChanged(int)), SLOT(slotCompositeOpValueChangedExternally()));

    d->page->colorLabelSelector->setFocusPolicy(Qt::StrongFocus);
    d->colorLabelProperty.reset(new KisMultinodeColorLabelProperty(nodes));
    d->colorLabelProperty->connectIgnoreCheckBox(d->page->chkColorLabel);
    d->colorLabelProperty->connectAutoEnableWidget(d->page->colorLabelSelector);
    d->colorLabelProperty->connectValueChangedSignal(this, SLOT(slotColorLabelValueChangedInternally()));
    d->colorLabelProperty->connectValueChangedSignal(&d->updatesCompressor, SLOT(start()));
    connect(d->page->colorLabelSelector, SIGNAL(currentIndexChanged(int)), SLOT(slotColorLabelValueChangedExternally()));

    if (!KisLayerUtils::checkNodesDiffer<const KoColorSpace*>(d->nodes, [](KisNodeSP node) { return node->colorSpace(); })) {

        d->page->lblColorSpace->setText(d->colorSpace->name());
        if (const KoColorProfile* profile = d->colorSpace->profile()) {
            d->page->lblProfile->setText(profile->name());
        }

        QRect bounds = d->nodes.first()->exactBounds();
        d->page->lblDimensions->setText(i18nc("layer dimensions", "(%1, %2), (%3, %4)",
                                        bounds.x(),
                                        bounds.y(),
                                        bounds.width(),
                                        bounds.height()));

        ChannelFlagAdapter::PropertyList props = ChannelFlagAdapter::adaptersList(nodes);
        if (!props.isEmpty()) {
            QVBoxLayout *vbox = new QVBoxLayout;
            Q_FOREACH (const ChannelFlagAdapter::Property &prop, props) {
                QCheckBox *chk = new QCheckBox(prop.name, this);
                vbox->addWidget(chk);

                KisMultinodePropertyInterface *multiprop =
                    new KisMultinodeProperty<ChannelFlagAdapter>(
                        nodes,
                        ChannelFlagAdapter(prop));

                multiprop->connectIgnoreCheckBox(chk);
                multiprop->connectValueChangedSignal(this, SLOT(slotFlagsValueChangedInternally()));
                multiprop->connectValueChangedSignal(&d->updatesCompressor, SLOT(start()));

                d->channelFlagsCheckboxes << chk;
                d->channelFlagsProps << toQShared(multiprop);
                d->channelFlagMasks << quint8(1u << d->colorSpace->channels()[prop.channelIndex]->displayPosition());
            }

            d->page->grpActiveChannels->setLayout(vbox);
        } else {
            d->page->grpActiveChannels->setVisible(false);
            d->page->lineActiveChannels->setVisible(false);
        }
    } else {
        d->page->grpActiveChannels->setVisible(false);
        d->page->lineActiveChannels->setVisible(false);
        d->page->cmbComposite->setEnabled(false);
        d->page->chkCompositeOp->setEnabled(false);
        d->page->lblDimensions->setText(i18n("*varies*"));
        d->page->lblColorSpace->setText(i18n("*varies*"));
        d->page->lblProfile->setText(i18n("*varies*"));
    }

    {
        QVBoxLayout *vbox = new QVBoxLayout;

        KisBaseNode::PropertyList props = LayerPropertyAdapter::adaptersList(nodes);
        Q_FOREACH (const KisBaseNode::Property &prop, props) {
            QCheckBox *chk = new QCheckBox(prop.name, this);
            chk->setIcon(prop.onIcon);
            vbox->addWidget(chk);

            KisMultinodePropertyInterface *multiprop =
                new KisMultinodeProperty<LayerPropertyAdapter>(
                    nodes,
                    LayerPropertyAdapter(prop.name));

            multiprop->connectIgnoreCheckBox(chk);
            multiprop->connectValueChangedSignal(this, SLOT(slotPropertyValueChangedInternally()));
            multiprop->connectValueChangedSignal(&d->updatesCompressor, SLOT(start()));

            d->layerPropCheckboxes << chk;
            d->layerProperties << toQShared(multiprop);
            d->layerPropertyIds << prop.id;
        }

        d->page->grpProperties->setLayout(vbox);
    }

    connect(&d->updatesCompressor, SIGNAL(timeout()), SLOT(updatePreview()));
}

KisDlgLayerProperties::~KisDlgLayerProperties()
{
    if (d->gpuOwner) {
        if (result() != QDialog::Accepted && !d->gpuCancelled) {
            namespace Gpu = LibrePaint::Image::Gpu;
            const auto status = d->gpuOwner->restoreLayerProperties(d->gpuBefore, d->selectedGpuProperties(true));
            if (status != Gpu::Result::Success && status != Gpu::Result::Closed && status != Gpu::Result::StaleVersion)
                qWarning("Could not restore GPU layer properties while destroying the dialog");
            d->cancelUnsupportedGpuPreviews();
        }
        return;
    }
    if (result() == QDialog::Accepted) {
        if (d->updatesCompressor.isActive()) {
            d->updatesCompressor.stop();
            updatePreview();
        }

        KisPostExecutionUndoAdapter *adapter =
            d->view->image()->postExecutionUndoAdapter();
        KisSavedMacroCommand *macro = adapter->createMacro(kundo2_i18n("Change Layer Properties"));
        macro->addCommand(toQShared(new KisLayerUtils::KisSimpleUpdateCommand(d->nodes, false)));
        Q_FOREACH(auto prop, d->allProperties()) {
            if (!prop->isIgnored()) {
                macro->addCommand(toQShared(prop->createPostExecutionUndoCommand()));
            }
        }
        macro->addCommand(toQShared(new KisLayerUtils::KisSimpleUpdateCommand(d->nodes, true)));
        adapter->addMacro(macro);
    }
    else /* if (result() == QDialog::Rejected) */ {
        Q_FOREACH(auto prop, d->allProperties()) {
            prop->setIgnored(true);
        }
        updatePreview();
    }
}

void KisDlgLayerProperties::accept()
{
    if (d->gpuOwner) {
        namespace Gpu = LibrePaint::Image::Gpu;
        if (d->gpuOwner->operations().hasActiveStroke()) {
            d->view->showFloatingMessage(i18n("Finish the current stroke before applying layer properties."), QIcon());
            return;
        }
        for (const auto &node : d->nodes) {
            if (node->compositeOpId() != COMPOSITE_OVER && node->compositeOpId() != COMPOSITE_MULT && node->compositeOpId() != COMPOSITE_SCREEN &&
                node->compositeOpId() != COMPOSITE_DARKEN && node->compositeOpId() != COMPOSITE_LIGHTEN) {
                d->view->showFloatingMessage(i18n("The selected blending mode cannot be applied to this document."), QIcon());
                return;
            }
        }
        const auto after = d->gpuOwner->operations().snapshot();
        const auto selection = d->selectedGpuProperties();
        if (d->gpuOwner->recordLayerProperties(d->gpuBefore, after, selection) != Gpu::Result::Success) {
            d->view->showFloatingMessage(i18n("Layer properties could not be applied. Try again or cancel the changes."), QIcon());
            return;
        }
        d->updatesCompressor.stop();
    }
    KoDialog::accept();
}

void KisDlgLayerProperties::reject()
{
    if (d->gpuOwner) {
        namespace Gpu = LibrePaint::Image::Gpu;
        const auto status = d->gpuOwner->restoreLayerProperties(d->gpuBefore, d->selectedGpuProperties(true));
        if (status != Gpu::Result::Success && status != Gpu::Result::Closed && status != Gpu::Result::StaleVersion) {
            d->view->showFloatingMessage(i18n("Layer property changes could not be cancelled. Try again."), QIcon());
            return;
        }
        d->cancelUnsupportedGpuPreviews();
        d->gpuCancelled = true;
        d->updatesCompressor.stop();
    }
    KoDialog::reject();
}

void KisDlgLayerProperties::slotCompositeOpValueChangedInternally()
{
    d->page->cmbComposite->validate(d->colorSpace);
    d->page->cmbComposite->selectCompositeOp(KoID(d->compositeOpProperty->value()));
    d->page->cmbComposite->setEnabled(!d->compositeOpProperty->isIgnored());
}

void KisDlgLayerProperties::slotCompositeOpValueChangedExternally()
{
    if (d->compositeOpProperty->isIgnored()) return;
    d->compositeOpProperty->setValue(d->page->cmbComposite->selectedCompositeOp().id());
}

void KisDlgLayerProperties::slotColorLabelValueChangedInternally()
{
    d->page->colorLabelSelector->setCurrentIndex(d->colorLabelProperty->value());
    d->page->colorLabelSelector->setEnabled(!d->colorLabelProperty->isIgnored());
}

void KisDlgLayerProperties::slotColorLabelValueChangedExternally()
{
    if (d->colorLabelProperty->isIgnored()) return;
    d->colorLabelProperty->setValue(d->page->colorLabelSelector->currentIndex());
}

void KisDlgLayerProperties::slotOpacityValueChangedInternally()
{
    d->page->intOpacity->setValue(d->opacityProperty->value());
    d->page->intOpacity->setEnabled(!d->opacityProperty->isIgnored());
}

void KisDlgLayerProperties::slotOpacityValueChangedExternally()
{
    if (d->opacityProperty->isIgnored()) return;
    d->opacityProperty->setValue(d->page->intOpacity->value());
}

void KisDlgLayerProperties::slotNameValueChangedInternally()
{
    if (d->page->editName->text() != d->nameProperty->value()) {
        d->page->editName->setText(d->nameProperty->value());
    }

    d->page->editName->setEnabled(!d->nameProperty->isIgnored());
}

void KisDlgLayerProperties::slotNameValueChangedExternally()
{
    if (d->nameProperty->isIgnored()) return;

    if (d->page->editName->text() != d->nameProperty->value()) {
        d->nameProperty->setValue(d->page->editName->text());
    }
}

void KisDlgLayerProperties::slotPropertyValueChangedInternally()
{
    Q_FOREACH (KisMultinodePropertyInterfaceSP prop, d->channelFlagsProps) {
        prop->rereadCurrentValue();
    }
}

void KisDlgLayerProperties::slotFlagsValueChangedInternally()
{
    Q_FOREACH (KisMultinodePropertyInterfaceSP prop, d->layerProperties) {
        prop->rereadCurrentValue();
    }
}

void KisDlgLayerProperties::updatePreview()
{
    if (d->gpuOwner) return;
    KisLayerUtils::KisSimpleUpdateCommand::updateNodes(d->nodes);
}
