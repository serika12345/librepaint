/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../colorprofiles/IccColorProfile.h"
#include "../colorprofiles/LcmsColorProfileContainer.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_LCMS_PROFILE_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&LcmsColorProfileContainer::method)), signature>)

#define ASSERT_ICC_SIGNATURE(owner, method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&owner::method)), signature>)

class IccContainerConstructionProbe final : public IccColorProfile::Container
{
public:
    IccContainerConstructionProbe() = default;
    ~IccContainerConstructionProbe() override = default;

    QString name() const override;
    QString info() const override;
    QString manufacturer() const override;
    QString copyright() const override;
    bool valid() const override;
    bool isSuitableForOutput() const override;
    bool isSuitableForInput() const override;
    bool isSuitableForWorkspace() const override;
    bool isSuitableForPrinting() const override;
    bool isSuitableForDisplay() const override;
    bool hasColorants() const override;
    QVector<KoColorimetryUtils::XYZ> getColorantsXYZ() const override;
    QVector<KoColorimetryUtils::xyY> getColorantsxyY() const override;
    std::optional<double> hdrReferenceWhite() const override;
    KoColorimetryUtils::XYZ getWhitePointXYZ() const override;
    KoColorimetryUtils::xyY getWhitePointxyY() const override;
    QVector<double> getEstimatedTRC() const override;
    bool compareTRC(TransferCharacteristics characteristics, float error) const override;
    QByteArray getProfileUniqueId() const override;
    bool hasCicpValues() const override;
    ColorPrimaries cicpPrimaries() const override;
    TransferCharacteristics cicpTransfer() const override;
};
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
    void iccProfileDataAndLifetimeSchemaRemainsStable();
    void iccContainerIdentityAndSuitabilitySchemaRemainsStable();
    void iccContainerColorimetrySchemaRemainsStable();
    void iccProfileIdentityPersistenceAndSuitabilitySchemaRemainsStable();
    void iccProfileColorimetryAndTransferSchemaRemainsStable();
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

