/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <resources/KisGradientConversion.h>

#include <resources/KoAbstractGradient.h>
#include <resources/KoStopGradient.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_STOP_GRADIENT_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoStopGradient::method)), signature>)

#define ASSERT_ABSTRACT_GRADIENT_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoAbstractGradient::method)), signature>)

#define ASSERT_GRADIENT_CONVERSION_SIGNATURE(method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGradientConversion::method)), signature>)

class AbstractGradientConstructionProbe final : public KoAbstractGradient
{
public:
    using KoAbstractGradient::KoAbstractGradient;

    KoResourceSP clone() const override;
    bool loadFromDevice(QIODevice *, KisResourcesInterfaceSP) override;
    QPair<QString, QString> resourceType() const override;
};
} // namespace

class KoStopGradientSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void abstractGradientTypeAndLifetimeSchemaRemainStable();
    void abstractGradientColorSpaceSignaturesRemainStable();
    void abstractGradientPresentationSignaturesRemainStable();
    void abstractGradientVariableColorSignaturesRemainStable();
    void abstractGradientDebugSignatureRemainsStable();
    void gradientStopTypeSchemaRemainsStable();
    void gradientStopValueSchemaRemainsStable();
    void stopGradientLifetimeAndResourceSchemaRemainsStable();
    void stopGradientCollectionAndVariableColorSchemaRemainsStable();
    void stopGradientConversionAndPersistenceSchemaRemainsStable();
    void gradientConversionToQGradientStopsSignaturesRemainStable();
    void gradientConversionToQGradientSignaturesRemainStable();
    void gradientConversionToAbstractGradientSignaturesRemainStable();
    void gradientConversionToStopGradientSignaturesRemainStable();
    void gradientConversionToSegmentGradientSignaturesRemainStable();
};

void KoStopGradientSchemaContractTest::abstractGradientTypeAndLifetimeSchemaRemainStable()
{
    using Gradient = KoAbstractGradient;
    using Probe = AbstractGradientConstructionProbe;

    static_assert(std::is_class_v<Gradient>);
    static_assert(std::is_base_of_v<KoResource, Gradient>);
    static_assert(std::is_abstract_v<Gradient>);
    static_assert(!std::is_abstract_v<Probe>);
    static_assert(std::is_constructible_v<Probe, const QString &>);
    static_assert(std::is_copy_constructible_v<Probe>);
    static_assert(std::has_virtual_destructor_v<Gradient>);
    static_assert(std::is_destructible_v<Gradient>);
}

void KoStopGradientSchemaContractTest::abstractGradientColorSpaceSignaturesRemainStable()
{
    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(colorAt, void (KoAbstractGradient::*)(KoColor &, qreal) const);
    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(colorSpace, const KoColorSpace *(KoAbstractGradient::*)() const);
    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(setColorSpace, void (KoAbstractGradient::*)(KoColorSpace *));
}

void KoStopGradientSchemaContractTest::abstractGradientPresentationSignaturesRemainStable()
{
    using GeneratePreviewSignature = QImage (KoAbstractGradient::*)(int, int) const;
    using GeneratePreviewWithResourcesSignature =
        QImage (KoAbstractGradient::*)(int, int, KoCanvasResourcesInterfaceSP) const;

    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(generatePreview, GeneratePreviewSignature);
    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(generatePreview, GeneratePreviewWithResourcesSignature);
    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(setSpread, void (KoAbstractGradient::*)(QGradient::Spread));
    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(spread, QGradient::Spread (KoAbstractGradient::*)() const);
    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(setType, void (KoAbstractGradient::*)(QGradient::Type));
    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(type, QGradient::Type (KoAbstractGradient::*)() const);
    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(toQGradient, QGradient * (KoAbstractGradient::*)() const);
    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(updatePreview, void (KoAbstractGradient::*)());
}

