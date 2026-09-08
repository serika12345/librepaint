/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisResourceLoader.h"

#include <QTest>

#include <type_traits>

namespace
{

class LoaderResource final : public KoResource
{
public:
    explicit LoaderResource(const QString &name);

    KoResourceSP clone() const override;
    bool loadFromDevice(QIODevice *device, KisResourcesInterfaceSP resourcesInterface) override;
    QPair<QString, QString> resourceType() const override;
};

class ResourceLoaderBaseProbe final : public KisResourceLoaderBase
{
public:
    using KisResourceLoaderBase::KisResourceLoaderBase;

    KoResourceSP create(const QString &name) override;
};

using TemplateLoader = KisResourceLoader<LoaderResource>;

#define ASSERT_RESOURCE_LOADER_SIGNATURE(type, method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

} // namespace

class KisResourceLoaderSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseTypeConstructionAndLifetimeSchemaRemainStable();
    void baseIdentityAndFormatSignaturesRemainStable();
    void baseCreationAndLoadingSignaturesRemainStable();
    void templateLoaderSchemaRemainsStable();
};

void KisResourceLoaderSchemaContractTest::baseTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Base = KisResourceLoaderBase;

    static_assert(std::is_class_v<Base>);
    static_assert(std::is_abstract_v<Base>);
    static_assert(std::is_constructible_v<ResourceLoaderBaseProbe,
                                          const QString &,
                                          const QString &,
                                          const QString &,
                                          const QStringList &>);
    static_assert(std::has_virtual_destructor_v<Base>);
}

void KisResourceLoaderSchemaContractTest::baseIdentityAndFormatSignaturesRemainStable()
{
    using Base = KisResourceLoaderBase;

    ASSERT_RESOURCE_LOADER_SIGNATURE(Base, filters, QStringList (Base::*)() const);
    ASSERT_RESOURCE_LOADER_SIGNATURE(Base, id, QString (Base::*)() const);
    ASSERT_RESOURCE_LOADER_SIGNATURE(Base, mimetypes, QStringList (Base::*)() const);
    ASSERT_RESOURCE_LOADER_SIGNATURE(Base, name, QString (Base::*)() const);
    ASSERT_RESOURCE_LOADER_SIGNATURE(Base, resourceSubType, QString (Base::*)() const);
    ASSERT_RESOURCE_LOADER_SIGNATURE(Base, resourceType, QString (Base::*)() const);
}

void KisResourceLoaderSchemaContractTest::baseCreationAndLoadingSignaturesRemainStable()
{
    using Base = KisResourceLoaderBase;

    ASSERT_RESOURCE_LOADER_SIGNATURE(Base, create, KoResourceSP (Base::*)(const QString &));
    ASSERT_RESOURCE_LOADER_SIGNATURE(Base,
                                     load,
                                     KoResourceSP (Base::*)(const QString &, QIODevice &, KisResourcesInterfaceSP));
    ASSERT_RESOURCE_LOADER_SIGNATURE(Base, load, bool (Base::*)(KoResourceSP, QIODevice &, KisResourcesInterfaceSP));
}

void KisResourceLoaderSchemaContractTest::templateLoaderSchemaRemainsStable()
{
    static_assert(std::is_class_v<TemplateLoader>);
    static_assert(std::is_base_of_v<KisResourceLoaderBase, TemplateLoader>);
    static_assert(
        std::
            is_constructible_v<TemplateLoader, const QString &, const QString &, const QString &, const QStringList &>);
    ASSERT_RESOURCE_LOADER_SIGNATURE(TemplateLoader, create, KoResourceSP (TemplateLoader::*)(const QString &));
}

QTEST_APPLESS_MAIN(KisResourceLoaderSchemaContractTest)

#include "KisResourceLoaderSchemaContractTest.moc"
