/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2018 Scott Petrovic <scottpetrovic@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISWELCOMEPAGEWIDGET_H
#define KISWELCOMEPAGEWIDGET_H

#include "kritaui_export.h"
#include "workspace/KisViewManager.h"
#include <KisKineticScroller.h>

#include <QWidget>
#include "ui_KisWelcomePage.h"

class QAction;
class KisMainWindow;

/// A widget for displaying if no documents are open. This will display in the MDI area
class KRITAUI_EXPORT KisWelcomePageWidget : public QWidget, public Ui::KisWelcomePage
{
    Q_OBJECT

    public:
    explicit KisWelcomePageWidget(QWidget *parent);
    ~KisWelcomePageWidget() override;

    void setMainWindow(KisMainWindow* m_mainWindow);

public Q_SLOTS:
    /// if a document is placed over this area, a dotted line will appear as an indicator
    /// that it is a droppable area. KisMainwindow is what triggers this
    void showDropAreaIndicator(bool show);

    void slotUpdateThemeColors();

private Q_SLOTS:
    void slotNewFileClicked();
    void slotOpenFileClicked();
    void slotPaste();

    void recentDocumentClicked(QModelIndex index);
    void slotRecentDocContextMenuRequest(const QPoint &pos);

    /**
     * Once all files in the recent documents model are checked, cleanup the UI if the model is empty
     */
    void slotRecentFilesModelIsUpToDate();

    void slotScrollerStateChanged(QScroller::State state){ KisKineticScroller::updateCursor(this, state); }

    bool isDevelopmentBuild();

protected:

    // QWidget overrides
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent * event) override;
    void dragMoveEvent(QDragMoveEvent * event) override;
    void dragLeaveEvent(QDragLeaveEvent * event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;


private:
    void showDevVersionHighlight();

    static void updateShortcutLink(QToolButton *button, QLabel *label, QAction *action);

    KisMainWindow *m_mainWindow {nullptr};

    // keeping track of link colors with theme change
    QColor textColor;
    QColor backgroundColor;
    QColor blendedColor;
    QString blendedStyle;

};

#endif // KISWELCOMEPAGEWIDGET_H
