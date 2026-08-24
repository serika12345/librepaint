
/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2018 Scott Petrovic <scottpetrovic@gmail.com>
 * SPDX-FileCopyrightText: 2021 L. E. Segovia <amy@amyspark.me>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "workspace/KisWelcomePageWidget.h"
#include "KisRecentDocumentsModelWrapper.h"
#include <QMimeData>
#include <QTemporaryFile>

#include "KisRemoteFileFetcher.h"
#include "application/kis_action.h"
#include "application/kis_action_manager.h"
#include "dialogs/KisDlgCreateNewDocument.h"
#include <QMenu>
#include <QScrollBar>

#include "kis_clipboard.h"
#include "kis_icon_utils.h"
#include <kis_painting_tweaks.h>
#include <KisKineticScroller.h>
#include "workspace/KisMainWindow.h"

#include <QCoreApplication>
#include <kis_debug.h>

#include <klocalizedstring.h>
#include <KritaVersionWrapper.h>


KisWelcomePageWidget::KisWelcomePageWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    devBuildIcon->setIcon(KisIconUtils::loadIcon("warning"));
    devBuildLabel->setVisible(false);

    // Recent docs...
    recentDocumentsListView->setDragEnabled(false);
    recentDocumentsListView->viewport()->setAutoFillBackground(false);
    recentDocumentsListView->setSpacing(2);
    recentDocumentsListView->installEventFilter(this);
    recentDocumentsListView->setViewMode(QListView::IconMode);
    recentDocumentsListView->setSelectionMode(QAbstractItemView::NoSelection);

    recentDocumentsListView->setIconSize(QSize(KisRecentDocumentsModelWrapper::ICON_SIZE_LENGTH, KisRecentDocumentsModelWrapper::ICON_SIZE_LENGTH));
    recentDocumentsListView->setVerticalScrollMode(QListView::ScrollPerPixel);
    recentDocumentsListView->verticalScrollBar()->setSingleStep(50);
    {
        QScroller* scroller = KisKineticScroller::createPreconfiguredScroller(recentDocumentsListView);
        if (scroller) {
            connect(scroller, SIGNAL(stateChanged(QScroller::State)), this, SLOT(slotScrollerStateChanged(QScroller::State)));
        }
    }
    recentDocumentsListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(recentDocumentsListView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(slotRecentDocContextMenuRequest(QPoint)));

    // Drop area..
    setAcceptDrops(true);
}

KisWelcomePageWidget::~KisWelcomePageWidget()
{
}

void KisWelcomePageWidget::setMainWindow(KisMainWindow* mainWin)
{
    if (mainWin) {
        m_mainWindow = mainWin;

        // set the shortcut links from actions (only if a shortcut exists)
        KisActionManager *actionManager = mainWin->viewManager()->actionManager();
        updateShortcutLink(newFileLink, newFileLinkShortcut, actionManager->actionByName(QStringLiteral("file_new")));
        updateShortcutLink(openFileLink, openFileShortcut, actionManager->actionByName(QStringLiteral("file_open")));
        connect(recentDocumentsListView, SIGNAL(clicked(QModelIndex)), this, SLOT(recentDocumentClicked(QModelIndex)));
        // we need the view manager to actually call actions, so don't create the connections
        // until after the view manager is set
        connect(newFileLink, SIGNAL(clicked(bool)), this, SLOT(slotNewFileClicked()));
        connect(openFileLink, SIGNAL(clicked(bool)), this, SLOT(slotOpenFileClicked()));
        connect(clearRecentFilesLink, SIGNAL(clicked(bool)), mainWin, SLOT(clearRecentFiles()));

        KisAction *pasteAction = mainWin->viewManager()->actionManager()->actionByName("edit_paste");
        connect(pasteAction, SIGNAL(triggered()), this, SLOT(slotPaste()));

        slotUpdateThemeColors();

        KisRecentDocumentsModelWrapper *recentFilesModel = KisRecentDocumentsModelWrapper::instance();
        connect(recentFilesModel, SIGNAL(sigModelIsUpToDate()), this, SLOT(slotRecentFilesModelIsUpToDate()));
        recentDocumentsListView->setModel(&recentFilesModel->model());
        slotRecentFilesModelIsUpToDate();
    }
}


