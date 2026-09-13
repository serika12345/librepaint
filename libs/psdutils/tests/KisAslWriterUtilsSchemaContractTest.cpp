/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <asl/kis_asl_writer_utils.h>

#include <QTest>

#include <type_traits>

class KisAslWriterUtilsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void schemaRemainsStable();
};

void KisAslWriterUtilsSchemaContractTest::schemaRemainsStable()
{
    using PatternUuidResolver = QString (*)(KoPatternSP);

    static_assert(std::is_same_v<decltype(&KisAslWriterUtils::getPatternUuidLazy<psd_byte_order::psdBigEndian>),
                                 PatternUuidResolver>);
}

QTEST_APPLESS_MAIN(KisAslWriterUtilsSchemaContractTest)

#include "KisAslWriterUtilsSchemaContractTest.moc"
