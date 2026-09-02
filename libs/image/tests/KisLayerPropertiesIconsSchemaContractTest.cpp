/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_layer_properties_icons.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_LAYER_PROPERTY_ICON_SIGNATURE(method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisLayerPropertiesIcons::method)), signature>)

} // namespace

class KisLayerPropertiesIconsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void layerPropertyIconOwnerAndLifetimeSchemaRemainsStable();
    void layerPropertyVisibilityAndLockIdentifierSchemaRemainsStable();
    void layerPropertyPresentationIdentifierSchemaRemainsStable();
    void layerPropertyFactorySignaturesRemainStable();
    void layerPropertyNodeStorageAndPresentationSignaturesRemainStable();
};

void KisLayerPropertiesIconsSchemaContractTest::layerPropertyIconOwnerAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisLayerPropertiesIcons>);
    static_assert(std::is_default_constructible_v<KisLayerPropertiesIcons>);
    static_assert(std::is_destructible_v<KisLayerPropertiesIcons>);
    ASSERT_LAYER_PROPERTY_ICON_SIGNATURE(instance, KisLayerPropertiesIcons * (*)());

    QVERIFY(true);
}

void KisLayerPropertiesIconsSchemaContractTest::layerPropertyVisibilityAndLockIdentifierSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::locked), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::visible), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::inheritAlpha), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::alphaLocked), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::passThrough), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::selectionActive), const KoID>);

    QVERIFY(true);
}

void KisLayerPropertiesIconsSchemaContractTest::layerPropertyPresentationIdentifierSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::layerStyle), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::onionSkins), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::colorLabelIndex), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::colorOverlay), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::colorizeNeedsUpdate), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::colorizeEditKeyStrokes), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::colorizeShowColoring), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::openFileLayerFile), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::layerError), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::layerColorSpaceMismatch), const KoID>);
    static_assert(std::is_same_v<decltype(KisLayerPropertiesIcons::antialiased), const KoID>);

    QVERIFY(true);
}

void KisLayerPropertiesIconsSchemaContractTest::layerPropertyFactorySignaturesRemainStable()
{
    ASSERT_LAYER_PROPERTY_ICON_SIGNATURE(getProperty, KisBaseNode::Property (*)(const KoID &, bool));
    ASSERT_LAYER_PROPERTY_ICON_SIGNATURE(getProperty, KisBaseNode::Property (*)(const KoID &, bool, bool, bool));
    ASSERT_LAYER_PROPERTY_ICON_SIGNATURE(getErrorProperty, KisBaseNode::Property (*)(const QString &));
    ASSERT_LAYER_PROPERTY_ICON_SIGNATURE(getColorSpaceMismatchProperty,
                                         KisBaseNode::Property (*)(const KoColorSpace *));

    QVERIFY(true);
}

void KisLayerPropertiesIconsSchemaContractTest::layerPropertyNodeStorageAndPresentationSignaturesRemainStable()
{
    ASSERT_LAYER_PROPERTY_ICON_SIGNATURE(setNodePropertyAutoUndo,
                                         void (*)(KisNodeSP, const KoID &, const QVariant &, KisImageSP));
    ASSERT_LAYER_PROPERTY_ICON_SIGNATURE(setNodeProperty,
                                         void (*)(KisBaseNode::PropertyList *, const KoID &, const QVariant &));
    ASSERT_LAYER_PROPERTY_ICON_SIGNATURE(nodeProperty, QVariant (*)(KisNodeSP, const KoID &, const QVariant &));
    ASSERT_LAYER_PROPERTY_ICON_SIGNATURE(updateIcons, void (KisLayerPropertiesIcons::*)());
    ASSERT_LAYER_PROPERTY_ICON_SIGNATURE(isStatelessProperty, bool (*)(const QString &));

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisLayerPropertiesIconsSchemaContractTest)

#include "KisLayerPropertiesIconsSchemaContractTest.moc"
