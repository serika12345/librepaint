/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_safe_document_loader.h"

#include <QTest>

#include <type_traits>

class KisSafeDocumentLoaderSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void safeDocumentLoaderPublicInterfaceRemainsStable();
};

void KisSafeDocumentLoaderSchemaContractTest::safeDocumentLoaderPublicInterfaceRemainsStable()
{
    using Loader = KisSafeDocumentLoader;
    using SetPath = void (Loader::*)(const QString &);
    using ReloadImage = void (Loader::*)();
    using LoadingFinished = void (Loader::*)(KisPaintDeviceSP, qreal, qreal, const QSize &);
    using LoadingFailed = void (Loader::*)();
    using FileExistsStateChanged = void (Loader::*)(bool);

    static_assert(std::is_base_of_v<QObject, Loader>);
    static_assert(std::is_constructible_v<Loader, const QString &, QObject *>);
    static_assert(std::has_virtual_destructor_v<Loader>);
    static_assert(std::is_same_v<decltype(&Loader::setPath), SetPath>);
    static_assert(std::is_same_v<decltype(&Loader::reloadImage), ReloadImage>);
    static_assert(std::is_same_v<decltype(&Loader::loadingFinished), LoadingFinished>);
    static_assert(std::is_same_v<decltype(&Loader::loadingFailed), LoadingFailed>);
    static_assert(std::is_same_v<decltype(&Loader::fileExistsStateChanged), FileExistsStateChanged>);
}

QTEST_GUILESS_MAIN(KisSafeDocumentLoaderSchemaContractTest)

#include "KisSafeDocumentLoaderSchemaContractTest.moc"
