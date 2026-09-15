/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "PerspectiveBasedAssistantHelper.h"

#include <QTest>

#include <type_traits>

class PerspectiveBasedAssistantHelperSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void handleBasedPerspectiveHelperSchemaRemainsStable();
};

void PerspectiveBasedAssistantHelperSchemaContractTest::handleBasedPerspectiveHelperSchemaRemainsStable()
{
    using Helper = PerspectiveBasedAssistantHelper;
    using Handles = const QList<KisPaintingAssistantHandleSP> &;

    static_assert(std::is_same_v<decltype(&Helper::getTetragon), bool (*)(Handles, bool, QPolygonF &)>);
    static_assert(std::is_same_v<decltype(&Helper::getAllConnectedTetragon), QPolygonF (*)(Handles)>);
    static_assert(
        std::is_same_v<decltype(static_cast<qreal (*)(Handles, bool, const QPointF &)>(&Helper::distanceInGrid)),
                       qreal (*)(Handles, bool, const QPointF &)>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(PerspectiveBasedAssistantHelperSchemaContractTest)

#include "PerspectiveBasedAssistantHelperSchemaContractTest.moc"
