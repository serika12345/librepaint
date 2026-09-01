/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../colorprofiles/LcmsColorProfileContainer.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_LCMS_PROFILE_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&LcmsColorProfileContainer::method)), signature>)
} // namespace

class LcmsColorProfileContainerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void lcmsProfileTypeAndMetadataSchemaRemainsStable();
    void lcmsProfileRepresentationAndSuitabilitySchemaRemainsStable();
    void lcmsProfileIntentAndTransferClassificationSchemaRemainsStable();
    void lcmsProfileColorimetrySchemaRemainsStable();
    void lcmsProfileTransferOperationSchemaRemainsStable();
};

void LcmsColorProfileContainerSchemaContractTest::lcmsProfileTypeAndMetadataSchemaRemainsStable()
{
    static_assert(std::is_class_v<LcmsColorProfileContainer>);
    static_assert(std::is_base_of_v<IccColorProfile::Container, LcmsColorProfileContainer>);
    static_assert(std::is_destructible_v<LcmsColorProfileContainer>);
    static_assert(std::has_virtual_destructor_v<LcmsColorProfileContainer>);

    ASSERT_LCMS_PROFILE_SIGNATURE(valid, bool (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(version, float (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(name, QString (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(info, QString (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(manufacturer, QString (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(copyright, QString (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(getProfileUniqueId, QByteArray (LcmsColorProfileContainer::*)() const);

    QVERIFY(true);
}

void LcmsColorProfileContainerSchemaContractTest::lcmsProfileRepresentationAndSuitabilitySchemaRemainsStable()
{
    ASSERT_LCMS_PROFILE_SIGNATURE(createFromLcmsProfile, IccColorProfile * (*)(cmsHPROFILE));
    ASSERT_LCMS_PROFILE_SIGNATURE(colorSpaceSignature, cmsColorSpaceSignature (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(deviceClass, cmsProfileClassSignature (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(lcmsProfile, cmsHPROFILE (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(isSuitableForOutput, bool (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(isSuitableForInput, bool (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(isSuitableForWorkspace, bool (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(isSuitableForPrinting, bool (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(isSuitableForDisplay, bool (LcmsColorProfileContainer::*)() const);

    QVERIFY(true);
}

void LcmsColorProfileContainerSchemaContractTest::lcmsProfileIntentAndTransferClassificationSchemaRemainsStable()
{
    ASSERT_LCMS_PROFILE_SIGNATURE(supportsPerceptual, bool (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(supportsSaturation, bool (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(supportsAbsolute, bool (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(supportsRelative, bool (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(hasColorants, bool (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(hasTRC, bool (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(isLinear, bool (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(hasCicpValues, bool (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(cicpPrimaries, ColorPrimaries (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(cicpTransfer, TransferCharacteristics (LcmsColorProfileContainer::*)() const);

    QVERIFY(true);
}

void LcmsColorProfileContainerSchemaContractTest::lcmsProfileColorimetrySchemaRemainsStable()
{
    ASSERT_LCMS_PROFILE_SIGNATURE(getColorantsXYZ,
                                  QVector<KoColorimetryUtils::XYZ> (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(getColorantsxyY,
                                  QVector<KoColorimetryUtils::xyY> (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(getWhitePointXYZ, KoColorimetryUtils::XYZ (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(getWhitePointxyY, KoColorimetryUtils::xyY (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(hdrReferenceWhite, std::optional<double> (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(getEstimatedTRC, QVector<double> (LcmsColorProfileContainer::*)() const);
    ASSERT_LCMS_PROFILE_SIGNATURE(compareTRC,
                                  bool (LcmsColorProfileContainer::*)(TransferCharacteristics, float) const);

    QVERIFY(true);
}

void LcmsColorProfileContainerSchemaContractTest::lcmsProfileTransferOperationSchemaRemainsStable()
{
    ASSERT_LCMS_PROFILE_SIGNATURE(LinearizeFloatValue, void (LcmsColorProfileContainer::*)(QVector<double> &) const);
    ASSERT_LCMS_PROFILE_SIGNATURE(DelinearizeFloatValue, void (LcmsColorProfileContainer::*)(QVector<double> &) const);
    ASSERT_LCMS_PROFILE_SIGNATURE(LinearizeFloatValueFast,
                                  void (LcmsColorProfileContainer::*)(QVector<double> &) const);
    ASSERT_LCMS_PROFILE_SIGNATURE(DelinearizeFloatValueFast,
                                  void (LcmsColorProfileContainer::*)(QVector<double> &) const);
    ASSERT_LCMS_PROFILE_SIGNATURE(transferFunction, cmsToneCurve * (*)(TransferCharacteristics));

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(LcmsColorProfileContainerSchemaContractTest)

#include "LcmsColorProfileContainerSchemaContractTest.moc"