void KisWelcomePageWidget::showDropAreaIndicator(bool show)
{
    if (!show) {
        QString dropFrameStyle = QStringLiteral("QFrame#dropAreaIndicator { border: 2px solid transparent }");
        dropFrameBorder->setStyleSheet(dropFrameStyle);
    } else {
        QColor textColor = qApp->palette().color(QPalette::Text);
        QColor backgroundColor = qApp->palette().color(QPalette::Window);
        QColor blendedColor = KisPaintingTweaks::blendColors(textColor, backgroundColor, 0.8);

        // QColor.name() turns it into a hex/web format
        QString dropFrameStyle = QString("QFrame#dropAreaIndicator { border: 2px dotted ").append(blendedColor.name()).append(" }") ;
        dropFrameBorder->setStyleSheet(dropFrameStyle);
    }
}

void KisWelcomePageWidget::slotUpdateThemeColors()
{
    textColor = qApp->palette().color(QPalette::Text);
    backgroundColor = qApp->palette().color(QPalette::Window);

    // make the welcome screen labels a subtle color so it doesn't clash with the main UI elements
    blendedColor = KisPaintingTweaks::blendColors(textColor, backgroundColor, 0.8);
    // only apply color to the widget itself, not to the tooltip or something
    blendedStyle = "QWidget{color: " + blendedColor.name() + "}";

    // what labels to change the color...
    startTitleLabel->setStyleSheet(blendedStyle);
    recentDocumentsLabel->setStyleSheet(blendedStyle);
    helpTitleLabel->setStyleSheet(blendedStyle);
    newFileLinkShortcut->setStyleSheet(blendedStyle);
    openFileShortcut->setStyleSheet(blendedStyle);
    clearRecentFilesLink->setStyleSheet(blendedStyle);
    recentDocumentsListView->setStyleSheet(blendedStyle);

#ifdef Q_OS_ANDROID
    blendedStyle = blendedStyle + "\nQPushButton { padding: 10px }";
#endif

    newFileLink->setStyleSheet(blendedStyle);
    openFileLink->setStyleSheet(blendedStyle);

    // make drop area QFrame have a dotted line
    dropFrameBorder->setObjectName("dropAreaIndicator");
    QString dropFrameStyle = QString("QFrame#dropAreaIndicator { border: 4px dotted ").append(blendedColor.name()).append("}");
    dropFrameBorder->setStyleSheet(dropFrameStyle);

    // only show drop area when we have a document over the empty area
    showDropAreaIndicator(false);

    // add icons for new and open settings to make them stand out a bit more
    openFileLink->setIconSize(QSize(48, 48));
    newFileLink->setIconSize(QSize(48, 48));

    openFileLink->setIcon(KisIconUtils::loadIcon("document-open"));
    newFileLink->setIcon(KisIconUtils::loadIcon("document-new"));

    sourceCodeIcon->setIcon(KisIconUtils::loadIcon(QStringLiteral("code")));

    sourceCodeLink->setText(QString("<a style=\"color: " + blendedColor.name() + " \" href=\"https://github.com/serika12345/librepaint\">")
                            .append(i18n("Source Code")).append("</a>"));

    const QColor faintTextColor = KisPaintingTweaks::blendColors(textColor, backgroundColor, 0.4);
    const QString &faintTextStyle = "QWidget{color: " + faintTextColor.name() + "}";
    labelNoRecentDocs->setStyleSheet(faintTextStyle);

    const QColor frameColor = KisPaintingTweaks::blendColors(textColor, backgroundColor, 0.1);
    const QString &frameQss = "{border: 1px solid " + frameColor.name() + "}";
    recentDocsStackedWidget->setStyleSheet("QStackedWidget#recentDocsStackedWidget" + frameQss);

    // show the dev version labels, if dev version is detected
    showDevVersionHighlight();

}

void KisWelcomePageWidget::dragEnterEvent(QDragEnterEvent *event)
{
    showDropAreaIndicator(true);
    if (event->mimeData()->hasUrls() ||
        event->mimeData()->hasFormat("application/x-krita-node-internal-pointer") ||
        event->mimeData()->hasFormat("application/x-qt-image")) {
        return event->accept();
    }

    return event->ignore();
}

void KisWelcomePageWidget::dropEvent(QDropEvent *event)
{
    showDropAreaIndicator(false);

    if (event->mimeData()->hasUrls() && !event->mimeData()->urls().empty()) {
        Q_FOREACH (const QUrl &url, event->mimeData()->urls()) {
            if (url.toLocalFile().endsWith(".bundle", Qt::CaseInsensitive)) {
                bool r = m_mainWindow->installBundle(url.toLocalFile());
                if (!r) {
                    qWarning() << "Could not install bundle" << url.toLocalFile();
                }
            } else if (!url.isLocalFile()) {
                QScopedPointer<QTemporaryFile> tmp(new QTemporaryFile());
                tmp->setFileName(url.fileName());

                KisRemoteFileFetcher fetcher;

                if (!fetcher.fetchFile(url, tmp.data())) {
                    qWarning() << "Fetching" << url << "failed";
                    continue;
                }
                const auto localUrl = QUrl::fromLocalFile(tmp->fileName());

                m_mainWindow->openDocument(localUrl.toLocalFile(), KisMainWindow::None);
            } else {
                m_mainWindow->openDocument(url.toLocalFile(), KisMainWindow::None);
            }
        }
    }
}

