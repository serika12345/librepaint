/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_import_catcher.h"

#include <QTest>

#include <type_traits>

class KisImportCatcherSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void importCatcherPublicInterfaceRemainsStable();
};

void KisImportCatcherSchemaContractTest::importCatcherPublicInterfaceRemainsStable()
{
    using Catcher = KisImportCatcher;
    using ImportedLayerCount = int (Catcher::*)() const;
    using AdaptClipToImageColorSpace = void (*)(KisPaintDeviceSP, KisImageSP);

    static_assert(std::is_base_of_v<QObject, Catcher>);
    static_assert(std::is_constructible_v<Catcher, const QString &, KisViewManager *, const QString &>);
    static_assert(std::has_virtual_destructor_v<Catcher>);
    static_assert(std::is_same_v<decltype(&Catcher::numLayersImported), ImportedLayerCount>);
    static_assert(std::is_same_v<decltype(&Catcher::adaptClipToImageColorSpace), AdaptClipToImageColorSpace>);
}

QTEST_GUILESS_MAIN(KisImportCatcherSchemaContractTest)

#include "KisImportCatcherSchemaContractTest.moc"
