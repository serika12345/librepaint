/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisVideoSaver.h"

#include <QTest>

#include <type_traits>

class KisAnimationVideoSaverSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void videoSaverPublicInterfaceRemainsStable();
};

void KisAnimationVideoSaverSchemaContractTest::videoSaverPublicInterfaceRemainsStable()
{
    using Saver = KisAnimationVideoSaver;
    using Image = KisImageSP (Saver::*)();
    using Encode = KisImportExportErrorCode (
        Saver::*)(const QString &, const QString &, const QStringList &, const KisAnimationRenderingOptions &);
    using Convert = KisImportExportErrorCode (*)(KisDocument *,
                                                 const QString &,
                                                 const QString &,
                                                 const QStringList &,
                                                 const KisAnimationRenderingOptions &,
                                                 bool);

    static_assert(std::is_base_of_v<QObject, Saver>);
    static_assert(std::is_constructible_v<Saver, KisDocument *, bool>);
    static_assert(std::has_virtual_destructor_v<Saver>);
    static_assert(std::is_same_v<decltype(&Saver::image), Image>);
    static_assert(std::is_same_v<decltype(&Saver::encode), Encode>);
    static_assert(std::is_same_v<decltype(&Saver::convert), Convert>);
}

QTEST_GUILESS_MAIN(KisAnimationVideoSaverSchemaContractTest)

#include "KisAnimationVideoSaverSchemaContractTest.moc"
