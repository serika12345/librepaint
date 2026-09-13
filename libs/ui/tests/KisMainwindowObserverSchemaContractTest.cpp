/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "application/ui/workspace/kis_mainwindow_observer.h"

#include <QTest>

#include <type_traits>

namespace
{
class MainwindowObserverProbe final : public KisMainwindowObserver
{
public:
    void setViewManager(KisViewManager *) override
    {
    }

protected:
    void setCanvas(KoCanvasBase *) override
    {
    }
    void unsetCanvas() override
    {
    }
};
} // namespace

class KisMainwindowObserverSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void observerTypeConstructionAndViewManagerSchemaRemainStable();
};

void KisMainwindowObserverSchemaContractTest::observerTypeConstructionAndViewManagerSchemaRemainStable()
{
    using Observer = KisMainwindowObserver;

    static_assert(std::is_base_of_v<KoCanvasObserverBase, Observer>);
    static_assert(std::is_abstract_v<Observer>);
    static_assert(std::is_constructible_v<MainwindowObserverProbe>);
    static_assert(std::has_virtual_destructor_v<Observer>);
    static_assert(std::is_same_v<decltype(&Observer::setViewManager), void (Observer::*)(KisViewManager *)>);
}

QTEST_GUILESS_MAIN(KisMainwindowObserverSchemaContractTest)

#include "KisMainwindowObserverSchemaContractTest.moc"
