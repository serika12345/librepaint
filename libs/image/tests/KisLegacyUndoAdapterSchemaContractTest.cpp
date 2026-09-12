/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_legacy_undo_adapter.h"

#include <QTest>

#include <type_traits>

class KisLegacyUndoAdapterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAndConstructionSchemaRemainStable();
    void currentCommandAndUndoSignaturesRemainStable();
    void commandAndMacroSignaturesRemainStable();
};

void KisLegacyUndoAdapterSchemaContractTest::typeAndConstructionSchemaRemainStable()
{
    using Adapter = KisLegacyUndoAdapter;

    static_assert(std::is_class_v<Adapter>);
    static_assert(std::is_base_of_v<KisUndoAdapter, Adapter>);
    static_assert(std::is_constructible_v<Adapter, KisUndoStore *, KisImageWSP>);
}

void KisLegacyUndoAdapterSchemaContractTest::currentCommandAndUndoSignaturesRemainStable()
{
    using Adapter = KisLegacyUndoAdapter;

    static_assert(std::is_same_v<decltype(&Adapter::presentCommand), const KUndo2Command *(Adapter::*)()>);
    static_assert(std::is_same_v<decltype(&Adapter::undoLastCommand), void (Adapter::*)()>);
}

void KisLegacyUndoAdapterSchemaContractTest::commandAndMacroSignaturesRemainStable()
{
    using Adapter = KisLegacyUndoAdapter;

    static_assert(std::is_same_v<decltype(&Adapter::addCommand), void (Adapter::*)(KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(&Adapter::beginMacro), void (Adapter::*)(const KUndo2MagicString &)>);
    static_assert(std::is_same_v<decltype(&Adapter::endMacro), void (Adapter::*)()>);
}

QTEST_GUILESS_MAIN(KisLegacyUndoAdapterSchemaContractTest)

#include "KisLegacyUndoAdapterSchemaContractTest.moc"
