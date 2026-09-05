/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_base_processor.h>

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
};

using Subject = KisBaseProcessor;

#define ASSERT_SIGNATURE(Method, Signature)                                                                            \
    static_assert(std::is_same_v<decltype(static_cast<Signature>(&Subject::Method)), Signature>)

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

#undef ASSERT_SIGNATURE

QTEST_APPLESS_MAIN(KisBaseProcessorSchemaContractTest)

#include "KisBaseProcessorSchemaContractTest.moc"
