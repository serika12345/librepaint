/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <filter/kis_filter.h>
#include <kis_base_processor.h>
#include <kis_bookmarked_configuration_manager.h>

#include <QTest>

#include <type_traits>

class KisBaseProcessorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseProcessorTypeLifetimeAndConstructionSchemaRemainStable();
    void baseProcessorIdentityMenuAndShortcutSignaturesRemainStable();
    void baseProcessorConfigurationFactorySignaturesRemainStable();
    void baseProcessorCapabilitySignaturesRemainStable();
    void baseProcessorBookmarkAndPresentationSignaturesRemainStable();
    void bookmarkManagerTypeLifetimeAndStableNamesSchemaRemainStable();
    void bookmarkManagerPersistenceSignaturesRemainStable();
    void bookmarkManagerQueryAndRemovalSignaturesRemainStable();
    void bookmarkManagerUniqueNameSignatureRemainsStable();
    void filterTypeLifetimeAndConstructionSchemaRemainStable();
    void filterProcessingSignaturesRemainStable();
    void filterProcessingExtentSignaturesRemainStable();
    void filterTransparencyAndMaskConfigurationSignaturesRemainStable();
};

using Subject = KisBaseProcessor;

#define ASSERT_SIGNATURE(Method, Signature)                                                                            \
    static_assert(std::is_same_v<decltype(static_cast<Signature>(&Subject::Method)), Signature>)

using BookmarkManager = KisBookmarkedConfigurationManager;

#define ASSERT_BOOKMARK_MANAGER_SIGNATURE(Method, Signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<Signature>(&BookmarkManager::Method)), Signature>)

using Filter = KisFilter;

#define ASSERT_FILTER_SIGNATURE(Method, Signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<Signature>(&Filter::Method)), Signature>)

class FilterConstructionProbe : public Filter
{
public:
    using Filter::Filter;

    void processImpl(KisPaintDeviceSP, const QRect &, const KisFilterConfigurationSP, KoUpdater *) const override
    {
    }
};

void KisBaseProcessorSchemaContractTest::baseProcessorTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<Subject>);
    static_assert(std::is_base_of_v<KisShared, Subject>);
    static_assert(std::is_constructible_v<Subject, const KoID &, const KoID &, const QString &>);
    static_assert(std::has_virtual_destructor_v<Subject>);
}

void KisBaseProcessorSchemaContractTest::baseProcessorIdentityMenuAndShortcutSignaturesRemainStable()
{
    using String = QString (Subject::*)() const;
    using Category = KoID (Subject::*)() const;
    using Shortcut = QKeySequence (Subject::*)() const;
    ASSERT_SIGNATURE(id, String);
    ASSERT_SIGNATURE(menuCategory, Category);
    ASSERT_SIGNATURE(menuEntry, String);
    ASSERT_SIGNATURE(name, String);
    ASSERT_SIGNATURE(shortcut, Shortcut);
}

void KisBaseProcessorSchemaContractTest::baseProcessorConfigurationFactorySignaturesRemainStable()
{
    using Widget = KisConfigWidget *(Subject::*)(QWidget *, KisPaintDeviceSP, bool) const;
    using Configuration = KisFilterConfigurationSP (Subject::*)(KisResourcesInterfaceSP) const;
    ASSERT_SIGNATURE(createConfigurationWidget, Widget);
    ASSERT_SIGNATURE(defaultConfiguration, Configuration);
    ASSERT_SIGNATURE(factoryConfiguration, Configuration);
}

void KisBaseProcessorSchemaContractTest::baseProcessorCapabilitySignaturesRemainStable()
{
    using Independence = ColorSpaceIndependence (Subject::*)() const;
    using Capability = bool (Subject::*)() const;
    ASSERT_SIGNATURE(colorSpaceIndependence, Independence);
    ASSERT_SIGNATURE(supportsAdjustmentLayers, Capability);
    ASSERT_SIGNATURE(supportsPainting, Capability);
    ASSERT_SIGNATURE(supportsThreading, Capability);
}

void KisBaseProcessorSchemaContractTest::baseProcessorBookmarkAndPresentationSignaturesRemainStable()
{
    using Bookmark = KisBookmarkedConfigurationManager *(Subject::*)();
    using ConstBookmark = const KisBookmarkedConfigurationManager *(Subject::*)() const;
    using Presentation = bool (Subject::*)();
    ASSERT_SIGNATURE(bookmarkManager, Bookmark);
    ASSERT_SIGNATURE(bookmarkManager, ConstBookmark);
    ASSERT_SIGNATURE(showConfigurationWidget, Presentation);
}

