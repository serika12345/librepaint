/*
 * This file is part of the KDE Libraries
 * SPDX-FileCopyrightText: 1999-2000 Espen Sand (espen@kde.org)
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 */

// I (espen) prefer that header files are included alphabetically

#include "khelpmenu.h"
#include <QTimer>
#include <QAction>
#include <QMenu>
#include <QWidget>
#include <QWhatsThis>

#include "kswitchlanguagedialog_p.h"

#include <klocalizedstring.h>
#include <kstandardaction.h>

using namespace KDEPrivate;

class KisKHelpMenuPrivate
{
public:
    ~KisKHelpMenuPrivate()
    {
        delete mMenu;
        delete mSwitchApplicationLanguage;
    }

    void createActions(KisKHelpMenu *q);

    QMenu *mMenu {nullptr};
    KisKSwitchLanguageDialog *mSwitchApplicationLanguage {nullptr};
    // TODO evaluate if we use static_cast<QWidget*>(parent()) instead of mParent to win that bit of memory
    QWidget *mParent {nullptr};
    bool mShowWhatsThis {false};
    bool mActionsCreated {false};

    QAction *mWhatsThisAction {nullptr};
    QAction *mSwitchApplicationLanguageAction {nullptr};
    QAction *mAboutAppAction {nullptr};
};

KisKHelpMenu::KisKHelpMenu(QWidget *parent, const QString &,
                     bool showWhatsThis)
    : QObject(parent), d(new KisKHelpMenuPrivate)
{
    d->mShowWhatsThis = showWhatsThis;
    d->mParent = parent;
    d->createActions(this);
}

KisKHelpMenu::KisKHelpMenu(QWidget *parent, const KAboutData &,
                     bool showWhatsThis)
    : QObject(parent), d(new KisKHelpMenuPrivate)
{
    d->mShowWhatsThis = showWhatsThis;
    d->mParent = parent;
    d->createActions(this);
}

KisKHelpMenu::~KisKHelpMenu()
{
    delete d;
}

void KisKHelpMenuPrivate::createActions(KisKHelpMenu *q)
{
    if (mActionsCreated) {
        return;
    }
    mActionsCreated = true;
    // LibrePaint does not expose upstream handbook or bug-report routes.
    if (mShowWhatsThis) {
        mWhatsThisAction = KStandardAction::whatsThis(q, SLOT(contextHelpActivated()), q);
    }

    mSwitchApplicationLanguageAction = KStandardAction::create(KStandardAction::SwitchApplicationLanguage, q, SLOT(switchApplicationLanguage()), q);
    mAboutAppAction = KStandardAction::aboutApp(q, SLOT(aboutApplication()), q);
    // LibrePaint does not expose the upstream KDE promotional dialog.
}

// Used by callers that do not use XMLGUI.
QMenu *KisKHelpMenu::menu()
{
    if (!d->mMenu) {
        d->mMenu = new QMenu();
        connect(d->mMenu, SIGNAL(destroyed()), this, SLOT(menuDestroyed()));

        d->mMenu->setTitle(i18n("&Help"));

        d->createActions(this);

        bool need_separator = false;
        if (d->mWhatsThisAction) {
            d->mMenu->addAction(d->mWhatsThisAction);
            need_separator = true;
        }

        if (d->mSwitchApplicationLanguageAction) {
            if (need_separator) {
                d->mMenu->addSeparator();
            }
            d->mMenu->addAction(d->mSwitchApplicationLanguageAction);
            need_separator = true;
        }

        if (need_separator) {
            d->mMenu->addSeparator();
        }

        if (d->mAboutAppAction) {
            d->mMenu->addAction(d->mAboutAppAction);
        }
    }

    return d->mMenu;
}

QAction *KisKHelpMenu::action(MenuId id) const
{
    switch (id) {
    case menuHelpContents:
        return nullptr;

    case menuWhatsThis:
        return d->mWhatsThisAction;

    case menuReportBug:
        return nullptr;

    case menuSwitchLanguage:
        return d->mSwitchApplicationLanguageAction;

    case menuAboutApp:
        return d->mAboutAppAction;

    case menuAboutKDE:
        return nullptr;
    }

    return nullptr;
}

void KisKHelpMenu::appHelpActivated()
{
    // No LibrePaint handbook endpoint has been configured.
}

void KisKHelpMenu::aboutApplication()
{
    if (receivers(SIGNAL(showAboutApplication())) > 0) {
        Q_EMIT showAboutApplication();
    }
}

void KisKHelpMenu::aboutKDE()
{
    // LibrePaint does not expose the upstream KDE promotional dialog.
}

void KisKHelpMenu::reportBug()
{
    // No LibrePaint bug-report endpoint has been configured.
}

void KisKHelpMenu::switchApplicationLanguage()
{
    if (!d->mSwitchApplicationLanguage) {
        d->mSwitchApplicationLanguage = new KisKSwitchLanguageDialog(d->mParent);
        connect(d->mSwitchApplicationLanguage, SIGNAL(finished(int)), this, SLOT(dialogFinished()));
    }
    d->mSwitchApplicationLanguage->show();
}

void KisKHelpMenu::dialogFinished()
{
    QTimer::singleShot(0, this, SLOT(timerExpired()));
}

void KisKHelpMenu::timerExpired()
{
    if (d->mSwitchApplicationLanguage && !d->mSwitchApplicationLanguage->isVisible()) {
        delete d->mSwitchApplicationLanguage; d->mSwitchApplicationLanguage = 0;
    }
}

void KisKHelpMenu::menuDestroyed()
{
    d->mMenu = 0;
}

void KisKHelpMenu::contextHelpActivated()
{
    QWhatsThis::enterWhatsThisMode();
}
