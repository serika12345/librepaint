/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisLazyCreateTransformMaskKeyframesCommand.h"

#include <QTest>

#include <type_traits>

class KisLazyCreateTransformMaskKeyframesCommandSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void publicInterfaceRemainsStable();
};

void KisLazyCreateTransformMaskKeyframesCommandSchemaContractTest::publicInterfaceRemainsStable()
{
    using Command = KisLazyCreateTransformMaskKeyframesCommand;
    using HasAnimation = bool (*)(KisTransformMaskSP);

    static_assert(std::is_base_of_v<KisCommandUtils::AggregateCommand, Command>);
    static_assert(std::is_constructible_v<Command, KisTransformMaskSP, KUndo2Command *>);
    static_assert(std::is_same_v<decltype(&Command::maskHasAnimation), HasAnimation>);
}

QTEST_GUILESS_MAIN(KisLazyCreateTransformMaskKeyframesCommandSchemaContractTest)

#include "KisLazyCreateTransformMaskKeyframesCommandSchemaContractTest.moc"
