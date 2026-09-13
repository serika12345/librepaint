/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <cos/kis_txt2_utls.h>

#include <QTest>

#include <type_traits>

class KisTxt2UtilsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void schemaRemainsStable();
};

void KisTxt2UtilsSchemaContractTest::schemaRemainsStable()
{
    using Utils = KisTxt2Utils;
    using HashProducer = QVariantHash (*)();
    using HashTransform = QVariantHash (*)(QVariantHash);
    using TyShTransform = QVariantHash (*)(const QVariantHash, const QRectF, int);

    static_assert(std::is_class_v<Utils>);
    static_assert(std::is_same_v<decltype(&Utils::defaultTxt2), HashProducer>);
    static_assert(std::is_same_v<decltype(&Utils::tyShFromTxt2), TyShTransform>);
    static_assert(std::is_same_v<decltype(&Utils::uncompressKeys), HashTransform>);
}

QTEST_APPLESS_MAIN(KisTxt2UtilsSchemaContractTest)

#include "KisTxt2UtilsSchemaContractTest.moc"
