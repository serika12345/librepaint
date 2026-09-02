/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <resources/KoStopGradient.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_STOP_GRADIENT_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoStopGradient::method)), signature>)
} // namespace

class KoStopGradientSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void gradientStopTypeSchemaRemainsStable();
    void gradientStopValueSchemaRemainsStable();
    void stopGradientLifetimeAndResourceSchemaRemainsStable();
    void stopGradientCollectionAndVariableColorSchemaRemainsStable();
    void stopGradientConversionAndPersistenceSchemaRemainsStable();
};

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

QTEST_GUILESS_MAIN(KoStopGradientSchemaContractTest)

#include "KoStopGradientSchemaContractTest.moc"
