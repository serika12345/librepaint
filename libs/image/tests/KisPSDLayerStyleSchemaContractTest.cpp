/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_asl_layer_style_serializer.h"
#include "kis_psd_layer_style.h"

#include <QTest>

#include <type_traits>

#define ASSERT_PSD_LAYER_STYLE_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPSDLayerStyle::method)), signature>)
#define ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(method, signature)                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAslLayerStyleSerializer::method)), signature>)

class KisPSDLayerStyleSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void psdLayerStyleTypeAndLifetimeSchemaRemainStable();
    void psdLayerStyleIdentityAndSerializationSignaturesRemainStable();
    void psdLayerStyleEffectAccessorsRemainStable();
    void psdLayerStyleStateAndResourceSignaturesRemainStable();
    void psdLayerStyleResourceSnapshotSignaturesRemainStable();
    void aslLayerStyleSerializerTypeAndStateSchemaRemainStable();
    void aslLayerStyleSerializerDeviceAndDocumentSignaturesRemainStable();
    void aslLayerStyleSerializerCollectionSignaturesRemainStable();
    void aslLayerStyleSerializerPatternAndGradientSignaturesRemainStable();
    void aslLayerStyleSerializerResourceLoadingSignaturesRemainStable();
};

void KisPSDLayerStyleSchemaContractTest::psdLayerStyleTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_same_v<KisPSDLayerStyleSP, QSharedPointer<KisPSDLayerStyle>>);
    static_assert(std::is_class_v<KisPSDLayerStyle>);
    static_assert(std::is_base_of_v<KoResource, KisPSDLayerStyle>);
    static_assert(std::is_constructible_v<KisPSDLayerStyle>);
    static_assert(std::is_constructible_v<KisPSDLayerStyle, const QString &, KisResourcesInterfaceSP>);
    static_assert(std::is_copy_constructible_v<KisPSDLayerStyle>);
    static_assert(std::has_virtual_destructor_v<KisPSDLayerStyle>);
    static_assert(!std::is_copy_assignable_v<KisPSDLayerStyle>);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(clone, KoResourceSP (KisPSDLayerStyle::*)() const);

    QVERIFY(true);
}

