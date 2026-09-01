/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorProfile.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_KO_COLOR_PROFILE_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoColorProfile::method)), signature>)
} // namespace

class KoColorProfileSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void profileIdentityAndLifetimeSignaturesRemainStable();
    void profileSuitabilityAndIntentSignaturesRemainStable();
    void profileChromaticitySignaturesRemainStable();
    void profileTransferFunctionSignaturesRemainStable();
    void profileSerializationSignaturesRemainStable();
};

void KoColorProfileSchemaContractTest::profileIdentityAndLifetimeSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoColorProfile>);
    static_assert(std::is_abstract_v<KoColorProfile>);
    static_assert(std::is_destructible_v<KoColorProfile>);
    static_assert(std::has_virtual_destructor_v<KoColorProfile>);

    ASSERT_KO_COLOR_PROFILE_SIGNATURE(clone, KoColorProfile * (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(colorModelID, QString (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(copyright, QString (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(fileName, QString (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(info, QString (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(manufacturer, QString (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(name, QString (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(operator==, bool (KoColorProfile::*)(const KoColorProfile &) const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(setFileName, void (KoColorProfile::*)(const QString &));
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(type, QString (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(uniqueId, QByteArray (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(valid, bool (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(version, float (KoColorProfile::*)() const);
}

void KoColorProfileSchemaContractTest::profileSuitabilityAndIntentSignaturesRemainStable()
{
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(isSuitableForDisplay, bool (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(isSuitableForInput, bool (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(isSuitableForOutput, bool (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(isSuitableForPrinting, bool (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(isSuitableForWorkspace, bool (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(supportsAbsolute, bool (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(supportsPerceptual, bool (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(supportsRelative, bool (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(supportsSaturation, bool (KoColorProfile::*)() const);
}

void KoColorProfileSchemaContractTest::profileChromaticitySignaturesRemainStable()
{
    using Colorants = QList<KoColorimetryUtils::xy>;

    ASSERT_KO_COLOR_PROFILE_SIGNATURE(colorantsForType,
                                      void (*)(ColorPrimaries, KoColorimetryUtils::xy &, Colorants &, bool));
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(getColorPrimaries, ColorPrimaries (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(getColorPrimariesName, QString (*)(ColorPrimaries));
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(getColorantsXYZ, QVector<KoColorimetryUtils::XYZ> (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(getColorantsxyY, QVector<KoColorimetryUtils::xyY> (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(getWhitePointXYZ, KoColorimetryUtils::XYZ (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(getWhitePointxyY, KoColorimetryUtils::xyY (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(hasColorants, bool (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(hdrReferenceWhite, std::optional<double> (KoColorProfile::*)() const);

    static_assert(std::is_same_v<decltype(KoColorProfile::colorantsForType(std::declval<ColorPrimaries>(),
                                                                           std::declval<KoColorimetryUtils::xy &>(),
                                                                           std::declval<Colorants &>())),
                                 void>);
}

void KoColorProfileSchemaContractTest::profileTransferFunctionSignaturesRemainStable()
{
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(compareTRC, bool (KoColorProfile::*)(TransferCharacteristics, float) const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(delinearizeFloatValue, void (KoColorProfile::*)(QVector<qreal> &) const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(delinearizeFloatValueFast, void (KoColorProfile::*)(QVector<qreal> &) const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(getEstimatedTRC, QVector<qreal> (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(getTransferCharacteristicName, QString (*)(TransferCharacteristics));
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(getTransferCharacteristics, TransferCharacteristics (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(hasTRC, bool (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(isLinear, bool (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(linearizeFloatValue, void (KoColorProfile::*)(QVector<qreal> &) const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(linearizeFloatValueFast, void (KoColorProfile::*)(QVector<qreal> &) const);
}

void KoColorProfileSchemaContractTest::profileSerializationSignaturesRemainStable()
{
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(load, bool (KoColorProfile::*)());
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(rawData, QByteArray (KoColorProfile::*)() const);
    ASSERT_KO_COLOR_PROFILE_SIGNATURE(save, bool (KoColorProfile::*)(const QString &));
}

QTEST_GUILESS_MAIN(KoColorProfileSchemaContractTest)

#include "KoColorProfileSchemaContractTest.moc"
