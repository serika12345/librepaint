/*
 * This file is part of the KDE project
 * SPDX-FileCopyrightText: 2013 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_input_configuration_page.h"
#include "ui_kis_input_configuration_page.h"

#include <QDebug>
#include <QDir>
#include <QMap>

#include "input/ui/kis_input_profile_manager.h"
#include "input/kis_input_profile.h"
#include "input/kis_shortcut_configuration.h"
#include "input/ui/kis_abstract_input_action.h"
#include "input/ui/kis_shortcut_configuration_text.h"
#include "kis_edit_profiles_dialog.h"
#include "kis_input_profile_model.h"
#include "kis_input_configuration_page_item.h"
#include <KoResourcePaths.h>
#include <kis_config.h>
#include <kis_signals_blocker.h>


#include "kis_icon_utils.h"

struct KisInputConfigurationPage::Private {
    QMap<KisAbstractInputAction *, KisInputConfigurationPageItem*> actionInputConfigurationMap;
};

KisInputConfigurationPage::KisInputConfigurationPage(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_d(new Private)
{
    ui = new Ui::KisInputConfigurationPage;
    ui->setupUi(this);

    ui->profileComboBox->setModel(new KisInputProfileModel(ui->profileComboBox));
    updateSelectedProfile();
    connect(ui->profileComboBox, SIGNAL(currentTextChanged(QString)), SLOT(changeCurrentProfile(QString)));

    ui->editProfilesButton->setIcon(KisIconUtils::loadIcon("document-edit"));

    connect(ui->editProfilesButton, SIGNAL(clicked(bool)), SLOT(editProfilesButtonClicked()));
    connect(KisInputProfileManager::instance(), SIGNAL(profilesChanged()), SLOT(updateSelectedProfile()));
    connect(KisInputProfileManager::instance(),
            &KisInputProfileManager::currentProfileChanged,
            this,
            &KisInputConfigurationPage::checkForConflicts);

    QList<KisAbstractInputAction *> actions = KisInputProfileManager::instance()->actions();
    Q_FOREACH(KisAbstractInputAction * action, actions) {
        KisInputConfigurationPageItem *item = new KisInputConfigurationPageItem(this);
        item->setAction(action);
        ui->configurationItemsArea->addWidget(item);
        m_d->actionInputConfigurationMap.insert(action, item);
        connect(item,
                &KisInputConfigurationPageItem::inputConfigurationChanged,
                this,
                &KisInputConfigurationPage::checkForConflicts,
                Qt::UniqueConnection);
    }
    checkForConflicts();
    ui->configurationItemsArea->addStretch(20); // ensures listed input are on top

    QScroller *scroller = KisKineticScroller::createPreconfiguredScroller(ui->scrollArea);
    if (scroller) {
        connect(scroller, SIGNAL(stateChanged(QScroller::State)),
                this, SLOT(slotScrollerStateChanged(QScroller::State)));
    }
}

KisInputConfigurationPage::~KisInputConfigurationPage() = default;

void KisInputConfigurationPage::saveChanges()
{
    KisInputProfileManager::instance()->saveProfiles();
}

void KisInputConfigurationPage::revertChanges()
{
    KisInputProfileManager::instance()->loadProfiles();
}

void KisInputConfigurationPage::checkForConflicts()
{
    // reset all of them first.
    Q_FOREACH (KisInputConfigurationPageItem *pageItem, m_d->actionInputConfigurationMap.values()) {
        pageItem->setWarningEnabled(false);
    }
    const QList<KisShortcutConfiguration *> conflictingShortcuts =
        KisInputProfileManager::instance()->getConflictingShortcuts(
            KisInputProfileManager::instance()->currentProfile());

    if (conflictingShortcuts.isEmpty()) {
        return;
    }

    struct ShortcutInfo {
        QVector<KisShortcutConfiguration *> shortcutObjects;
        QStringList actionTexts;
    };
    QMap<QString, ShortcutInfo> conflictingShortcutsMap;

    Q_FOREACH (KisShortcutConfiguration *shortcut, conflictingShortcuts) {
        KisAbstractInputAction *action =
            KisInputProfileManager::instance()->action(shortcut->actionId());
        if (action) {
            if (m_d->actionInputConfigurationMap.contains(action)) {
                ShortcutInfo &shortcutInfo =
                    conflictingShortcutsMap[KisShortcutConfigurationText::inputText(*shortcut)];
                shortcutInfo.shortcutObjects.append(shortcut);
                const QString subActionName = action->shortcutIndexes().key(shortcut->mode());
                shortcutInfo.actionTexts.append(action->name() + " → " + subActionName);
            } else {
                qWarning() << "KisInputConfigurationPageItem does not exist for the specified action:"
                           << action->name();
            }
        } else {
            qWarning() << "Input action is unavailable for shortcut:" << shortcut->actionId();
        }
    }

    QMap<KisAbstractInputAction *, QSet<QString>> infoTexts;
    Q_FOREACH (const ShortcutInfo &shortcutInfo, conflictingShortcutsMap) {
        const QString text = "<b>" + KisShortcutConfigurationText::inputText(*shortcutInfo.shortcutObjects.first()) + "</b>" + "<br>&nbsp;&nbsp;"
            + shortcutInfo.actionTexts.join("<br>&nbsp;&nbsp;");
        Q_FOREACH (KisShortcutConfiguration *shortcut, shortcutInfo.shortcutObjects) {
            KisAbstractInputAction *action =
                KisInputProfileManager::instance()->action(shortcut->actionId());
            if (action) {
                infoTexts[action] << text;
            }
        }
    }

    QMapIterator<KisAbstractInputAction *, QSet<QString>> it(infoTexts);
    while (it.hasNext()) {
        it.next();
        QSetIterator<QString> it2(it.value());
        if (!it2.hasNext()) {
            continue;
        }
        QString text = "<ul> <li>" + it2.next() + "</li>";
        while (it2.hasNext()) {
            text += "<li>" + it2.next() + "</li>";
        }
        text += "</ul>";
        m_d->actionInputConfigurationMap[it.key()]->setWarningEnabled(true, text);
    }
}

void KisInputConfigurationPage::setDefaults()
{
    KisSignalsBlocker(ui->profileComboBox, KisInputProfileManager::instance());

    QDir profileDir(KoResourcePaths::saveLocation("data", "input/", false));
    KisConfig(false).setCurrentInputProfile("LibrePaint Default");

    if (profileDir.exists()) {
        QStringList entries = profileDir.entryList(QStringList() << "*.profile", QDir::Files | QDir::NoDotAndDotDot);
        Q_FOREACH(const QString & file, entries) {
            profileDir.remove(file);
        }

        KisInputProfileManager::instance()->loadProfiles();
        changeCurrentProfile("LibrePaint Default");
        KisInputProfileManager::instance()->setCurrentProfile(KisInputProfileManager::instance()->profile("LibrePaint Default"));
    }
    ui->profileComboBox->setCurrentItem("LibrePaint Default");
}

void KisInputConfigurationPage::editProfilesButtonClicked()
{
    KisEditProfilesDialog dialog;
    dialog.exec();
}

void KisInputConfigurationPage::updateSelectedProfile()
{
    if (KisInputProfileManager::instance()->currentProfile()) {
        ui->profileComboBox->setCurrentItem(KisInputProfileManager::instance()->currentProfile()->name());
    }
}

void KisInputConfigurationPage::changeCurrentProfile(const QString &newProfile)
{
    KisInputProfileManager::instance()->setCurrentProfile(KisInputProfileManager::instance()->profile(newProfile));
}
