/*
*  SPDX-FileCopyrightText: 2007 Cyrille Berger <cberger@cberger.net>
*  SPDX-FileCopyrightText: 2021 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
*
* SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "IccColorProfile.h"

#include <cmath>
#include <cstdint>
#include <limits>

#include <lcms2.h>

#include <QDebug>
#include <QFile>
#include <QSharedPointer>

#include <KoColorConversions.h>
#include <KoColorProfileQuery.h>
#include <kis_assert.h>

#include "LcmsColorProfileContainer.h"
#include "LcmsPredefinedPipelineFunctions.h"
#include "kis_dom_utils.h"

#include <KisLazyStorage.h>
#include <KisLazyValueWrapper.h>


struct IccColorProfile::Data::Private {
    QByteArray rawData;
};

IccColorProfile::Data::Data() 
    : d(new Private)
{
}
IccColorProfile::Data::Data(const QByteArray &rawData) 
    : d(new Private)
{
    d->rawData = rawData;
}

IccColorProfile::Data::~Data()
{
}

QByteArray IccColorProfile::Data::rawData()
{
    return d->rawData;
}

void IccColorProfile::Data::setRawData(const QByteArray &rawData)
{
    d->rawData = rawData;
}

IccColorProfile::Container::Container()
{
}

IccColorProfile::Container::~Container()
{
}

struct IccColorProfile::Private {
    struct ProfileInfo {
        QVector<KoChannelInfo::DoubleRange> uiMinMaxes;
        bool canCreateCyclicTransform = false;
    };

    using LazyProfileInfo = KisLazyStorage<KisLazyValueWrapper<ProfileInfo>, std::function<ProfileInfo()>>;

    struct Shared {
        QScopedPointer<IccColorProfile::Data> data;
        QScopedPointer<LcmsColorProfileContainer> lcmsProfile;
        LazyProfileInfo profileInfo = LazyProfileInfo(LazyProfileInfo::init_value_tag{}, {});

        Shared()
            : data(new IccColorProfile::Data())
        {
        }
    };

    Private()
        : shared(QSharedPointer<Shared>::create())
    {
    }
    QSharedPointer<Shared> shared;

    ProfileInfo calculateFloatUIMinMax() const;
};

IccColorProfile::IccColorProfile(const QString &fileName)
    : KoColorProfile(fileName), d(new Private)
{
}

IccColorProfile::IccColorProfile(const QByteArray &rawData)
    : KoColorProfile(QString()), d(new Private)
{
    setRawData(rawData);
    init();
}

IccColorProfile::IccColorProfile(const KoColorProfileQuery &query)
: KoColorProfile(QString()), d(new Private)
{
    d->shared = QSharedPointer<Private::Shared>::create();

    KIS_SAFE_ASSERT_RECOVER_RETURN(query.isValid());

    cmsCIExyY whitePoint;

    QList<KoColorimetryUtils::xy> modifiedColorants = query.rgbColorants;

    KoColorimetryUtils::xy wp = query.whitePoint;

    KoColorProfile::colorantsForType(query.primaries, wp, modifiedColorants, true);


    if (modifiedColorants.size()>=2) {
        whitePoint.x = wp.x;
        whitePoint.y = wp.y;
        whitePoint.Y = 1.0;
    }

    cmsToneCurve *mainCurve = LcmsColorProfileContainer::transferFunction(query.transfer);

    cmsCIExyYTRIPLE primaries;

    if (modifiedColorants.size()>0 && modifiedColorants.size() <= 3) {
        primaries = {{modifiedColorants[0].x, modifiedColorants[0].y, 1.0},
                     {modifiedColorants[1].x, modifiedColorants[1].y, 1.0},
                     {modifiedColorants[2].x, modifiedColorants[2].y, 1.0}};
    }

    cmsHPROFILE iccProfile = nullptr;

    if (query.isGrayscale()) {
        iccProfile = cmsCreateGrayProfile(&whitePoint, mainCurve);
        // cleanup
        cmsFreeToneCurve(mainCurve);
    } else /*if (colorants.size()>2 || colorPrimariesType != 2)*/ {
        // generate rgb profile.
        cmsToneCurve *curve[3];
        curve[0] = curve[1] = curve[2] = mainCurve;
        iccProfile = cmsCreateRGBProfile(&whitePoint, &primaries, curve);
        cmsFreeToneCurve(mainCurve);
    }

    if (!iccProfile) {
        qWarning() << "WARNING: LCMS failed to create a profile for the requested parameters";
        qWarning().nospace() << "    transfer function: " << getTransferCharacteristicName(query.transfer) << " (" << query.transfer << ")";
        qWarning().nospace() << "    named primaries:" << getColorPrimariesName(query.primaries) << " (" << query.primaries << ")";
        qWarning() << "    requested colorants:" << query.rgbColorants;
        // leave the profile in invalid state and return
        return;
    }

    QStringList name;
    name.append("LibrePaint");
    if (query.primaries == PRIMARIES_ITU_R_BT_2020_2_AND_2100_0
        && (query.transfer == TRC_ITU_R_BT_2100_0_PQ || query.transfer == TRC_ITU_R_BT_2100_0_HLG)) {
        name.append(QStringLiteral("Rec. 2100"));
    } else {
        name.append(KoColorProfile::getColorPrimariesName(query.primaries));
    }
    name.append(KoColorProfile::getTransferCharacteristicName(query.transfer));

    cmsCIEXYZ media_blackpoint = {0.0, 0.0, 0.0};
    cmsWriteTag (iccProfile, cmsSigMediaBlackPointTag, &media_blackpoint);

    if (query.transfer == TRC_ITU_R_BT_2100_0_PQ && query.primaries != PRIMARIES_UNSPECIFIED) {
        double nits = query.hdrReferenceWhite? *query.hdrReferenceWhite: 203.0;
        LcmsPredefinedPipelineFunctions::setPerceptualQuantizerAToBDummyPipeline(iccProfile, cmsSigAToB0Tag, query.primaries, nits);
        LcmsPredefinedPipelineFunctions::setPerceptualQuantizerBToADummyPipeline(iccProfile, cmsSigBToA0Tag, query.primaries, nits);
        LcmsPredefinedPipelineFunctions::setDiffuseWhitePerceptualQuantizer(iccProfile, nits);
        // Otherwise this is recognised as a matrix shaper...
        cmsWriteTag(iccProfile, cmsSigRedColorantTag, nullptr);
        cmsWriteTag(iccProfile, cmsSigGreenColorantTag, nullptr);
        cmsWriteTag(iccProfile, cmsSigBlueColorantTag, nullptr);
        cmsWriteTag(iccProfile, cmsSigRedTRCTag, nullptr);
        cmsWriteTag(iccProfile, cmsSigGreenTRCTag, nullptr);
        cmsWriteTag(iccProfile, cmsSigBlueTRCTag, nullptr);

        name.append("("+KisDomUtils::toString(nits)+"cd/m²)");

    }

    // we need to not write when primaries are unspecified, because otherwise kocolorprofile's own matching mechanism breaks.
    if (query.primaries < 256 && query.primaries != PRIMARIES_UNSPECIFIED && query.transfer < 256) {
        cmsVideoSignalType cicpValues;
        cicpValues.ColourPrimaries = quint8(query.primaries);
        cicpValues.TransferCharacteristics = quint8(query.transfer);
        cicpValues.MatrixCoefficients = 0;
        cicpValues.VideoFullRangeFlag = 0; // According to the H.273 spec, 0 is the default value.
        cmsWriteTag (iccProfile, cmsSigcicpTag, &cicpValues);
    }

    //set the color profile info on the iccProfile;
    cmsMLU *mlu;
    mlu = cmsMLUalloc (NULL, 1);
    cmsMLUsetWide(mlu, "en", "US", name.join(" ").toStdWString().data());
    cmsWriteTag (iccProfile, cmsSigProfileDescriptionTag, mlu);
    cmsMLUfree (mlu);
    mlu = cmsMLUalloc (NULL, 1);
    cmsMLUsetASCII (mlu, "en", "US", QString("Profile generated by LibrePaint, Public domain.").toLatin1());
    cmsWriteTag(iccProfile, cmsSigCopyrightTag, mlu);
    cmsMLUfree (mlu);

    // Still setting this, as it also writes the non H.273 values.
    if (query.primaries != PRIMARIES_UNSPECIFIED) {
        setCharacteristics(query.primaries, query.transfer);
    }

    setRawData(LcmsColorProfileContainer::lcmsProfileToByteArray(iccProfile));
    cmsCloseProfile(iccProfile);

    setFileName(name.join(" ").remove("(").remove(")").replace("cd/m²", "nits").split(" ").join("-")+".icc");
    init();
}