void LcmsColorProfileContainerSchemaContractTest::iccProfileDataAndLifetimeSchemaRemainsStable()
{
    using Data = IccColorProfile::Data;

    static_assert(std::is_class_v<IccColorProfile>);
    static_assert(std::is_base_of_v<KoColorProfile, IccColorProfile>);
    static_assert(std::is_class_v<Data>);
    static_assert(std::is_default_constructible_v<Data>);
    static_assert(std::is_constructible_v<Data, const QByteArray &>);
    static_assert(std::is_destructible_v<Data>);
    ASSERT_ICC_SIGNATURE(Data, rawData, QByteArray (Data::*)());
    ASSERT_ICC_SIGNATURE(Data, setRawData, void (Data::*)(const QByteArray &));

    static_assert(std::is_copy_constructible_v<IccColorProfile>);
    static_assert(std::is_constructible_v<IccColorProfile, const KoColorProfileQuery &>);
    static_assert(std::is_constructible_v<IccColorProfile, const QByteArray &>);
    static_assert(std::is_constructible_v<IccColorProfile, const QString &>);
    static_assert(std::is_default_constructible_v<IccColorProfile>);
    static_assert(std::is_destructible_v<IccColorProfile>);
    static_assert(std::has_virtual_destructor_v<IccColorProfile>);
    ASSERT_ICC_SIGNATURE(IccColorProfile, clone, KoColorProfile * (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, asLcms, LcmsColorProfileContainer * (IccColorProfile::*)() const);

    QVERIFY(true);
}

void LcmsColorProfileContainerSchemaContractTest::iccContainerIdentityAndSuitabilitySchemaRemainsStable()
{
    using Container = IccColorProfile::Container;

    static_assert(std::is_class_v<Container>);
    static_assert(std::is_abstract_v<Container>);
    static_assert(std::is_default_constructible_v<IccContainerConstructionProbe>);
    static_assert(std::is_destructible_v<Container>);
    static_assert(std::has_virtual_destructor_v<Container>);
    ASSERT_ICC_SIGNATURE(Container, name, QString (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, info, QString (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, manufacturer, QString (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, copyright, QString (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, valid, bool (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, isSuitableForOutput, bool (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, isSuitableForInput, bool (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, isSuitableForWorkspace, bool (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, isSuitableForPrinting, bool (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, isSuitableForDisplay, bool (Container::*)() const);

    QVERIFY(true);
}

void LcmsColorProfileContainerSchemaContractTest::iccContainerColorimetrySchemaRemainsStable()
{
    using Container = IccColorProfile::Container;

    ASSERT_ICC_SIGNATURE(Container, cicpPrimaries, ColorPrimaries (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, cicpTransfer, TransferCharacteristics (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, compareTRC, bool (Container::*)(TransferCharacteristics, float) const);
    ASSERT_ICC_SIGNATURE(Container, getColorantsXYZ, QVector<KoColorimetryUtils::XYZ> (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, getColorantsxyY, QVector<KoColorimetryUtils::xyY> (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, getEstimatedTRC, QVector<double> (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, getProfileUniqueId, QByteArray (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, getWhitePointXYZ, KoColorimetryUtils::XYZ (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, getWhitePointxyY, KoColorimetryUtils::xyY (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, hasCicpValues, bool (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, hasColorants, bool (Container::*)() const);
    ASSERT_ICC_SIGNATURE(Container, hdrReferenceWhite, std::optional<double> (Container::*)() const);

    QVERIFY(true);
}

void LcmsColorProfileContainerSchemaContractTest::iccProfileIdentityPersistenceAndSuitabilitySchemaRemainsStable()
{
    ASSERT_ICC_SIGNATURE(IccColorProfile, colorModelID, QString (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile,
                         getFloatUIMinMax,
                         const QVector<KoChannelInfo::DoubleRange> &(IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, load, bool (IccColorProfile::*)());
    ASSERT_ICC_SIGNATURE(IccColorProfile, operator==, bool (IccColorProfile::*)(const KoColorProfile &) const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, rawData, QByteArray (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, save, bool (IccColorProfile::*)());
    ASSERT_ICC_SIGNATURE(IccColorProfile, type, QString (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, uniqueId, QByteArray (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, valid, bool (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, version, float (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, isSuitableForOutput, bool (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, isSuitableForInput, bool (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, isSuitableForWorkspace, bool (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, isSuitableForPrinting, bool (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, isSuitableForDisplay, bool (IccColorProfile::*)() const);

    QVERIFY(true);
}

void LcmsColorProfileContainerSchemaContractTest::iccProfileColorimetryAndTransferSchemaRemainsStable()
{
    ASSERT_ICC_SIGNATURE(IccColorProfile, compareTRC, bool (IccColorProfile::*)(TransferCharacteristics, float) const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, delinearizeFloatValue, void (IccColorProfile::*)(QVector<qreal> &) const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, delinearizeFloatValueFast, void (IccColorProfile::*)(QVector<qreal> &) const);
    ASSERT_ICC_SIGNATURE(IccColorProfile,
                         getColorantsXYZ,
                         QVector<KoColorimetryUtils::XYZ> (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile,
                         getColorantsxyY,
                         QVector<KoColorimetryUtils::xyY> (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, getEstimatedTRC, QVector<qreal> (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, getWhitePointXYZ, KoColorimetryUtils::XYZ (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, getWhitePointxyY, KoColorimetryUtils::xyY (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, hasColorants, bool (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, hasTRC, bool (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, hdrReferenceWhite, std::optional<double> (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, isLinear, bool (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, linearizeFloatValue, void (IccColorProfile::*)(QVector<qreal> &) const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, linearizeFloatValueFast, void (IccColorProfile::*)(QVector<qreal> &) const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, supportsAbsolute, bool (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, supportsPerceptual, bool (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, supportsRelative, bool (IccColorProfile::*)() const);
    ASSERT_ICC_SIGNATURE(IccColorProfile, supportsSaturation, bool (IccColorProfile::*)() const);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(LcmsColorProfileContainerSchemaContractTest)

#include "LcmsColorProfileContainerSchemaContractTest.moc"
