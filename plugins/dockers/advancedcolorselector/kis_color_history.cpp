/*
 *  SPDX-FileCopyrightText: 2010 Adam Celarek <kdedev at xibo dot at>
 *  SPDX-FileCopyrightText: 2022 Bourumir Wyngs <bourumir.wyngs@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kis_color_history.h"
#include "KisUniqueColorSet.h"
#include "kis_canvas2.h"
#include "KisViewManager.h"
#include "canvas/kis_canvas_resource_provider.h"

#include <KoCompositeOpRegistry.h>

#include <QToolButton>
#include <QList>
#include <kis_icon.h>
#include "KisDocument.h"
#include "kis_config_notifier.h"

Q_GLOBAL_STATIC(KisColorHistoryNotifier, s_color_history_change_notifier);

KisColorHistory::KisColorHistory(QWidget *parent)
    : KisColorPatches("lastUsedColors", parent)
    , m_document(0)
    , m_resourceProvider(0)
{
    m_fallbackColorHistoryModel = std::make_unique<KisUniqueColorSet>(new KisUniqueColorSet(this));
    m_clearButton = new QToolButton(this);
    m_clearButton->setIcon(KisIconUtils::loadIcon("dialog-cancel-16"));
    m_clearButton->setToolTip(i18n("Clear all color history"));
    m_clearButton->setAutoRaise(true);
    updateUserSettings();

    connect(m_clearButton, SIGNAL(clicked()), this, SLOT(clearColorHistory()));
    connect(KisConfigNotifier::instance(), SIGNAL(colorHistoryModeChanged()), this, SLOT(updateUserSettings()));
    connect(s_color_history_change_notifier, SIGNAL(colorHistoryChanged(const QList<KoColor>&)),
            this, SLOT(colorHistoryChanged()));

    setAdditionalButtons({m_clearButton});
}

KisColorHistory::~KisColorHistory()
{
}

void KisColorHistory::unsetCanvas()
{
    m_resourceProvider = 0;
    m_document = 0;

    KisColorPatches::unsetCanvas();
}

void KisColorHistory::setCanvas(KisCanvas2 *canvas)
{
    if (!canvas) return;

    KisColorPatches::setCanvas(canvas);

    if (m_resourceProvider) {
        m_resourceProvider->disconnect(this);
        m_resourceProvider->colorHistoryModel()->disconnect(this);
    }
    if (m_document) {
        m_document->colorHistoryModel()->disconnect(this);
    }

    m_resourceProvider = canvas->imageView()->resourceProvider();
    m_document = canvas->viewManager()->document();

    setColors(colorHistoryModel()->colorList());

    connect(m_resourceProvider, SIGNAL(sigFGColorUsed(KoColor)),
            this, SLOT(addColorToHistory(KoColor)), Qt::UniqueConnection);

    // Messy, but this whole plugin should be removed shortly in favour of the wide gamut selector.
    // We only really need this so the plugin will respond to the color history being cleared in the wide gamut selector.
    connect(m_resourceProvider->colorHistoryModel(), SIGNAL(sigReset()), this, SLOT(colorHistoryChanged()));
    connect(m_document->colorHistoryModel(), SIGNAL(sigReset()), this, SLOT(colorHistoryChanged()));
}

KisColorSelectorBase *KisColorHistory::createPopup() const
{
    KisColorHistory *ret = new KisColorHistory();
    ret->setCanvas(m_canvas);
    ret->setColors(colors());
    return ret;
}

void KisColorHistory::addColorToHistory(const KoColor &color)
{
    // don't add color in erase mode. See https://bugs.kde.org/show_bug.cgi?id=298940
    if (m_resourceProvider && m_resourceProvider->currentCompositeOp() == COMPOSITE_ERASE) return;

    colorHistoryModel()->addColor(color);

    updateColorHistory(colorHistoryModel()->colorList());
    s_color_history_change_notifier->notifyColorHistoryChanged(colorHistoryModel()->colorList());
}

void KisColorHistory::clearColorHistory()
{
    QList<KoColor> empty;
    colorHistoryModel()->clear();
    updateColorHistory(empty);
    s_color_history_change_notifier->notifyColorHistoryChanged(empty);
}

KisUniqueColorSet *KisColorHistory::colorHistoryModel()
{
    if (m_history_per_document && m_document && m_document->colorHistoryModel()) {
        return m_document->colorHistoryModel();
    } else if (m_resourceProvider && m_resourceProvider->colorHistoryModel()) {
        return m_resourceProvider->colorHistoryModel();
    } else {
        return m_fallbackColorHistoryModel.get();
    }
}

void KisColorHistory::updateColorHistory(const QList<KoColor> &history)
{
    Q_UNUSED(history);
    setColors(colorHistoryModel()->colorList());
}

void KisColorHistory::updateUserSettings()
{

    KisConfig config(true);
    m_history_per_document = config.colorHistoryPerDocument();
    updateColorHistory(colorHistoryModel()->colorList()); // Show with respect to the current strategy
}

void KisColorHistory::colorHistoryChanged()
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(sender() != this);
    setColors(colorHistoryModel()->colorList());
}

void KisColorHistoryNotifier::notifyColorHistoryChanged(const QList<KoColor> &history)
{
    Q_EMIT colorHistoryChanged(history);
}
