/*
 * This file is part of the KDE Libraries
 * SPDX-FileCopyrightText: 1999-2000 Espen Sand (espen@kde.org)
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 */

#ifndef KHELPMENU_H
#define KHELPMENU_H

#include <kritawidgetutils_export.h>

#include <QObject>
#include <QString>

class QMenu;
class QWidget;
class QAction;

class KAboutData;
class KisKHelpMenuPrivate;

/**
 * @short Compatibility help menu for the actions available in LibrePaint.
 *
 * This class retains the existing XMLGUI API while providing the available
 * context-help, language, and application-about actions. The upstream
 * handbook, bug-report, and About KDE routes are intentionally unavailable.
 *
 * This class is used in KisKMainWindow so
 * normally you don't need to use this class yourself. However, if you
 * need the help menu or any of its dialog boxes in your code that is
 * not subclassed from KisKMainWindow you should use this class.
 *
 * The usage is simple:
 *
 * \code
 * mHelpMenu = new KisKHelpMenu( this, <someText> );
 * kmenubar->addMenu(mHelpMenu->menu() );
 * \endcode
 *
 * IMPORTANT:
 * The first time you use KisKHelpMenu::menu(), a QMenu object is
 * allocated. Only one object is created by the class so if you call
 * KisKHelpMenu::menu() twice or more, the same pointer is returned. The class
 * will destroy the popupmenu in the destructor so do not delete this
 * pointer yourself.
 *
 * The KisKHelpMenu object will be deleted when its parent is destroyed but you
 * can delete it yourself if you want. The code below will always work.
 *
 * \code
 * MyClass::~MyClass()
 * {
 *   delete mHelpMenu;
 * }
 * \endcode
 *
 *
 * Using your own "about application" dialog box:
 *
 * The standard "about application" dialog box is quite simple. If you
 * need a dialog box with more functionality you must design that one
 * yourself. When you want to display the dialog, you simply need to
 * connect the help menu signal showAboutApplication() to your slot.
 *
 * \code
 * void MyClass::myFunc()
 * {
 *   ..
 *   KisKHelpMenu *helpMenu = new KisKHelpMenu( this );
 *   connect( helpMenu, SIGNAL(showAboutApplication()),
 *          this, SLOT(myDialogSlot()));
 *   ..
 * }
 *
 * void MyClass::myDialogSlot()
 * {
 *   <activate your custom dialog>
 * }
 * \endcode
 *
 * \image html khelpmenu.png "KDE Help Menu"
 *
 * @verbatim
   [KDE Action Restrictions][$i]
   actions/help_contents=false
   actions/help_whats_this=false
   actions/help_report_bug=false
   actions/switch_application_language=false
   actions/help_about_app=false
   actions/help_about_kde=false
   @endverbatim
 *
 * @author Espen Sand (espen@kde.org)
 */

class KRITAWIDGETUTILS_EXPORT KisKHelpMenu : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * @param parent The parent of the dialog boxes. The boxes are modeless
     *        and will be centered with respect to the parent.
     * @param aboutAppText Retained for source compatibility with the original
     *        API. LibrePaint supplies its application dialog elsewhere.
     * @param showWhatsThis Decides whether a "Whats this" entry will be
     *        added to the dialog.
     *
     */
    explicit KisKHelpMenu(QWidget *parent = 0, const QString &aboutAppText = QString(),
                       bool showWhatsThis = true);

    /**
     * Constructor.
     *
     * This alternative constructor retains the original API for callers that
     * supply application metadata.
     *
     * @param parent The parent of the dialog boxes. The boxes are modeless
     *        and will be centered with respect to the parent.
     * @param aboutData Retained for source compatibility with the original
     *        API. LibrePaint supplies its application dialog elsewhere.
     * @param showWhatsThis Decides whether a "Whats this" entry will be
     *        added to the dialog.
     */
    KisKHelpMenu(QWidget *parent, const KAboutData &aboutData,
              bool showWhatsThis = true);

    /**
     * Destructor
     *
     * Destroys the language dialog and the menu pointer returned by menu().
     */
    ~KisKHelpMenu() override;

    /**
     * Returns a popup menu you can use in the menu bar or where you
     * need it.
     *
     * The returned menu is configured with a title and the available menu
     * entries. Therefore adding the returned pointer to your menu is enough
     * to have access to the help menu.
     *
     * Note: This method will only create one instance of the menu. If
     * you call this method twice or more the same pointer is returned.
     */
    QMenu *menu();

    enum MenuId {
        menuHelpContents = 0,
        menuWhatsThis = 1,
        menuAboutApp = 2,
        menuAboutKDE = 3,
        menuReportBug = 4,
        menuSwitchLanguage = 5
    };

    /**
     * Returns the QAction associated with the given parameter. Unavailable
     * compatibility actions return nullptr.
     *
     * @param id The id of the action of which you want to get QAction *
     */
    QAction *action(MenuId id) const;

public Q_SLOTS:
    /**
     * Compatibility no-op. LibrePaint has no configured handbook endpoint.
     */
    void appHelpActivated();

    /**
     * Activates What's This help for the application.
     */
    void contextHelpActivated();

    /**
     * Emits showAboutApplication() when an application-specific handler is
     * connected.
     */
    void aboutApplication();

    /**
     * Compatibility no-op. LibrePaint does not expose an About KDE dialog.
     */
    void aboutKDE();

    /**
     * Compatibility no-op. LibrePaint has no configured bug-report endpoint.
     */
    void reportBug();

    /**
     * Opens the changing default application language dialog box.
     */
    void switchApplicationLanguage();

private Q_SLOTS:
    /**
     * Connected to the menu pointer (if created) to detect a delete
     * operation on the pointer. You should not delete the pointer in your
     * code yourself. Let the KisKHelpMenu destructor do the job.
     */
    void menuDestroyed();

    /**
     * Connected to the remaining modeless dialogs to detect when they finish.
     */
    void dialogFinished();

    /**
     * Deletes modeless dialogs that have finished and are no longer visible.
     * This slot is activated by a one-shot timer from dialogFinished().
     */
    void timerExpired();

Q_SIGNALS:
    /**
     * This signal is emitted from aboutApplication() when an
     * application-specific handler is connected.
     */
    void showAboutApplication();

private:
    KisKHelpMenuPrivate *const d;
};

#endif
