/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisAnimationRender.h"

#include <QTest>

#include <type_traits>

class KisAnimationRenderSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void renderEntrySignatureRemainsStable();
};

void KisAnimationRenderSchemaContractTest::renderEntrySignatureRemainsStable()
{
    using Render = bool (*)(KisDocument *, KisViewManager *, KisAnimationRenderingOptions);

    static_assert(std::is_same_v<decltype(&KisAnimationRender::render), Render>);
}

QTEST_GUILESS_MAIN(KisAnimationRenderSchemaContractTest)

#include "KisAnimationRenderSchemaContractTest.moc"
