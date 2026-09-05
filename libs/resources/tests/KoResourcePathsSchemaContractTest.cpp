/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoResourcePaths.h"

#include <QTest>

#include <type_traits>
#include <utility>

class KoResourcePathsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeOptionsAndLifetimeSchemaRemainStable();
    void applicationLocationsSchemaRemainStable();
    void assetRegistrationSchemaRemainStable();
    void assetLookupSignaturesRemainStable();
    void writableLocationSignaturesRemainStable();
};

void KoResourcePathsSchemaContractTest::typeOptionsAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_same_v<KoResourcePaths::SearchOptions, QFlags<KoResourcePaths::SearchOption>>);
    static_assert(std::is_class_v<KoResourcePaths>);
    static_assert(std::is_enum_v<KoResourcePaths::SearchOption>);
    static_assert(static_cast<int>(KoResourcePaths::NoSearchOptions) == 0);
    static_assert(static_cast<int>(KoResourcePaths::Recursive) == 1);
    static_assert(static_cast<int>(KoResourcePaths::IgnoreExecBit) == 4);
    static_assert(std::is_default_constructible_v<KoResourcePaths>);
    static_assert(std::has_virtual_destructor_v<KoResourcePaths>);
}

void KoResourcePathsSchemaContractTest::applicationLocationsSchemaRemainStable()
{
    using LocationSignature = QString (*)();

    static_assert(std::is_same_v<decltype(KoResourcePaths::s_overrideAppDataLocation), QString>);
    static_assert(std::is_same_v<decltype(static_cast<LocationSignature>(&KoResourcePaths::getAppDataLocation)),
                                 LocationSignature>);
    static_assert(std::is_same_v<decltype(static_cast<LocationSignature>(&KoResourcePaths::getApplicationRoot)),
                                 LocationSignature>);
}

void KoResourcePathsSchemaContractTest::assetRegistrationSchemaRemainStable()
{
    using AddDirectorySignature = void (*)(const QString &, const QString &, bool);
    using AddTypeSignature = void (*)(const QString &, const char *, const QString &, bool);
    using DirectoriesSignature = QStringList (*)(const QString &);

    static_assert(std::is_same_v<decltype(static_cast<AddDirectorySignature>(&KoResourcePaths::addAssetDir)),
                                 AddDirectorySignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<AddTypeSignature>(&KoResourcePaths::addAssetType)), AddTypeSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<DirectoriesSignature>(&KoResourcePaths::assetDirs)), DirectoriesSignature>);

    static_assert(std::is_same_v<decltype(KoResourcePaths::addAssetDir(std::declval<const QString &>(),
                                                                       std::declval<const QString &>())),
                                 void>);
    static_assert(std::is_same_v<decltype(KoResourcePaths::addAssetType(std::declval<const QString &>(),
                                                                        std::declval<const char *>(),
                                                                        std::declval<const QString &>())),
                                 void>);
}

void KoResourcePathsSchemaContractTest::assetLookupSignaturesRemainStable()
{
    using FindAllSignature = QStringList (*)(const QString &, const QString &, KoResourcePaths::SearchOptions);
    using FindOneSignature = QString (*)(const QString &, const QString &);
    using FindDirectoriesSignature = QStringList (*)(const QString &);

    static_assert(
        std::is_same_v<decltype(static_cast<FindAllSignature>(&KoResourcePaths::findAllAssets)), FindAllSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<FindOneSignature>(&KoResourcePaths::findAsset)), FindOneSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FindDirectoriesSignature>(&KoResourcePaths::findDirs)),
                                 FindDirectoriesSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FindOneSignature>(&KoResourcePaths::locate)), FindOneSignature>);

    static_assert(
        std::is_same_v<decltype(KoResourcePaths::findAllAssets(std::declval<const QString &>())), QStringList>);
}

void KoResourcePathsSchemaContractTest::writableLocationSignaturesRemainStable()
{
    using LocateLocalSignature = QString (*)(const QString &, const QString &, bool);
    using SaveLocationSignature = QString (*)(const QString &, const QString &, bool);

    static_assert(std::is_same_v<decltype(static_cast<LocateLocalSignature>(&KoResourcePaths::locateLocal)),
                                 LocateLocalSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SaveLocationSignature>(&KoResourcePaths::saveLocation)),
                                 SaveLocationSignature>);

    static_assert(std::is_same_v<decltype(KoResourcePaths::locateLocal(std::declval<const QString &>(),
                                                                       std::declval<const QString &>())),
                                 QString>);
    static_assert(std::is_same_v<decltype(KoResourcePaths::saveLocation(std::declval<const QString &>())), QString>);
}

QTEST_APPLESS_MAIN(KoResourcePathsSchemaContractTest)

#include "KoResourcePathsSchemaContractTest.moc"
