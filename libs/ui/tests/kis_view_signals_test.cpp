/*
 *  SPDX-FileCopyrightText: 2026 Moritz Staudinger
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_view_signals_test.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QTabWidget>
#include <QTimer>

#include <KPageDialog>
#include <KPageWidgetItem>

#include <document/KisDocument.h>
#include <dialogs/kis_dlg_preferences.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <kis_transform_mask.h>
#include <opengl/kis_opengl.h>

#include <KoInteractionTool.h>
#include <KoToolManager.h>

#include <Canvas.h>
#include <Krita.h>
#include <ManagedColor.h>
#include <Resource.h>
#include <View.h>

#include <testui.h>
#include <util.h>

void KisViewSignalsTest::initTestCase()
{
    Q_INIT_RESOURCE(krita);
    KisOpenGL::setDefaultSurfaceConfig(KisOpenGL::RendererConfig());

    m_document = createEmptyDocument();
    QVERIFY(m_document);

    m_mainWindow = KisPart::instance()->createMainWindow();
    QVERIFY(m_mainWindow);

    m_kisView = m_mainWindow->newView(m_document);
    QVERIFY(m_kisView);

    m_viewManager = m_mainWindow->viewManager();
    QVERIFY(m_viewManager);
    m_viewManager->setCurrentView(m_kisView);

    const auto timers = m_mainWindow->findChildren<QTimer*>();
    for (QTimer *timer : timers) {
        if (timer && timer->isSingleShot() && timer->interval() == 1000) {
            timer->stop();
        }
    }

    QApplication::processEvents();

    m_view = new View(m_kisView);
    QVERIFY(m_view);
}

void KisViewSignalsTest::cleanupTestCase()
{
    delete m_view;
    m_view = nullptr;

    m_viewManager = nullptr;
    m_kisView = nullptr;

    if (m_mainWindow) {
        m_mainWindow->hide();
        QApplication::processEvents();
        delete m_mainWindow;
        m_mainWindow = nullptr;
        QApplication::sendPostedEvents();
        QApplication::processEvents();
    }

    delete m_document;
    m_document = nullptr;
    QApplication::sendPostedEvents();
    QApplication::processEvents();
}

void KisViewSignalsTest::testCurrentToolChanged()
{
    const QString brushToolId = "KritaShape/KisToolBrush";
    const QString currentToolId = KoToolManager::instance()->activeToolId();
    const QString targetToolId = currentToolId == brushToolId ? QString::fromLatin1(KoInteractionTool_ID) : brushToolId;

    QSignalSpy spy(m_view, SIGNAL(currentToolChanged(QString)));
    QVERIFY(spy.isValid());

    KoToolManager::instance()->switchToolRequested(targetToolId);
    QApplication::processEvents();

    QVERIFY(spy.count() >= 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), targetToolId);
    QCOMPARE(KoToolManager::instance()->activeToolId(), targetToolId);
}

void KisViewSignalsTest::testCurrentBrushPresetChanged()
{
    QString type = "preset";
    QMap<QString, Resource*> presets = Krita().resources(type);
    if (presets.size() < 2) {
        qDeleteAll(presets);
        QSKIP("Need at least two paintop presets to test preset change signals.");
    }

    Resource *initialPreset = nullptr;
    Resource *targetPreset = nullptr;
    for (auto it = presets.begin(); it != presets.end(); ++it) {
        if (!it.value()) {
            continue;
        }

        if (!initialPreset) {
            initialPreset = it.value();
            continue;
        }

        if (!targetPreset) {
            targetPreset = it.value();
            break;
        }
    }

    if (!initialPreset || !targetPreset) {
        qDeleteAll(presets);
        QSKIP("Could not obtain two valid paintop presets from the resource model.");
    }

    m_view->setCurrentBrushPreset(initialPreset);
    QApplication::processEvents();

    QScopedPointer<Resource> currentPreset(m_view->currentBrushPreset());
    QVERIFY(currentPreset);
    QCOMPARE(currentPreset->name(), initialPreset->name());

    QSignalSpy spy(m_view, SIGNAL(currentBrushPresetChanged()));
    QVERIFY(spy.isValid());

    m_view->setCurrentBrushPreset(targetPreset);
    QApplication::processEvents();

    QVERIFY(spy.count() >= 1);

    QScopedPointer<Resource> changedPreset(m_view->currentBrushPreset());
    QVERIFY(changedPreset);
    QCOMPARE(changedPreset->name(), targetPreset->name());

    qDeleteAll(presets);
}

void KisViewSignalsTest::testForegroundColorChanged()
{
    QScopedPointer<ManagedColor> targetColor(ManagedColor::fromQColor(QColor(12, 34, 56, 255), m_view->canvas()));
    QVERIFY(targetColor);

    QSignalSpy spy(m_view, SIGNAL(foregroundColorChanged()));
    QVERIFY(spy.isValid());

    m_view->setForeGroundColor(targetColor.data());
    QApplication::processEvents();

    QVERIFY(spy.count() >= 1);

    QScopedPointer<ManagedColor> changedColor(m_view->foregroundColor());
    QVERIFY(changedColor);
    QVERIFY(*changedColor == *targetColor);
}

void KisViewSignalsTest::testBackgroundColorChanged()
{
    QScopedPointer<ManagedColor> targetColor(ManagedColor::fromQColor(QColor(78, 90, 123, 255), m_view->canvas()));
    QVERIFY(targetColor);

    QSignalSpy spy(m_view, SIGNAL(backgroundColorChanged()));
    QVERIFY(spy.isValid());

    m_view->setBackGroundColor(targetColor.data());
    QApplication::processEvents();

    QVERIFY(spy.count() >= 1);

    QScopedPointer<ManagedColor> changedColor(m_view->backgroundColor());
    QVERIFY(changedColor);
    QVERIFY(*changedColor == *targetColor);
}

void KisViewSignalsTest::testConfigureActionOpensToolsPreferences()
{
    /*
     * Consumer: The selection actions panel's Configure action.
     * Operation: Requests the General / Tools preferences destination and triggers the action.
     * Observable result: The modal preferences dialog presents the General page with the Tools tab selected.
     * Failure impact: Users configuring the selection actions panel land on unrelated preferences.
     */
    KisAction *configureAction = m_viewManager->actionManager()->actionByName(
        QStringLiteral("options_configure"));
    QVERIFY(configureAction);

    configureAction->setData(QVariantList {
        KisDlgPreferences::Page::General,
        KisDlgPreferences::GeneralTabs::Tools,
    });

    bool dialogWasShown = false;
    QString pageName;
    QString tabName;

    QTimer closeGuard;
    closeGuard.setSingleShot(true);
    connect(&closeGuard, &QTimer::timeout, []() {
        if (QWidget *modalWidget = QApplication::activeModalWidget()) {
            modalWidget->close();
        }
    });
    closeGuard.start(5000);

    QTimer::singleShot(0, this, [&]() {
        KPageDialog *dialog = qobject_cast<KPageDialog *>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }

        dialogWasShown = true;
        if (KPageWidgetItem *page = dialog->currentPage()) {
            pageName = page->name();
            if (QTabWidget *tabs = page->widget()->findChild<QTabWidget *>()) {
                tabName = tabs->tabText(tabs->currentIndex());
            }
        }

        if (QPushButton *cancelButton = dialog->button(QDialogButtonBox::Cancel)) {
            cancelButton->click();
        } else {
            dialog->reject();
        }
    });

    configureAction->trigger();
    closeGuard.stop();

    QVERIFY(dialogWasShown);
    QCOMPARE(pageName, QStringLiteral("General"));
    QCOMPARE(tabName, QStringLiteral("Tools"));
}

KISTEST_MAIN(KisViewSignalsTest)