void KisBaseProcessorSchemaContractTest::bookmarkManagerTypeLifetimeAndStableNamesSchemaRemainStable()
{
    static_assert(std::is_class_v<BookmarkManager>);
    static_assert(std::is_same_v<decltype(BookmarkManager::ConfigDefault), const char[]>);
    static_assert(std::is_same_v<decltype(BookmarkManager::ConfigLastUsed), const char[]>);
    static_assert(std::is_constructible_v<BookmarkManager, const QString &, KisSerializableConfigurationFactory *>);
    static_assert(std::is_destructible_v<BookmarkManager>);
}

void KisBaseProcessorSchemaContractTest::bookmarkManagerPersistenceSignaturesRemainStable()
{
    ASSERT_BOOKMARK_MANAGER_SIGNATURE(load, KisSerializableConfigurationSP (BookmarkManager::*)(const QString &) const);
    ASSERT_BOOKMARK_MANAGER_SIGNATURE(save,
                                      void (BookmarkManager::*)(const QString &, const KisSerializableConfigurationSP));
    ASSERT_BOOKMARK_MANAGER_SIGNATURE(exists, bool (BookmarkManager::*)(const QString &) const);
}

void KisBaseProcessorSchemaContractTest::bookmarkManagerQueryAndRemovalSignaturesRemainStable()
{
    ASSERT_BOOKMARK_MANAGER_SIGNATURE(configurations, QList<QString> (BookmarkManager::*)() const);
    ASSERT_BOOKMARK_MANAGER_SIGNATURE(defaultConfiguration,
                                      KisSerializableConfigurationSP (BookmarkManager::*)() const);
    ASSERT_BOOKMARK_MANAGER_SIGNATURE(remove, void (BookmarkManager::*)(const QString &));
}

void KisBaseProcessorSchemaContractTest::bookmarkManagerUniqueNameSignatureRemainsStable()
{
    ASSERT_BOOKMARK_MANAGER_SIGNATURE(uniqueName, QString (BookmarkManager::*)(const KLocalizedString &));
}

void KisBaseProcessorSchemaContractTest::filterTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<Filter>);
    static_assert(std::is_base_of_v<KisBaseProcessor, Filter>);
    static_assert(std::is_constructible_v<FilterConstructionProbe, const KoID &, const KoID &, const QString &>);
    static_assert(std::has_virtual_destructor_v<Filter>);
}

void KisBaseProcessorSchemaContractTest::filterProcessingSignaturesRemainStable()
{
    using SingleDevice =
        void (Filter::*)(KisPaintDeviceSP, const QRect &, const KisFilterConfigurationSP, KoUpdater *) const;
    using TwoDevices = void (Filter::*)(KisPaintDeviceSP,
                                        KisPaintDeviceSP,
                                        KisSelectionSP,
                                        const QRect &,
                                        const KisFilterConfigurationSP,
                                        KoUpdater *) const;
    ASSERT_FILTER_SIGNATURE(processImpl, SingleDevice);
    ASSERT_FILTER_SIGNATURE(process, SingleDevice);
    ASSERT_FILTER_SIGNATURE(process, TwoDevices);
}

void KisBaseProcessorSchemaContractTest::filterProcessingExtentSignaturesRemainStable()
{
    using Extent = QRect (Filter::*)(const QRect &, const KisFilterConfigurationSP, int) const;
    using LevelOfDetail = bool (Filter::*)(const KisFilterConfigurationSP, int) const;
    ASSERT_FILTER_SIGNATURE(neededRect, Extent);
    ASSERT_FILTER_SIGNATURE(changedRect, Extent);
    ASSERT_FILTER_SIGNATURE(supportsLevelOfDetail, LevelOfDetail);
}

void KisBaseProcessorSchemaContractTest::filterTransparencyAndMaskConfigurationSignaturesRemainStable()
{
    ASSERT_FILTER_SIGNATURE(needsTransparentPixels,
                            bool (Filter::*)(const KisFilterConfigurationSP, const KoColorSpace *) const);
    ASSERT_FILTER_SIGNATURE(configurationAllowedForMask, bool (Filter::*)(KisFilterConfigurationSP) const);
    ASSERT_FILTER_SIGNATURE(fixLoadedFilterConfigurationForMasks, void (Filter::*)(KisFilterConfigurationSP) const);
}

#undef ASSERT_FILTER_SIGNATURE
#undef ASSERT_BOOKMARK_MANAGER_SIGNATURE
#undef ASSERT_SIGNATURE

QTEST_APPLESS_MAIN(KisBaseProcessorSchemaContractTest)

#include "KisBaseProcessorSchemaContractTest.moc"
