/*
 *  SPDX-FileCopyrightText: 2006 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2008 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_dlg_adjustment_layer.h"
#include <kguiitem.h>
#include <klocalizedstring.h>
#include <kstandardguiitem.h>

#include <QGroupBox>
#include <QLayout>
#include <QGridLayout>
#include <QPixmap>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <filter/kis_filter_registry.h>

#include "KoDialog.h"
#include "filter/kis_filter.h"
#include "kis_config_widget.h"
#include "filter/kis_filter_configuration.h"
#include "kis_paint_device.h"
#include "kis_node.h"
#include "kis_node_filter_interface.h"
#include <application/kis_config.h>
#include "application/ui/workspace/KisViewManager.h"
#include <kis_icon_utils.h>
#include <QtGlobal>
#include <qnamespace.h>
#include <qobjectdefs.h>


KisDlgAdjustmentLayer::KisDlgAdjustmentLayer(KisNodeSP node,
                                             KisNodeFilterInterface* nfi,
                                             KisPaintDeviceSP paintDevice,
                                             const QString &layerName,
                                             const QString &caption,
                                             KisViewManager *view, QWidget *parent)
    : KoDialog(parent, Qt::Dialog)
    , m_node(node)
    , m_nodeFilterInterface(nfi)
    , m_currentFilter(0)
    , m_customName(false)
    , m_layerName(layerName)
{
    setCaption(caption);
    setButtons(None);

    QWidget * page = new QWidget(this);
    wdgFilterNodeCreation.setupUi(page);
    setMainWidget(page);
    if (!m_nodeFilterInterface) {
        setObjectName(QStringLiteral("gpuFilterNodeDialog"));
        m_previewDiagnostic = new QLabel(page);
        m_previewDiagnostic->setObjectName(QStringLiteral("filterPreviewDiagnostic"));
        m_previewDiagnostic->setWordWrap(true);
        m_previewDiagnostic->hide();
        page->layout()->addWidget(m_previewDiagnostic);
        wdgFilterNodeCreation.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }

    wdgFilterNodeCreation.filterGalleryToggle->setChecked(wdgFilterNodeCreation.filterSelector->isFilterGalleryVisible());
    wdgFilterNodeCreation.filterGalleryToggle->setIcon(KisIconUtils::loadIcon("sidebaricon"));
    wdgFilterNodeCreation.filterGalleryToggle->setMaximumWidth(wdgFilterNodeCreation.filterGalleryToggle->height());
    connect(wdgFilterNodeCreation.filterSelector, SIGNAL(sigFilterGalleryToggled(bool)), wdgFilterNodeCreation.filterGalleryToggle, SLOT(setChecked(bool)));
    connect(wdgFilterNodeCreation.filterGalleryToggle, SIGNAL(toggled(bool)), wdgFilterNodeCreation.filterSelector, SLOT(showFilterGallery(bool)));
    connect(wdgFilterNodeCreation.filterSelector, SIGNAL(sigSizeChanged()), this, SLOT(slotFilterWidgetSizeChanged()));

    KGuiItem::assign(wdgFilterNodeCreation.buttonBox->button(QDialogButtonBox::Ok), KStandardGuiItem::ok());
    KGuiItem::assign(wdgFilterNodeCreation.buttonBox->button(QDialogButtonBox::Cancel), KStandardGuiItem::cancel());
    connect(wdgFilterNodeCreation.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(wdgFilterNodeCreation.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    wdgFilterNodeCreation.filterSelector->setView(view);
    wdgFilterNodeCreation.filterSelector->showFilterGallery(KisConfig(true).showFilterGalleryLayerMaskDialog());

    wdgFilterNodeCreation.filterSelector->setPaintDevice(false, paintDevice);
    wdgFilterNodeCreation.layerName->setText(layerName);

    connect(wdgFilterNodeCreation.filterSelector, SIGNAL(configurationChanged()), SLOT(slotConfigChanged()));
    connect(wdgFilterNodeCreation.layerName, SIGNAL(textChanged(QString)), SLOT(slotNameChanged(QString)));

    slotConfigChanged();
}

KisDlgAdjustmentLayer::~KisDlgAdjustmentLayer()
{
    KisConfig(true).setShowFilterGalleryLayerMaskDialog(wdgFilterNodeCreation.filterSelector->isFilterGalleryVisible());
}

void KisDlgAdjustmentLayer::slotNameChanged(const QString &text)
{
    Q_UNUSED(text);
    m_customName = !text.isEmpty();
    if (m_nodeFilterInterface) enableButtonOk(m_currentFilter);
    else Q_EMIT configurationPreviewRequested(m_currentFilter, layerName());
}

void KisDlgAdjustmentLayer::setInitialConfiguration(const KisFilterConfigurationSP &configuration, bool preserveName)
{
    if (!configuration) return;
    m_customName = preserveName;
    const auto filter = KisFilterRegistry::instance()->value(configuration->name());
    if (!filter) return;
    wdgFilterNodeCreation.filterSelector->setFilter(filter, configuration);
    slotConfigChanged();
}

void KisDlgAdjustmentLayer::setPreviewAccepted(bool accepted, const QString &diagnostic)
{
    if (m_nodeFilterInterface) return;
    wdgFilterNodeCreation.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(accepted);
    if (m_previewDiagnostic) {
        m_previewDiagnostic->setText(diagnostic);
        m_previewDiagnostic->setVisible(!diagnostic.isEmpty());
    }
}

KisFilterConfigurationSP  KisDlgAdjustmentLayer::filterConfiguration() const
{
    KisFilterConfigurationSP config = wdgFilterNodeCreation.filterSelector->configuration();

    return config;
}

QString KisDlgAdjustmentLayer::layerName() const
{
    return wdgFilterNodeCreation.layerName->text();
}

void KisDlgAdjustmentLayer::slotConfigChanged()
{
    m_currentFilter = filterConfiguration();
    if (m_nodeFilterInterface) enableButtonOk(m_currentFilter);
    if (m_nodeFilterInterface && m_currentFilter)
        m_nodeFilterInterface->setFilter(m_currentFilter->cloneWithResourcesSnapshot());
    if (m_currentFilter && !m_customName) {
        wdgFilterNodeCreation.layerName->blockSignals(true);
        wdgFilterNodeCreation.layerName->setText(m_layerName + " (" + wdgFilterNodeCreation.filterSelector->currentFilter()->name() + ")");
        wdgFilterNodeCreation.layerName->blockSignals(false);
    }
    if (!m_nodeFilterInterface) {
        Q_EMIT configurationPreviewRequested(m_currentFilter, layerName());
        return;
    }
    if (m_node) m_node->setDirty();
}

void KisDlgAdjustmentLayer::requestPreview()
{
    slotConfigChanged();
}

void KisDlgAdjustmentLayer::adjustSize()
{
    QWidget::adjustSize();
}

void KisDlgAdjustmentLayer::slotFilterWidgetSizeChanged()
{
    QMetaObject::invokeMethod(this, "adjustSize", Qt::QueuedConnection);
}