void KisWelcomePageWidget::dragMoveEvent(QDragMoveEvent *event)
{
    m_mainWindow->dragMoveEvent(event);

    if (event->mimeData()->hasUrls() ||
        event->mimeData()->hasFormat("application/x-krita-node-internal-pointer") ||
        event->mimeData()->hasFormat("application/x-qt-image")) {
        return event->accept();
    }

    return event->ignore();
}

void KisWelcomePageWidget::dragLeaveEvent(QDragLeaveEvent */*event*/)
{
    showDropAreaIndicator(false);
    m_mainWindow->dragLeave();
}

bool KisWelcomePageWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == recentDocumentsListView && event->type() == QEvent::Leave) {
        recentDocumentsListView->clearSelection();
    }
    return QWidget::eventFilter(watched, event);
}

void KisWelcomePageWidget::showDevVersionHighlight()
{
    // always flag development version
    if (isDevelopmentBuild()) {
        devBuildLabel->setText(i18n("DEV BUILD"));
        devBuildIcon->setVisible(true);
        devBuildLabel->setVisible(true);
    } else {
        devBuildIcon->setVisible(false);
        devBuildLabel->setVisible(false);
    }
}

void KisWelcomePageWidget::updateShortcutLink(QToolButton *button, QLabel *label, QAction *action)
{
    if (action) {
        QString shortcutText = action->shortcut().toString(QKeySequence::NativeText);
        if (shortcutText.isEmpty()) {
            label->setText(QString());
        } else {
            label->setText(QStringLiteral("(%1)").arg(shortcutText));
        }
        button->show();
        label->show();
    } else {
        button->hide();
        label->hide();
    }
}

void KisWelcomePageWidget::recentDocumentClicked(QModelIndex index)
{
    QString fileUrl = index.data(Qt::ToolTipRole).toString();
    m_mainWindow->openDocument(fileUrl, KisMainWindow::None );
}

void KisWelcomePageWidget::slotRecentDocContextMenuRequest(const QPoint &pos)
{
    QMenu contextMenu;
    QModelIndex index = recentDocumentsListView->indexAt(pos);
    QAction *actionForget = 0;
    if (index.isValid()) {
        actionForget = new QAction(i18n("Forget \"%1\"", index.data(Qt::DisplayRole).toString()), &contextMenu);
        contextMenu.addAction(actionForget);
    }
    QAction *triggered = contextMenu.exec(recentDocumentsListView->mapToGlobal(pos));

    if (index.isValid() && triggered == actionForget) {
        m_mainWindow->removeRecentFile(index.data(Qt::ToolTipRole).toString());
    }
}

bool KisWelcomePageWidget::isDevelopmentBuild()
{
    return KritaVersionWrapper::isDevelopersBuild();
}

void KisWelcomePageWidget::slotNewFileClicked()
{
    m_mainWindow->slotFileNew();
}

void KisWelcomePageWidget::slotOpenFileClicked()
{
    m_mainWindow->slotFileOpen();
}

void KisWelcomePageWidget::slotPaste()
{
    if (!this->isVisible())
        return;

    // Don't do anything if there's no image in the clipboard
    if (!KisClipboard::instance()->hasImage())
        return;

    KisDlgCreateNewDocument *dlg = new KisDlgCreateNewDocument(this);

    dlg->SelectPage(KisDlgCreateNewDocument::Page::CreateFromClipboard);
    dlg->exec();
    dlg->deleteLater();
}

void KisWelcomePageWidget::slotRecentFilesModelIsUpToDate()
{
    KisRecentDocumentsModelWrapper *recentFilesModel = KisRecentDocumentsModelWrapper::instance();
    const bool modelIsEmpty = recentFilesModel->model().rowCount() == 0;

    if (modelIsEmpty) {
        recentDocsStackedWidget->setCurrentWidget(labelNoRecentDocs);
    } else {
        recentDocsStackedWidget->setCurrentWidget(recentDocumentsListView);
    }
    clearRecentFilesLink->setVisible(!modelIsEmpty);
}
