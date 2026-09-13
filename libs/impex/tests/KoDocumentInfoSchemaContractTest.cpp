/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoDocumentInfo.h>

#include <QTest>

#include <type_traits>

class KoDocumentInfoSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void lifetimeAndUpdatedSignalSchemaRemainStable();
};

void KoDocumentInfoSchemaContractTest::lifetimeAndUpdatedSignalSchemaRemainStable()
{
    using UpdatedSignal = void (KoDocumentInfo::*)(const QString &, const QString &);

    static_assert(std::is_base_of_v<QObject, KoDocumentInfo>);
    static_assert(std::has_virtual_destructor_v<KoDocumentInfo>);
    static_assert(std::is_same_v<decltype(&KoDocumentInfo::infoUpdated), UpdatedSignal>);
}

QTEST_GUILESS_MAIN(KoDocumentInfoSchemaContractTest)

#include "KoDocumentInfoSchemaContractTest.moc"