void KoStopGradientSchemaContractTest::abstractGradientVariableColorSignaturesRemainStable()
{
    using CloneSignature = KoAbstractGradientSP (KoAbstractGradient::*)(KoCanvasResourcesInterfaceSP) const;
    using UpdateSignature = void (KoAbstractGradient::*)(KoCanvasResourcesInterfaceSP);

    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(bakeVariableColors, UpdateSignature);
    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(cloneAndBakeVariableColors, CloneSignature);
    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(cloneAndUpdateVariableColors, CloneSignature);
    ASSERT_ABSTRACT_GRADIENT_SIGNATURE(updateVariableColors, UpdateSignature);
}

void KoStopGradientSchemaContractTest::abstractGradientDebugSignatureRemainsStable()
{
    using DebugSignature = QDebug (*)(QDebug, KoAbstractGradientSP);

    static_assert(std::is_same_v<decltype(static_cast<DebugSignature>(&operator<<)), DebugSignature>);
}

void KoStopGradientSchemaContractTest::gradientStopTypeSchemaRemainsStable()
{
    static_assert(std::is_same_v<KoStopGradientSP, QSharedPointer<KoStopGradient>>);
    static_assert(std::is_class_v<KoStopGradient>);
    static_assert(std::is_enum_v<KoGradientStopType>);
    static_assert(COLORSTOP == 0);
    static_assert(FOREGROUNDSTOP == 1);
    static_assert(BACKGROUNDSTOP == 2);
    static_assert(std::is_class_v<KoGradientStop>);
    static_assert(std::is_class_v<KoGradientStopHueSort>);
    static_assert(std::is_class_v<KoGradientStopValueSort>);
}

void KoStopGradientSchemaContractTest::gradientStopValueSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&KoGradientStop::color), KoColor KoGradientStop::*>);
    static_assert(std::is_same_v<decltype(&KoGradientStop::position), qreal KoGradientStop::*>);
    static_assert(std::is_same_v<decltype(&KoGradientStop::type), KoGradientStopType KoGradientStop::*>);
    static_assert(std::is_constructible_v<KoGradientStop>);
    static_assert(std::is_constructible_v<KoGradientStop, qreal, KoColor, KoGradientStopType>);

    using EqualitySignature = bool (KoGradientStop::*)(const KoGradientStop &) const;
    using TypeFromStringSignature = KoGradientStopType (*)(QString);
    using TypeStringSignature = QString (KoGradientStop::*)() const;
    using SortSignature = bool (KoGradientStopHueSort::*)(const KoGradientStop &, const KoGradientStop &);
    using ValueSortSignature = bool (KoGradientStopValueSort::*)(const KoGradientStop &, const KoGradientStop &);

    static_assert(
        std::is_same_v<decltype(static_cast<EqualitySignature>(&KoGradientStop::operator==)), EqualitySignature>);
    static_assert(std::is_same_v<decltype(static_cast<TypeFromStringSignature>(&KoGradientStop::typeFromString)),
                                 TypeFromStringSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<TypeStringSignature>(&KoGradientStop::typeString)), TypeStringSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SortSignature>(&KoGradientStopHueSort::operator())), SortSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ValueSortSignature>(&KoGradientStopValueSort::operator())),
                                 ValueSortSignature>);
}

void KoStopGradientSchemaContractTest::stopGradientLifetimeAndResourceSchemaRemainsStable()
{
    using ResourceTypeSignature = QPair<QString, QString> (KoStopGradient::*)() const;

    static_assert(std::is_base_of_v<KoAbstractGradient, KoStopGradient>);
    static_assert(std::is_default_constructible_v<KoStopGradient>);
    static_assert(std::is_constructible_v<KoStopGradient, const QString &>);
    static_assert(std::is_copy_constructible_v<KoStopGradient>);
    static_assert(std::has_virtual_destructor_v<KoStopGradient>);
    static_assert(!std::is_copy_assignable_v<KoStopGradient>);

    ASSERT_STOP_GRADIENT_SIGNATURE(operator==, bool (KoStopGradient::*)(const KoStopGradient &) const);
    ASSERT_STOP_GRADIENT_SIGNATURE(clone, KoResourceSP (KoStopGradient::*)() const);
    ASSERT_STOP_GRADIENT_SIGNATURE(defaultFileExtension, QString (KoStopGradient::*)() const);
    ASSERT_STOP_GRADIENT_SIGNATURE(resourceType, ResourceTypeSignature);
}