IccColorProfile::IccColorProfile(const IccColorProfile &rhs)
    : KoColorProfile(rhs)
    , d(new Private(*rhs.d))
{
    Q_ASSERT(d->shared);
}

IccColorProfile::~IccColorProfile()
{
    Q_ASSERT(d->shared);
}

KoColorProfile *IccColorProfile::clone() const
{
    return new IccColorProfile(*this);
}

QByteArray IccColorProfile::rawData() const
{
    return d->shared->data->rawData();
}

void IccColorProfile::setRawData(const QByteArray &rawData)
{
    d->shared->data->setRawData(rawData);
}

bool IccColorProfile::valid() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->valid();
    }
    return false;
}
float IccColorProfile::version() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->version();
    }
    return 0.0;
}

QString IccColorProfile::colorModelID() const
{
    QString model;

    switch (d->shared->lcmsProfile->colorSpaceSignature()) {
    case cmsSigRgbData:
        model = "RGBA";
        break;
    case cmsSigLabData:
        model = "LABA";
        break;
    case cmsSigCmykData:
        model = "CMYKA";
        break;
    case cmsSigGrayData:
        model = "GRAYA";
        break;
    case cmsSigXYZData:
        model = "XYZA";
        break;
    case cmsSigYCbCrData:
        model = "YCbCrA";
        break;
    default:
        // In theory we should be able to interpret the colorspace signature as a 4 char array...
        model = QString();
    }

    return model;
}
bool IccColorProfile::isSuitableForOutput() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->isSuitableForOutput() && d->shared->profileInfo->value.canCreateCyclicTransform;
    }
    return false;
}
bool IccColorProfile::isSuitableForInput() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->isSuitableForInput() && d->shared->profileInfo->value.canCreateCyclicTransform;
    }
    return false;
}
bool IccColorProfile::isSuitableForWorkspace() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->isSuitableForWorkspace() && d->shared->profileInfo->value.canCreateCyclicTransform;
    }
    return false;
}


