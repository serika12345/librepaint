/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-only */
#include "../xmlgui/KisShortcutsDialog.h"
#include "../xmlgui/KisShortcutsEditor.h"
#include "../xmlgui/kkeysequencewidget.h"
#include "../xmlgui/kmainwindow.h"
#include "../xmlgui/kshortcutwidget.h"
#include "../xmlgui/ktoggletoolbaraction.h"
#include "../xmlgui/ktoolbar.h"
#include "../xmlgui/kxmlguibuilder.h"
#include "../xmlgui/kxmlguifactory.h"
#include "../xmlgui/kxmlguiwindow.h"
#include <QTest>
#include <utility>

class KKeySequenceWidgetSchemaContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void shortcutsDialogTypeAndCollectionSchemaRemainStable();
};
void KKeySequenceWidgetSchemaContractTest::shortcutsDialogTypeAndCollectionSchemaRemainStable()
{
    using E = KisShortcutsEditor;

    QVERIFY(defaultActionTypes == (E::WidgetAction | E::WindowAction | E::ApplicationAction));
}

QTEST_GUILESS_MAIN(KKeySequenceWidgetSchemaContractTest)
#include "KKeySequenceWidgetSchemaContractTest.moc"