void KoStopGradientSchemaContractTest::stopGradientCollectionAndVariableColorSchemaRemainsStable()
{
    ASSERT_STOP_GRADIENT_SIGNATURE(bakeVariableColors, void (KoStopGradient::*)(KoCanvasResourcesInterfaceSP));
    ASSERT_STOP_GRADIENT_SIGNATURE(colorAt, void (KoStopGradient::*)(KoColor &, qreal) const);
    ASSERT_STOP_GRADIENT_SIGNATURE(requiredCanvasResources, QList<int> (KoStopGradient::*)() const);
    ASSERT_STOP_GRADIENT_SIGNATURE(setStops, void (KoStopGradient::*)(QList<KoGradientStop>));
    ASSERT_STOP_GRADIENT_SIGNATURE(stops, QList<KoGradientStop> (KoStopGradient::*)() const);
    ASSERT_STOP_GRADIENT_SIGNATURE(stopsAt, bool (KoStopGradient::*)(KoGradientStop &, KoGradientStop &, qreal) const);
    ASSERT_STOP_GRADIENT_SIGNATURE(updateVariableColors, void (KoStopGradient::*)(KoCanvasResourcesInterfaceSP));
}

void KoStopGradientSchemaContractTest::stopGradientConversionAndPersistenceSchemaRemainsStable()
{
    using FromQGradientSignature = QSharedPointer<KoStopGradient> (*)(const QGradient *);
    using FromXmlSignature = KoStopGradient (*)(const QDomElement &);

    static_assert(std::is_same_v<decltype(static_cast<FromQGradientSignature>(&KoStopGradient::fromQGradient)),
                                 FromQGradientSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FromXmlSignature>(&KoStopGradient::fromXML)), FromXmlSignature>);
    ASSERT_STOP_GRADIENT_SIGNATURE(loadFromDevice, bool (KoStopGradient::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_STOP_GRADIENT_SIGNATURE(saveSvgGradient, QString (KoStopGradient::*)() const);
    ASSERT_STOP_GRADIENT_SIGNATURE(saveToDevice, bool (KoStopGradient::*)(QIODevice *) const);
    ASSERT_STOP_GRADIENT_SIGNATURE(toQGradient, QGradient * (KoStopGradient::*)() const);
    ASSERT_STOP_GRADIENT_SIGNATURE(toXML, void (KoStopGradient::*)(QDomDocument &, QDomElement &) const);
}

void KoStopGradientSchemaContractTest::gradientConversionToQGradientStopsSignaturesRemainStable()
{
    using FromAbstractSignature = QGradientStops (*)(KoAbstractGradientSP, KoCanvasResourcesInterfaceSP);
    using FromSegmentSignature = QGradientStops (*)(KoSegmentGradientSP, KoCanvasResourcesInterfaceSP);
    using FromStopSignature = QGradientStops (*)(KoStopGradientSP, KoCanvasResourcesInterfaceSP);

    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toQGradientStops, FromAbstractSignature);
    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toQGradientStops, FromSegmentSignature);
    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toQGradientStops, FromStopSignature);
    static_assert(
        std::is_same_v<decltype(KisGradientConversion::toQGradientStops(std::declval<KoAbstractGradientSP>())),
                       QGradientStops>);
    static_assert(std::is_same_v<decltype(KisGradientConversion::toQGradientStops(std::declval<KoSegmentGradientSP>())),
                                 QGradientStops>);
    static_assert(std::is_same_v<decltype(KisGradientConversion::toQGradientStops(std::declval<KoStopGradientSP>())),
                                 QGradientStops>);
}