bool IccColorProfile::isSuitableForPrinting() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->isSuitableForPrinting();
    }
    return false;
}

bool IccColorProfile::isSuitableForDisplay() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->isSuitableForDisplay();
    }
    return false;
}

bool IccColorProfile::supportsPerceptual() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->supportsPerceptual();
    }
    return false;
}
bool IccColorProfile::supportsSaturation() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->supportsSaturation();
    }
    return false;
}
bool IccColorProfile::supportsAbsolute() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->supportsAbsolute();
    }
    return false;
}
bool IccColorProfile::supportsRelative() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->supportsRelative();
    }
    return false;
}
bool IccColorProfile::hasColorants() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->hasColorants();
    }
    return false;
}
bool IccColorProfile::hasTRC() const
{
    if (d->shared->lcmsProfile)
        return d->shared->lcmsProfile->hasTRC();
    return false;
}
bool IccColorProfile::isLinear() const
{
    if (d->shared->lcmsProfile)
        return d->shared->lcmsProfile->isLinear();
    return false;
}
QVector <KoColorimetryUtils::XYZ> IccColorProfile::getColorantsXYZ() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->getColorantsXYZ();
    }
    return QVector<KoColorimetryUtils::XYZ>();
}
QVector<KoColorimetryUtils::xyY> IccColorProfile::getColorantsxyY() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->getColorantsxyY();
    }
    return QVector<KoColorimetryUtils::xyY>();
}
KoColorimetryUtils::XYZ IccColorProfile::getWhitePointXYZ() const
{
    KoColorimetryUtils::XYZ d50Dummy {0.9642, 1.0000, 0.8249};
    if (d->shared->lcmsProfile) {
    return d->shared->lcmsProfile->getWhitePointXYZ();
    }
    return d50Dummy;
}
KoColorimetryUtils::xyY IccColorProfile::getWhitePointxyY() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->getWhitePointxyY();
    }
    return KoColorimetryUtils::xyY{0.34773, 0.35952, 1.0};
}

