/*
 *  SPDX-FileCopyrightText: 2007 Cyrille Berger <cberger@cberger.net>
 *  SPDX-FileCopyrightText: 2011 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2011 Srikanth Tiyyagura <srikanth.tulasiram@gmail.com>
 *  SPDX-FileCopyrightText: 2022 L. E. Segovia <amy@amyspark.me>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_color_space_selector.h"
#include "kis_advanced_color_space_selector.h"
#include "kis_color_space_profile_value_p.h"


#include <QUrl>

#include <KoFileDialog.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorSpaceEngine.h>
#include <KoID.h>

#include <KoConfig.h>
#include <algorithm>
#include <kis_icon.h>

#include <QStandardPaths>
#include <QPointer>
#include <QScopeGuard>
#include <QSignalBlocker>

#include <KoResourcePaths.h>


#include <kis_debug.h>
#include <QtGlobal>
#include <qcontainerfwd.h>
#include <qdialog.h>
#include <qdir.h>
#include <qlist.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qwidget.h>

#include "ui_wdgcolorspaceselector.h"

struct KisColorSpaceSelector::Private {
    Ui_WdgColorSpaceSelector* colorSpaceSelector;
    QString knsrcFile;
    bool profileValid = false;
    QString defaultsuffix;
    bool profileSetManually;
    KoID previousModel;

    KisColorSpaceProfileValue selectedProfile() const
    {
        return colorSpaceSelector->cmbProfile->currentData().value<KisColorSpaceProfileValue>();
    }

    void selectProfile(const KisColorSpaceProfileValue &profile)
    {
        auto *combo = colorSpaceSelector->cmbProfile;
        for (int i = 0; i < combo->count(); ++i) {
            if (kisSameColorSpaceProfile(combo->itemData(i).value<KisColorSpaceProfileValue>(), profile)) {
                combo->setCurrentIndex(i);
                return;
            }
        }
        combo->addSqueezedItem(i18nc("An unlisted profile retained by the color space selector", "%1 (current profile)", profile->name()),
                               QVariant::fromValue(profile));
        combo->setCurrentIndex(combo->count() - 1);
    }
};

KisColorSpaceSelector::KisColorSpaceSelector(QWidget *parent)
    : QWidget(parent)
    , d(new Private)
{
    setObjectName("KisColorSpaceSelector");
    d->colorSpaceSelector = new Ui_WdgColorSpaceSelector;
    d->colorSpaceSelector->setupUi(this);
    d->colorSpaceSelector->cmbColorModels->setIDList(KoColorSpaceRegistry::instance()->colorModelsList(KoColorSpaceRegistry::OnlyUserVisible));
    fillCmbDepths(d->colorSpaceSelector->cmbColorModels->currentItem());

    d->colorSpaceSelector->bnInstallProfile->setIcon(koIcon("document-import-16"));
    d->colorSpaceSelector->bnInstallProfile->setToolTip(i18n("Import profile"));

    connect(d->colorSpaceSelector->cmbColorModels, SIGNAL(activated(KoID)),
            this, SLOT(slotModelsComboBoxActivated(KoID)));
    connect(d->colorSpaceSelector->cmbColorDepth, SIGNAL(activated(KoID)),
            this, SLOT(slotDepthsComboBoxActivated()));
    connect(d->colorSpaceSelector->cmbProfile, SIGNAL(textActivated(QString)),
            this, SLOT(slotProfilesComboBoxActivated()));
    connect(d->colorSpaceSelector->bnInstallProfile, SIGNAL(clicked()),
            this, SLOT(installProfile()));

    d->defaultsuffix = " "+i18nc("This is appended to the color profile which is the default for the given colorspace and bit-depth","(Default)");
    d->profileSetManually = false;
    d->previousModel = d->colorSpaceSelector->cmbColorModels->currentItem();

    connect(d->colorSpaceSelector->bnAdvanced, SIGNAL(clicked()), this,  SLOT(slotOpenAdvancedSelector()));

    fillCmbProfiles();
}

KisColorSpaceSelector::~KisColorSpaceSelector()
{
    delete d->colorSpaceSelector;
    delete d;
}


void KisColorSpaceSelector::fillCmbProfiles()
{
    const auto currentProfile = d->selectedProfile();
    auto *registry = KoColorSpaceRegistry::instance();
    const QString colorSpaceId = registry->colorSpaceId(d->colorSpaceSelector->cmbColorModels->currentItem(), d->colorSpaceSelector->cmbColorDepth->currentItem());
    const QString defaultProfileName = registry->defaultProfileForColorSpace(colorSpaceId);
    QList<KisColorSpaceProfileValue> profiles;
    for (const auto *profile : registry->profilesFor(colorSpaceId)) {
        profiles.append(KisColorSpaceProfileValue(profile->clone()));
    }
    std::sort(profiles.begin(), profiles.end(), [](const auto &left, const auto &right) {
        return left->name() < right->name();
    });
    {
        auto *combo = d->colorSpaceSelector->cmbProfile;
        const QSignalBlocker blocker(combo);
        combo->clear();
        int defaultIndex = 0;
        for (const auto &profile : profiles) {
            const bool isDefault = profile->name() == defaultProfileName;
            if (isDefault) defaultIndex = combo->count();
            combo->addSqueezedItem(profile->name() + (isDefault ? d->defaultsuffix : QString()),
                                   QVariant::fromValue(profile));
        }
        if (d->profileSetManually && currentProfile && registry->profileIsCompatible(currentProfile.data(), colorSpaceId)) {
            d->selectProfile(currentProfile);
        } else {
            combo->setCurrentIndex(profiles.isEmpty() ? -1 : defaultIndex);
        }
    }
    colorSpaceChanged();
}

void KisColorSpaceSelector::fillCmbDepths(const KoID& id)
{
    KoID activeDepth = d->colorSpaceSelector->cmbColorDepth->currentItem();
    d->colorSpaceSelector->cmbColorDepth->clear();
    QList<KoID> depths = KoColorSpaceRegistry::instance()->colorDepthList(id, KoColorSpaceRegistry::OnlyUserVisible);
    d->colorSpaceSelector->cmbColorDepth->setIDList(depths, false);
    if (depths.contains(activeDepth)) {
        d->colorSpaceSelector->cmbColorDepth->setCurrent(activeDepth);
    }
}



const KoColorSpace* KisColorSpaceSelector::currentColorSpace()
{
    const auto profile = d->selectedProfile();
    if (!profile) return nullptr;
    return KoColorSpaceRegistry::instance()->colorSpace(
        d->colorSpaceSelector->cmbColorModels->currentItem().id(),
        d->colorSpaceSelector->cmbColorDepth->currentItem().id(), profile.data());
}

void KisColorSpaceSelector::setCurrentColorModel(const KoID& id)
{
    d->colorSpaceSelector->cmbColorModels->setCurrent(id);
    d->previousModel = id;
    fillCmbDepths(id);
}

void KisColorSpaceSelector::setCurrentColorDepth(const KoID& id)
{
    d->colorSpaceSelector->cmbColorDepth->setCurrent(id);
    if (!d->profileSetManually) {
        fillCmbProfiles();
    }
}

void KisColorSpaceSelector::setCurrentProfile(const QString& name)
{
    auto *registry = KoColorSpaceRegistry::instance();
    const auto *profile = registry->profileByName(name);
    const QString colorSpaceId = registry->colorSpaceId(d->colorSpaceSelector->cmbColorModels->currentItem(), d->colorSpaceSelector->cmbColorDepth->currentItem());
    if (profile && registry->profileIsCompatible(profile, colorSpaceId)) {
        d->selectProfile(KisColorSpaceProfileValue(profile->clone()));
    }
}

void KisColorSpaceSelector::setCurrentColorSpace(const KoColorSpace* colorSpace)
{
    if (!colorSpace) {
        return;
    }
    const KoID model = colorSpace->colorModelId();
    const KoID depth = colorSpace->colorDepthId();
    const KisColorSpaceProfileValue profile(colorSpace->profile()->clone());
    {
        const QSignalBlocker blocker(this);
        setCurrentColorModel(model);
        d->colorSpaceSelector->cmbColorDepth->setCurrent(depth);
        fillCmbProfiles();
        d->selectProfile(profile);
    }
    colorSpaceChanged();
}

void KisColorSpaceSelector::showColorBrowserButton(bool showButton) {
    d->colorSpaceSelector->bnAdvanced->setVisible(showButton);
}

void KisColorSpaceSelector::showDepth(bool showDepth)
{
    d->colorSpaceSelector->cmbColorDepth->setVisible(showDepth);
    d->colorSpaceSelector->lblColorDepths->setVisible(showDepth);
}

void KisColorSpaceSelector::colorSpaceChanged()
{
    const auto *selected = currentColorSpace();
    const bool valid = selected != nullptr;
    d->profileValid = valid;
    const QPointer<KisColorSpaceSelector> guard(this);
    emit(selectionChanged(valid));
    if (guard && valid && currentColorSpace() == selected) {
        Q_EMIT colorSpaceChanged(selected);
    }
}

void KisColorSpaceSelector::installProfile()
{
    QStringList mime;
    KoFileDialog dialog(this, KoFileDialog::OpenFiles, "OpenDocumentICC");
    dialog.setCaption(i18n("Install Color Profiles"));
    dialog.setDefaultDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    dialog.setMimeTypeFilters(QStringList() << "application/vnd.iccprofile", "application/vnd.iccprofile");

    QStringList profileNames = dialog.filenames();

    KoColorSpaceEngine *iccEngine = KoColorSpaceEngineRegistry::instance()->get("icc");
    Q_ASSERT(iccEngine);

    QString saveLocation = KoResourcePaths::saveLocation("icc_profiles");

    Q_FOREACH (const QString &profileName, profileNames) {
        QUrl file(profileName);
        if (!QFile::copy(profileName, saveLocation + file.fileName())) {
            dbgKrita << "Could not install profile!";
            return;
        }
        iccEngine->addProfile(saveLocation + file.fileName());

    }

    fillCmbProfiles();
}

void KisColorSpaceSelector::slotOpenAdvancedSelector()
{
    const QPointer<KisColorSpaceSelector> guard(this);
    QPointer<KisAdvancedColorSpaceSelector> advancedSelector = new KisAdvancedColorSpaceSelector(this, i18n("Select a Color Space"));
    auto cleanup = qScopeGuard([&] { delete advancedSelector.data(); });
    advancedSelector->setModal(true);
    if (const auto *selected = currentColorSpace()) {
        advancedSelector->setCurrentColorSpace(selected);
    }
    connect(advancedSelector, &KisAdvancedColorSpaceSelector::selectionChanged, this, &KisColorSpaceSelector::slotProfileValid);
    const auto result = advancedSelector->exec();
    if (!guard || !advancedSelector) return;
    if (result == QDialog::Accepted && d->profileValid) {
        const auto *selected = advancedSelector->currentColorSpace();
        delete advancedSelector.data();
        d->profileSetManually = true;
        setCurrentColorSpace(selected);
    }
}

void KisColorSpaceSelector::slotProfileValid(bool valid)
{
    d->profileValid = valid;
}


void KisColorSpaceSelector::slotModelsComboBoxActivated(const KoID& id)
{
    if (d->previousModel != id) {
        d->previousModel = id;
        d->profileSetManually = false;
        fillCmbDepths(id);
        fillCmbProfiles();
    }
}

void KisColorSpaceSelector::slotDepthsComboBoxActivated()
{
    fillCmbProfiles();
}


void KisColorSpaceSelector::slotProfilesComboBoxActivated()
{
    d->profileSetManually = true;
    colorSpaceChanged();
}