void KoStopGradientSchemaContractTest::gradientConversionToQGradientSignaturesRemainStable()
{
    using FromAbstractSignature = QGradient *(*)(KoAbstractGradientSP, KoCanvasResourcesInterfaceSP);
    using FromSegmentSignature = QGradient *(*)(KoSegmentGradientSP, KoCanvasResourcesInterfaceSP);
    using FromStopSignature = QGradient *(*)(KoStopGradientSP, KoCanvasResourcesInterfaceSP);

    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toQGradient, FromAbstractSignature);
    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toQGradient, FromSegmentSignature);
    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toQGradient, FromStopSignature);
    static_assert(std::is_same_v<decltype(KisGradientConversion::toQGradient(std::declval<KoAbstractGradientSP>())),
                                 QGradient *>);
    static_assert(
        std::is_same_v<decltype(KisGradientConversion::toQGradient(std::declval<KoSegmentGradientSP>())), QGradient *>);
    static_assert(
        std::is_same_v<decltype(KisGradientConversion::toQGradient(std::declval<KoStopGradientSP>())), QGradient *>);
}

void KoStopGradientSchemaContractTest::gradientConversionToAbstractGradientSignaturesRemainStable()
{
    using FromSegmentSignature = KoAbstractGradientSP (*)(KoSegmentGradientSP);
    using FromStopSignature = KoAbstractGradientSP (*)(KoStopGradientSP);
    using FromQGradientSignature = KoAbstractGradientSP (*)(const QGradient *);
    using FromStopsSignature = KoAbstractGradientSP (*)(const QGradientStops &);

    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toAbstractGradient, FromSegmentSignature);
    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toAbstractGradient, FromStopSignature);
    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toAbstractGradient, FromQGradientSignature);
    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toAbstractGradient, FromStopsSignature);
}

void KoStopGradientSchemaContractTest::gradientConversionToStopGradientSignaturesRemainStable()
{
    using FromAbstractSignature = KoStopGradientSP (*)(KoAbstractGradientSP, KoCanvasResourcesInterfaceSP);
    using FromSegmentSignature = KoStopGradientSP (*)(KoSegmentGradientSP, KoCanvasResourcesInterfaceSP);
    using FromQGradientSignature = KoStopGradientSP (*)(const QGradient *);
    using FromStopsSignature = KoStopGradientSP (*)(const QGradientStops &);

    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toStopGradient, FromAbstractSignature);
    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toStopGradient, FromSegmentSignature);
    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toStopGradient, FromQGradientSignature);
    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toStopGradient, FromStopsSignature);
    static_assert(std::is_same_v<decltype(KisGradientConversion::toStopGradient(std::declval<KoAbstractGradientSP>())),
                                 KoStopGradientSP>);
    static_assert(std::is_same_v<decltype(KisGradientConversion::toStopGradient(std::declval<KoSegmentGradientSP>())),
                                 KoStopGradientSP>);
}

void KoStopGradientSchemaContractTest::gradientConversionToSegmentGradientSignaturesRemainStable()
{
    using FromAbstractSignature = KoSegmentGradientSP (*)(KoAbstractGradientSP);
    using FromStopSignature = KoSegmentGradientSP (*)(KoStopGradientSP);
    using FromQGradientSignature = KoSegmentGradientSP (*)(const QGradient *);
    using FromStopsSignature = KoSegmentGradientSP (*)(const QGradientStops &);

    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toSegmentGradient, FromAbstractSignature);
    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toSegmentGradient, FromStopSignature);
    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toSegmentGradient, FromQGradientSignature);
    ASSERT_GRADIENT_CONVERSION_SIGNATURE(toSegmentGradient, FromStopsSignature);
}

QTEST_GUILESS_MAIN(KoStopGradientSchemaContractTest)

#include "KoStopGradientSchemaContractTest.moc"