std::optional<double> IccColorProfile::hdrReferenceWhite() const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->hdrReferenceWhite();
    }
    return std::nullopt;
}
QVector <qreal> IccColorProfile::getEstimatedTRC() const
{
    QVector <qreal> dummy(3);
    dummy.fill(2.2);//estimated sRGB trc.
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->getEstimatedTRC();
    }
    return dummy;
}

bool IccColorProfile::compareTRC(TransferCharacteristics characteristics, float error) const
{
    if (d->shared->lcmsProfile) {
        return d->shared->lcmsProfile->compareTRC(characteristics, error);
    }
    return false;
}

void IccColorProfile::linearizeFloatValue(QVector <qreal> & Value) const
{
    if (d->shared->lcmsProfile)
        d->shared->lcmsProfile->LinearizeFloatValue(Value);
}
void IccColorProfile::delinearizeFloatValue(QVector <qreal> & Value) const
{
    if (d->shared->lcmsProfile)
        d->shared->lcmsProfile->DelinearizeFloatValue(Value);
}
void IccColorProfile::linearizeFloatValueFast(QVector <qreal> & Value) const
{
    if (d->shared->lcmsProfile)
        d->shared->lcmsProfile->LinearizeFloatValueFast(Value);
}
void IccColorProfile::delinearizeFloatValueFast(QVector<qreal> &Value) const
{
    if (d->shared->lcmsProfile)
        d->shared->lcmsProfile->DelinearizeFloatValueFast(Value);
}

QByteArray IccColorProfile::uniqueId() const
{
    QByteArray dummy;
    if (d->shared->lcmsProfile) {
        dummy = d->shared->lcmsProfile->getProfileUniqueId();
    }
    return dummy;
}

bool IccColorProfile::load()
{
    QFile file(fileName());
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray rawData = file.readAll();
        setRawData(rawData);
        file.close();
        if (init()) {
            return true;
        }
    }
    qWarning() << "Failed to load profile from " << fileName();
    return false;
}

bool IccColorProfile::save()
{
    return false;
}

bool IccColorProfile::init()
{
    if (!d->shared->lcmsProfile) {
        d->shared->lcmsProfile.reset(new LcmsColorProfileContainer(d->shared->data.data()));
    }
    if (d->shared->lcmsProfile->init()) {
        setName(d->shared->lcmsProfile->name());
        setInfo(d->shared->lcmsProfile->info());
        setManufacturer(d->shared->lcmsProfile->manufacturer());
        setCopyright(d->shared->lcmsProfile->copyright());
        if (d->shared->lcmsProfile->hasCicpValues()) {
            setCharacteristics(d->shared->lcmsProfile->cicpPrimaries(), d->shared->lcmsProfile->cicpTransfer());
        }
        if (d->shared->lcmsProfile->valid()) {
            d->shared->profileInfo = Private::LazyProfileInfo([this] () {
                return d->calculateFloatUIMinMax();
            });
        }
        return true;
    } else {
        return false;
    }
}

LcmsColorProfileContainer *IccColorProfile::asLcms() const
{
    Q_ASSERT(d->shared->lcmsProfile);
    return d->shared->lcmsProfile.data();
}

bool IccColorProfile::operator==(const KoColorProfile &rhs) const
{
    const IccColorProfile *rhsIcc = dynamic_cast<const IccColorProfile *>(&rhs);
    if (rhsIcc) {
        return d->shared == rhsIcc->d->shared;
    }
    return false;
}

