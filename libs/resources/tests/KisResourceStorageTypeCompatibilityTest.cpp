/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisResourceStorage.h>

#include <QTest>

#include <array>
#include <tuple>

class KisResourceStorageTypeCompatibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resourceCacheStorageTypeIdsAndNamesRemainStable();
};

// Compatibility requirement: Existing resourcecache.sqlite databases depend on stable storage_types IDs and names.
void KisResourceStorageTypeCompatibilityTest::resourceCacheStorageTypeIdsAndNamesRemainStable()
{
    using Storage = KisResourceStorage;
    using StorageType = Storage::StorageType;

    // Consumer: Users updating LibrePaint with a resource-cache database from an earlier run.
    // Operation: LibrePaint reads stored storage_type_id values while registering and cleaning up resource storages.
    // Observable result: Each storage type resolves to the same database ID and untranslated lookup name.
    // Failure impact: Resources can be assigned to the wrong storage kind or temporary resources can survive cleanup.
    const std::array<std::tuple<StorageType, int, const char *>, 7> storageTypes{{
        {StorageType::Unknown, 1, "Unknown"},
        {StorageType::Folder, 2, "Folder"},
        {StorageType::Bundle, 3, "Bundle"},
        {StorageType::AdobeBrushLibrary, 4, "Adobe Brush Library"},
        {StorageType::AdobeStyleLibrary, 5, "Adobe Style Library"},
        {StorageType::Memory, 6, "Memory"},
        {StorageType::FontStorage, 7, "Font Storage"},
    }};

    for (const auto &[storageType, expectedId, expectedName] : storageTypes) {
        QCOMPARE(static_cast<int>(storageType), expectedId);
        QCOMPARE(Storage::storageTypeToUntranslatedString(storageType), QString::fromLatin1(expectedName));
    }
}

QTEST_GUILESS_MAIN(KisResourceStorageTypeCompatibilityTest)

#include "KisResourceStorageTypeCompatibilityTest.moc"
