/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisQQuickPopupWidget.h>

#include <QTest>

#include <type_traits>

class KisQQuickPopupWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rootObjectReadySignalSignatureRemainsStable();
};

void KisQQuickPopupWidgetSchemaContractTest::rootObjectReadySignalSignatureRemainsStable()
{
    using RootObjectReadySignal = void (KisQQuickPopupWidget::*)();

    static_assert(std::is_same_v<decltype(&KisQQuickPopupWidget::signalRootObjectReady), RootObjectReadySignal>);
}

QTEST_GUILESS_MAIN(KisQQuickPopupWidgetSchemaContractTest)

#include "KisQQuickPopupWidgetSchemaContractTest.moc"
