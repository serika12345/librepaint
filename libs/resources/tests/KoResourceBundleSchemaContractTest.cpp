/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoResourceBundle.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_RESOURCE_BUNDLE_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoResourceBundle::method)), signature>)
} // namespace

class KoResourceBundleSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resourceBundleIdentityAndLifecycleSignaturesRemainStable();
    void resourceBundleMetadataAndThumbnailSignaturesRemainStable();
    void resourceBundlePersistenceSignaturesRemainStable();
    void resourceBundleInventorySignaturesRemainStable();
    void resourceBundleManifestAndTagSignaturesRemainStable();
};

void KoResourceBundleSchemaContractTest::resourceBundleIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_same_v<KoResourceBundleSP, QSharedPointer<KoResourceBundle>>);
    static_assert(std::is_class_v<KoResourceBundle>);
    static_assert(std::is_constructible_v<KoResourceBundle, QString const &>);
    static_assert(std::has_virtual_destructor_v<KoResourceBundle>);
}

void KoResourceBundleSchemaContractTest::resourceBundleMetadataAndThumbnailSignaturesRemainStable()
{
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(defaultFileExtension, QString (KoResourceBundle::*)() const);
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(filename, QString (KoResourceBundle::*)() const);
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(image, QImage (KoResourceBundle::*)() const);
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(metaData,
                                     const QString (KoResourceBundle::*)(const QString &, const QString &) const);
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(setMetaData, void (KoResourceBundle::*)(const QString &, const QString &));
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(setThumbnail, void (KoResourceBundle::*)(QImage));

    static_assert(
        std::is_same_v<decltype(std::declval<const KoResourceBundle &>().metaData(std::declval<const QString &>())),
                       const QString>);
}

void KoResourceBundleSchemaContractTest::resourceBundlePersistenceSignaturesRemainStable()
{
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(load, bool (KoResourceBundle::*)());
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(loadFromDevice, bool (KoResourceBundle::*)(QIODevice *));
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(save, bool (KoResourceBundle::*)());
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(saveManifest, void (KoResourceBundle::*)(QScopedPointer<KoStore> &));
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(saveMetadata, void (KoResourceBundle::*)(QScopedPointer<KoStore> &));
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(saveToDevice, bool (KoResourceBundle::*)(QIODevice *) const);
}

void KoResourceBundleSchemaContractTest::resourceBundleInventorySignaturesRemainStable()
{
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(
        addResource,
        void (KoResourceBundle::*)(QString, QString, QVector<KisTagSP>, const QString, const int, const QString));
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(exportResource,
                                     bool (KoResourceBundle::*)(const QString &, const QString &, QIODevice *));
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(loadResource, bool (KoResourceBundle::*)(KoResourceSP));
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(resource, KoResourceSP (KoResourceBundle::*)(const QString &, const QString &));
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(resourceCount, int (KoResourceBundle::*)() const);
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(resourceMd5, QString (KoResourceBundle::*)(const QString &));
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(resourceTypes, QStringList (KoResourceBundle::*)() const);

    static_assert(
        std::is_same_v<decltype(std::declval<KoResourceBundle &>().addResource(std::declval<QString>(),
                                                                               std::declval<QString>(),
                                                                               std::declval<QVector<KisTagSP>>(),
                                                                               std::declval<const QString>())),
                       void>);
    static_assert(
        std::is_same_v<decltype(std::declval<KoResourceBundle &>().addResource(std::declval<QString>(),
                                                                               std::declval<QString>(),
                                                                               std::declval<QVector<KisTagSP>>(),
                                                                               std::declval<const QString>(),
                                                                               std::declval<const int>())),
                       void>);
}

void KoResourceBundleSchemaContractTest::resourceBundleManifestAndTagSignaturesRemainStable()
{
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(getTagsList, QList<QString> (KoResourceBundle::*)());
    ASSERT_RESOURCE_BUNDLE_SIGNATURE(manifest, KoResourceBundleManifest & (KoResourceBundle::*)());
}

QTEST_APPLESS_MAIN(KoResourceBundleSchemaContractTest)

#include "KoResourceBundleSchemaContractTest.moc"
