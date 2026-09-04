/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorConversionTransformationFactory.h>
#include <KoColorSpaceFactory.h>

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoColorSpaceFactory::method)), signature>)

#define ASSERT_COLOR_CONVERSION_FACTORY_SIGNATURE(method, signature)                                                   \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<signature>(&KoColorConversionTransformationFactory::method)), signature>)

class ColorSpaceFactoryConstructorProbe : public KoColorSpaceFactory
{
protected:
    ColorSpaceFactoryConstructorProbe()
        : KoColorSpaceFactory()
    {
    }
};

class ColorConversionFactoryConstructorProbe final : public KoColorConversionTransformationFactory
{
public:
    using KoColorConversionTransformationFactory::KoColorConversionTransformationFactory;

    KoColorConversionTransformation *
    createColorTransformation(const KoColorSpace *,
                              const KoColorSpace *,
                              KoColorConversionTransformation::Intent,
                              KoColorConversionTransformation::ConversionFlags) const override;
};

} // namespace

class KoColorSpaceFactorySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorSpaceFactoryTypeLifetimeAndIdentitySchemaRemainStable();
    void colorSpaceFactoryCapabilitySignaturesRemainStable();
    void colorSpaceFactoryProfileAndCreationSignaturesRemainStable();
    void colorSpaceFactoryConversionLinksAndTransformationLifetimeSchemaRemainStable();
    void colorConversionTransformationEndpointsSchemaRemainStable();
};

void KoColorSpaceFactorySchemaContractTest::colorSpaceFactoryTypeLifetimeAndIdentitySchemaRemainStable()
{
    static_assert(std::is_class_v<KoColorSpaceFactory>);
    static_assert(std::is_abstract_v<KoColorSpaceFactory>);
    static_assert(std::is_abstract_v<ColorSpaceFactoryConstructorProbe>);
    static_assert(std::has_virtual_destructor_v<KoColorSpaceFactory>);
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(id, QString (KoColorSpaceFactory::*)() const);
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(name, QString (KoColorSpaceFactory::*)() const);
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(userVisible, bool (KoColorSpaceFactory::*)() const);
}

void KoColorSpaceFactorySchemaContractTest::colorSpaceFactoryCapabilitySignaturesRemainStable()
{
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(colorDepthId, KoID (KoColorSpaceFactory::*)() const);
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(colorModelId, KoID (KoColorSpaceFactory::*)() const);
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(colorSpaceEngine, QString (KoColorSpaceFactory::*)() const);
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(crossingCost, int (KoColorSpaceFactory::*)() const);
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(isHdr, bool (KoColorSpaceFactory::*)() const);
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(referenceDepth, int (KoColorSpaceFactory::*)() const);
}

void KoColorSpaceFactorySchemaContractTest::colorSpaceFactoryProfileAndCreationSignaturesRemainStable()
{
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(createColorProfile,
                                         KoColorProfile * (KoColorSpaceFactory::*)(const QByteArray &) const);
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(defaultProfile, QString (KoColorSpaceFactory::*)() const);
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(grabColorSpace,
                                         const KoColorSpace *(KoColorSpaceFactory::*)(const KoColorProfile *));
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(profileIsCompatible,
                                         bool (KoColorSpaceFactory::*)(const KoColorProfile *) const);
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(requiredConnectionProfiles,
                                         QList<KoColorProfileQuery> (KoColorSpaceFactory::*)(const KoColorProfile *)
                                             const);
}

void KoColorSpaceFactorySchemaContractTest::
    colorSpaceFactoryConversionLinksAndTransformationLifetimeSchemaRemainStable()
{
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(colorConversionLinks,
                                         QList<KoColorConversionTransformationFactory *> (KoColorSpaceFactory::*)()
                                             const);
    ASSERT_COLOR_SPACE_FACTORY_SIGNATURE(
        colorConversionLinksFromProfile,
        QList<KoColorConversionTransformationFactory *> (KoColorSpaceFactory::*)(const KoColorProfile *) const);

    using ConversionFactory = KoColorConversionTransformationFactory;
    using ConstructorProbe = ColorConversionFactoryConstructorProbe;
    static_assert(std::is_class_v<ConversionFactory>);
    static_assert(std::is_constructible_v<ConstructorProbe,
                                          const QString &,
                                          const QString &,
                                          const QString &,
                                          const QString &,
                                          const QString &,
                                          const QString &>);
    static_assert(std::has_virtual_destructor_v<ConversionFactory>);
}

void KoColorSpaceFactorySchemaContractTest::colorConversionTransformationEndpointsSchemaRemainStable()
{
    using Factory = KoColorConversionTransformationFactory;
    ASSERT_COLOR_CONVERSION_FACTORY_SIGNATURE(dstColorDepthId, QString (Factory::*)() const);
    ASSERT_COLOR_CONVERSION_FACTORY_SIGNATURE(dstColorModelId, QString (Factory::*)() const);
    ASSERT_COLOR_CONVERSION_FACTORY_SIGNATURE(dstProfile, QString (Factory::*)() const);
    ASSERT_COLOR_CONVERSION_FACTORY_SIGNATURE(srcColorDepthId, QString (Factory::*)() const);
    ASSERT_COLOR_CONVERSION_FACTORY_SIGNATURE(srcColorModelId, QString (Factory::*)() const);
    ASSERT_COLOR_CONVERSION_FACTORY_SIGNATURE(srcProfile, QString (Factory::*)() const);
}

QTEST_GUILESS_MAIN(KoColorSpaceFactorySchemaContractTest)

#include "KoColorSpaceFactorySchemaContractTest.moc"