void KisPSDLayerStyleSchemaContractTest::psdLayerStyleIdentityAndSerializationSignaturesRemainStable()
{
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(registerResourceLoader, void (*)(KisResourceLoaderRegistry &));
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(isSerializable, bool (KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(loadFromDevice, bool (KisPSDLayerStyle::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(saveToDevice, bool (KisPSDLayerStyle::*)(QIODevice *) const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(clear, void (KisPSDLayerStyle::*)());
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(name, QString (KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(setName, void (KisPSDLayerStyle::*)(const QString &));
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(uuid, QUuid (KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(setUuid, void (KisPSDLayerStyle::*)(const QUuid &));
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(psdUuid, QString (KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(setPsdUuid, void (KisPSDLayerStyle::*)(const QString &));

    QVERIFY(true);
}

void KisPSDLayerStyleSchemaContractTest::psdLayerStyleEffectAccessorsRemainStable()
{
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(context, const psd_layer_effects_context *(KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(context, psd_layer_effects_context * (KisPSDLayerStyle::*)());
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(dropShadow, const psd_layer_effects_drop_shadow *(KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(dropShadow, psd_layer_effects_drop_shadow * (KisPSDLayerStyle::*)());
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(innerShadow, const psd_layer_effects_inner_shadow *(KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(innerShadow, psd_layer_effects_inner_shadow * (KisPSDLayerStyle::*)());
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(outerGlow, const psd_layer_effects_outer_glow *(KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(outerGlow, psd_layer_effects_outer_glow * (KisPSDLayerStyle::*)());
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(innerGlow, const psd_layer_effects_inner_glow *(KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(innerGlow, psd_layer_effects_inner_glow * (KisPSDLayerStyle::*)());
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(satin, const psd_layer_effects_satin *(KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(satin, psd_layer_effects_satin * (KisPSDLayerStyle::*)());
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(colorOverlay,
                                     const psd_layer_effects_color_overlay *(KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(colorOverlay, psd_layer_effects_color_overlay * (KisPSDLayerStyle::*)());
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(gradientOverlay,
                                     const psd_layer_effects_gradient_overlay *(KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(gradientOverlay, psd_layer_effects_gradient_overlay * (KisPSDLayerStyle::*)());
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(patternOverlay,
                                     const psd_layer_effects_pattern_overlay *(KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(patternOverlay, psd_layer_effects_pattern_overlay * (KisPSDLayerStyle::*)());
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(stroke, const psd_layer_effects_stroke *(KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(stroke, psd_layer_effects_stroke * (KisPSDLayerStyle::*)());
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(bevelAndEmboss,
                                     const psd_layer_effects_bevel_emboss *(KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(bevelAndEmboss, psd_layer_effects_bevel_emboss * (KisPSDLayerStyle::*)());

    QVERIFY(true);
}

void KisPSDLayerStyleSchemaContractTest::psdLayerStyleStateAndResourceSignaturesRemainStable()
{
    using ResourceTypeSignature = QPair<QString, QString> (KisPSDLayerStyle::*)() const;

    ASSERT_PSD_LAYER_STYLE_SIGNATURE(resourceType, ResourceTypeSignature);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(isEmpty, bool (KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(isEnabled, bool (KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(setEnabled, void (KisPSDLayerStyle::*)(bool));
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(resourcesInterface, KisResourcesInterfaceSP (KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(setResourcesInterface, void (KisPSDLayerStyle::*)(KisResourcesInterfaceSP));
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(requiredCanvasResources, QList<int> (KisPSDLayerStyle::*)() const);

    QVERIFY(true);
}

void KisPSDLayerStyleSchemaContractTest::psdLayerStyleResourceSnapshotSignaturesRemainStable()
{
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(hasLocalResourcesSnapshot, bool (KisPSDLayerStyle::*)() const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(
        cloneWithResourcesSnapshot,
        KisPSDLayerStyleSP (KisPSDLayerStyle::*)(KisResourcesInterfaceSP, KoCanvasResourcesInterfaceSP) const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(linkedResources,
                                     QList<KoResourceLoadResult> (KisPSDLayerStyle::*)(KisResourcesInterfaceSP) const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(sideLoadedResources,
                                     QList<KoResourceLoadResult> (KisPSDLayerStyle::*)(KisResourcesInterfaceSP) const);
    ASSERT_PSD_LAYER_STYLE_SIGNATURE(clearSideLoadedResources, void (KisPSDLayerStyle::*)());

    QVERIFY(true);
}

void KisPSDLayerStyleSchemaContractTest::aslLayerStyleSerializerTypeAndStateSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAslLayerStyleSerializer>);
    static_assert(std::is_default_constructible_v<KisAslLayerStyleSerializer>);
    static_assert(std::is_destructible_v<KisAslLayerStyleSerializer>);
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(isInitialized, bool (KisAslLayerStyleSerializer::*)());
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(isValid, bool (KisAslLayerStyleSerializer::*)());

    QVERIFY(true);
}

void KisPSDLayerStyleSchemaContractTest::aslLayerStyleSerializerDeviceAndDocumentSignaturesRemainStable()
{
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(saveToDevice, void (KisAslLayerStyleSerializer::*)(QIODevice &));
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(saveToFile, bool (KisAslLayerStyleSerializer::*)(const QString &));
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(readFromDevice, void (KisAslLayerStyleSerializer::*)(QIODevice &));
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(readFromFile, bool (KisAslLayerStyleSerializer::*)(const QString &));
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(formXmlDocument, QDomDocument (KisAslLayerStyleSerializer::*)() const);
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(formPsdXmlDocument,
                                                QDomDocument (KisAslLayerStyleSerializer::*)() const);

    QVERIFY(true);
}

void KisPSDLayerStyleSchemaContractTest::aslLayerStyleSerializerCollectionSignaturesRemainStable()
{
    using StylesHash = QHash<QString, KisPSDLayerStyleSP>;

    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(assignAllLayerStylesToLayers,
                                                void (KisAslLayerStyleSerializer::*)(KisNodeSP, const QString &));
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(collectAllLayerStyles, QVector<KisPSDLayerStyleSP> (*)(KisNodeSP));
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(styles,
                                                QVector<KisPSDLayerStyleSP> (KisAslLayerStyleSerializer::*)() const);
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(
        setStyles,
        void (KisAslLayerStyleSerializer::*)(const QVector<KisPSDLayerStyleSP> &));
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(stylesHash, StylesHash (KisAslLayerStyleSerializer::*)());

    QVERIFY(true);
}

void KisPSDLayerStyleSchemaContractTest::aslLayerStyleSerializerPatternAndGradientSignaturesRemainStable()
{
    using PatternsHash = QHash<QString, KoPatternSP>;

    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(patterns, PatternsHash (KisAslLayerStyleSerializer::*)() const);
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(gradients,
                                                QVector<KoAbstractGradientSP> (KisAslLayerStyleSerializer::*)() const);
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(registerPSDPattern,
                                                void (KisAslLayerStyleSerializer::*)(const QDomDocument &));

    QVERIFY(true);
}

void KisPSDLayerStyleSchemaContractTest::aslLayerStyleSerializerResourceLoadingSignaturesRemainStable()
{
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(readFromPSDXML,
                                                void (KisAslLayerStyleSerializer::*)(const QDomDocument &));
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(sideLoadLinkedResources,
                                                void (*)(KisPSDLayerStyle *, KisResourcesInterfaceSP));
    ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE(fetchLinkedResourceSignatures,
                                                QVector<KoResourceSignature> (*)(const KisPSDLayerStyle *));

    QVERIFY(true);
}

#undef ASSERT_ASL_LAYER_STYLE_SERIALIZER_SIGNATURE
#undef ASSERT_PSD_LAYER_STYLE_SIGNATURE

QTEST_GUILESS_MAIN(KisPSDLayerStyleSchemaContractTest)

#include "KisPSDLayerStyleSchemaContractTest.moc"
