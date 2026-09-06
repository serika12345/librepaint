/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_auto_brush_factory.h>
#include <kis_predefined_brush_factory.h>
#include <kis_text_brush_factory.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_AUTO_FACTORY_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAutoBrushFactory::method)), signature>)
#define ASSERT_PREDEFINED_FACTORY_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPredefinedBrushFactory::method)), signature>)
#define ASSERT_TEXT_FACTORY_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisTextBrushFactory::method)), signature>)
} // namespace

class KisConcreteBrushFactoriesSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void autoFactoryIdentityAndLifetimeSchemaRemainStable();
    void autoFactoryCreationAndSerializationSchemaRemainStable();
    void predefinedFactoryIdentitySchemaRemainsStable();
    void predefinedFactoryCreationAndSerializationSchemaRemainStable();
    void textFactoryIdentityAndLifetimeSchemaRemainStable();
    void textFactoryCreationAndSerializationSchemaRemainStable();
};

void KisConcreteBrushFactoriesSchemaContractTest::autoFactoryIdentityAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAutoBrushFactory>);
    static_assert(std::is_default_constructible_v<KisAutoBrushFactory>);
    static_assert(std::has_virtual_destructor_v<KisAutoBrushFactory>);
    ASSERT_AUTO_FACTORY_SIGNATURE(id, QString (KisAutoBrushFactory::*)() const);
}

void KisConcreteBrushFactoriesSchemaContractTest::autoFactoryCreationAndSerializationSchemaRemainStable()
{
    ASSERT_AUTO_FACTORY_SIGNATURE(
        createBrush,
        KoResourceLoadResult (KisAutoBrushFactory::*)(const QDomElement &, KisResourcesInterfaceSP));
    ASSERT_AUTO_FACTORY_SIGNATURE(
        createBrush,
        KoResourceLoadResult (KisAutoBrushFactory::*)(const KisBrushModel::BrushData &, KisResourcesInterfaceSP));
    ASSERT_AUTO_FACTORY_SIGNATURE(createBrush,
                                  KoResourceLoadResult (KisAutoBrushFactory::*)(const KisBrushModel::CommonData &,
                                                                                const KisBrushModel::AutoBrushData &,
                                                                                KisResourcesInterfaceSP));
    ASSERT_AUTO_FACTORY_SIGNATURE(
        createBrushModel,
        std::optional<KisBrushModel::BrushData> (KisAutoBrushFactory::*)(const QDomElement &, KisResourcesInterfaceSP));
    ASSERT_AUTO_FACTORY_SIGNATURE(
        toXML,
        void (KisAutoBrushFactory::*)(QDomDocument &, QDomElement &, const KisBrushModel::BrushData &));
}

void KisConcreteBrushFactoriesSchemaContractTest::predefinedFactoryIdentitySchemaRemainsStable()
{
    static_assert(std::is_class_v<KisPredefinedBrushFactory>);
    static_assert(std::is_constructible_v<KisPredefinedBrushFactory, const QString &>);
    ASSERT_PREDEFINED_FACTORY_SIGNATURE(id, QString (KisPredefinedBrushFactory::*)() const);
}

void KisConcreteBrushFactoriesSchemaContractTest::predefinedFactoryCreationAndSerializationSchemaRemainStable()
{
    ASSERT_PREDEFINED_FACTORY_SIGNATURE(
        createBrush,
        KoResourceLoadResult (KisPredefinedBrushFactory::*)(const KisBrushModel::BrushData &, KisResourcesInterfaceSP));
    ASSERT_PREDEFINED_FACTORY_SIGNATURE(
        createBrush,
        KoResourceLoadResult (KisPredefinedBrushFactory::*)(const QDomElement &, KisResourcesInterfaceSP));
    ASSERT_PREDEFINED_FACTORY_SIGNATURE(
        createBrushModel,
        std::optional<KisBrushModel::BrushData> (KisPredefinedBrushFactory::*)(const QDomElement &,
                                                                               KisResourcesInterfaceSP));
    ASSERT_PREDEFINED_FACTORY_SIGNATURE(
        loadFromBrushResource,
        void (*)(KisBrushModel::CommonData &, KisBrushModel::PredefinedBrushData &, KisBrushSP));
    ASSERT_PREDEFINED_FACTORY_SIGNATURE(
        toXML,
        void (KisPredefinedBrushFactory::*)(QDomDocument &, QDomElement &, const KisBrushModel::BrushData &));
}

void KisConcreteBrushFactoriesSchemaContractTest::textFactoryIdentityAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisTextBrushFactory>);
    static_assert(std::is_default_constructible_v<KisTextBrushFactory>);
    static_assert(std::has_virtual_destructor_v<KisTextBrushFactory>);
    ASSERT_TEXT_FACTORY_SIGNATURE(id, QString (KisTextBrushFactory::*)() const);
}

void KisConcreteBrushFactoriesSchemaContractTest::textFactoryCreationAndSerializationSchemaRemainStable()
{
    ASSERT_TEXT_FACTORY_SIGNATURE(
        createBrush,
        KoResourceLoadResult (KisTextBrushFactory::*)(const QDomElement &, KisResourcesInterfaceSP));
    ASSERT_TEXT_FACTORY_SIGNATURE(
        createBrush,
        KoResourceLoadResult (KisTextBrushFactory::*)(const KisBrushModel::BrushData &, KisResourcesInterfaceSP));
    ASSERT_TEXT_FACTORY_SIGNATURE(
        createBrushModel,
        std::optional<KisBrushModel::BrushData> (KisTextBrushFactory::*)(const QDomElement &, KisResourcesInterfaceSP));
    ASSERT_TEXT_FACTORY_SIGNATURE(
        toXML,
        void (KisTextBrushFactory::*)(QDomDocument &, QDomElement &, const KisBrushModel::BrushData &));
}

QTEST_GUILESS_MAIN(KisConcreteBrushFactoriesSchemaContractTest)

#include "KisConcreteBrushFactoriesSchemaContractTest.moc"