const QVector<KoChannelInfo::DoubleRange> &IccColorProfile::getFloatUIMinMax(void) const
{
    Q_ASSERT(!d->shared->profileInfo->value.uiMinMaxes.isEmpty());
    return d->shared->profileInfo->value.uiMinMaxes;
}

IccColorProfile::Private::ProfileInfo
IccColorProfile::Private::calculateFloatUIMinMax() const
{
    Private::ProfileInfo info;
    QVector<KoChannelInfo::DoubleRange> &ret = info.uiMinMaxes;

    cmsHPROFILE cprofile = shared->lcmsProfile->lcmsProfile();
    Q_ASSERT(cprofile);

    cmsColorSpaceSignature color_space_sig = cmsGetColorSpace(cprofile);
    unsigned int num_channels = cmsChannelsOf(color_space_sig);
    unsigned int color_space_mask = _cmsLCMScolorSpace(color_space_sig);

    Q_ASSERT(num_channels >= 1 && num_channels <= 4); // num_channels==1 is for grayscale, we need to handle it
    Q_ASSERT(color_space_mask);

    // to try to find the max range of float/doubles for this profile,
    // pass in min/max int and make the profile convert that
    // this is far from perfect, we need a better way, if possible to get the "bounds" of a profile

    uint16_t in_min_pixel[4] = {0, 0, 0, 0};
    uint16_t in_max_pixel[4] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    qreal out_min_pixel[4] = {0, 0, 0, 0};
    qreal out_max_pixel[4] = {0, 0, 0, 0};

    cmsHTRANSFORM trans = cmsCreateTransform(
                              cprofile,
                              (COLORSPACE_SH(color_space_mask) | CHANNELS_SH(num_channels) | BYTES_SH(2)),
                              cprofile,
                              (COLORSPACE_SH(color_space_mask) | FLOAT_SH(1) | CHANNELS_SH(num_channels) | BYTES_SH(0)), //NOTE THAT 'BYTES' FIELD IS SET TO ZERO ON DLB because 8 bytes overflows the bitfield
                              INTENT_ABSOLUTE_COLORIMETRIC, 0);      // does the intent matter in this case?
                                                                     // absolute colorimetric gives bigger bounds with cmyk's Chemical Proof

    if (trans) {
        cmsDoTransform(trans, in_min_pixel, out_min_pixel, 1);
        cmsDoTransform(trans, in_max_pixel, out_max_pixel, 1);
        cmsDeleteTransform(trans);
    }//else, we'll just default to [0..1] below

    // Some (calibration) profiles may have a weird RGB->XYZ transformation matrix,
    // which is not invertible. Therefore, such profile cannot be used as
    // a workspace color profile and we should convert the image to sRGB
    // right on image loading

    // LCMS doesn't have a separate method for checking if conversion matrix
    // is invertible, therefore we just try to create a simple transformation,
    // where the profile is both, input and output. If the transformation
    // is created successfully, then this profile is probably suitable for
    // usage as a working color space.

    info.canCreateCyclicTransform = bool(trans);

    ret.resize(num_channels);
    for (unsigned int i = 0; i < num_channels; ++i) {
        if (color_space_sig == cmsSigYCbCrData) {
            // Although YCbCr profiles are essentially LUT-based
            // (due to the inability of ICC to represent multiple successive
            // matrix transforms except with BtoD0 tags in V4),
            // YCbCr is intended to be a roundtrip transform to the
            // corresponding RGB transform (BT.601, BT.709).
            // Force enable the full range of values.
            ret[i].minVal = 0;
            ret[i].maxVal = 1;
        } else if (out_min_pixel[i] < out_max_pixel[i]) {
            ret[i].minVal = out_min_pixel[i];
            ret[i].maxVal = out_max_pixel[i];
        } else {
            // apparently we can't even guarantee that converted_to_double(0x0000) < converted_to_double(0xFFFF)
            // assume [0..1] in such cases
            // we need to find a really solid way of determining the bounds of a profile, if possible
            ret[i].minVal = 0;
            ret[i].maxVal = 1;
        }
    }

    return info;
}
