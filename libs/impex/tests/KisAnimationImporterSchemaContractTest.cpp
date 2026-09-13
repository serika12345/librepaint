/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_animation_importer.h"

#include <QTest>

#include <type_traits>

class KisAnimationImporterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void animationImporterPublicInterfaceRemainsStable();
};

void KisAnimationImporterSchemaContractTest::animationImporterPublicInterfaceRemainsStable()
{
    using Importer = KisAnimationImporter;
    using Import = KisImportExportErrorCode (Importer::*)(QStringList, int, int, bool, bool, int, bool, QList<int>);

    static_assert(std::is_base_of_v<QObject, Importer>);
    static_assert(std::is_constructible_v<Importer, KisImageSP, KoUpdaterPtr>);
    static_assert(std::is_constructible_v<Importer, KisDocument *>);
    static_assert(std::has_virtual_destructor_v<Importer>);
    static_assert(std::is_same_v<decltype(&Importer::import), Import>);
}

QTEST_GUILESS_MAIN(KisAnimationImporterSchemaContractTest)

#include "KisAnimationImporterSchemaContractTest.moc"
