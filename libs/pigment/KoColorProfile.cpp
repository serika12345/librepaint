/*
 *  SPDX-FileCopyrightText: 2007 Cyrille Berger <cberger@cberger.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <algorithm>
#include <array>
#include <cmath>

#include "KoColorProfile.h"
#include "kis_assert.h"

struct Q_DECL_HIDDEN KoColorProfile::Private {
    QString name;
    QString info;
    QString fileName;
    QString manufacturer;
    QString copyright;
    int primaries {-1};
    TransferCharacteristics characteristics {TRC_UNSPECIFIED};
};

KoColorProfile::KoColorProfile(const QString &fileName) : d(new Private)
{
//     dbgPigment <<" Profile filename =" << fileName;
    d->fileName = fileName;
}

KoColorProfile::KoColorProfile(const KoColorProfile& profile)
    : d(new Private(*profile.d))
{
}

KoColorProfile::~KoColorProfile()
{
    delete d;
}

bool KoColorProfile::load()
{
    return false;
}

bool KoColorProfile::save(const QString & filename)
{
    Q_UNUSED(filename);
    return false;
}


QString KoColorProfile::name() const
{
    return d->name;
}

QString KoColorProfile::info() const
{
    return d->info;
}
QString KoColorProfile::manufacturer() const
{
    return d->manufacturer;
}
QString KoColorProfile::copyright() const
{
    return d->copyright;
}
QString KoColorProfile::fileName() const
{
    return d->fileName;
}

void KoColorProfile::setFileName(const QString &f)
{
    d->fileName = f;
}

ColorPrimaries KoColorProfile::getColorPrimaries() const
{
    if (d->primaries == -1) {
        ColorPrimaries primaries = PRIMARIES_UNSPECIFIED;
        KoColorimetryUtils::xyY wp = getWhitePointxyY();

        bool match = false;
        if (hasColorants()) {
            QVector<KoColorimetryUtils::xyY> col = getColorantsxyY();
            if (col.size()<3) {
                KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(col.size() < 3, PRIMARIES_UNSPECIFIED);
                //too few colorants.
                d->primaries = int(primaries);
                return (primaries);
            }
            QVector<KoColorimetryUtils::xyY> colorants = col;
            colorants.append(wp);
            QList<KoColorimetryUtils::xy> compare;
            KoColorimetryUtils::xy wp2;

            QVector<ColorPrimaries> primariesList = {PRIMARIES_ITU_R_BT_709_5, PRIMARIES_ITU_R_BT_601_6, PRIMARIES_ITU_R_BT_470_6_SYSTEM_M,
                                                     PRIMARIES_ITU_R_BT_2020_2_AND_2100_0, PRIMARIES_SMPTE_EG_432_1, PRIMARIES_SMPTE_RP_431_2,
                                                     PRIMARIES_SMPTE_ST_428_1, PRIMARIES_GENERIC_FILM, PRIMARIES_SMPTE_240M, PRIMARIES_EBU_Tech_3213_E,
                                                     PRIMARIES_ADOBE_RGB_1998, PRIMARIES_PROPHOTO, PRIMARIES_ITU_R_BT_470_6_SYSTEM_B_G};

            for (ColorPrimaries check: primariesList) {
                colorantsForType(check, wp2, compare);
                if (compare.size() <3) {
                    KIS_SAFE_ASSERT_RECOVER(compare.size() < 3) { continue; }
                    //too few colorants, skip.
                }
                compare.append(wp2);
                match = true;
                for (int i=0; i<colorants.size(); i++) {
                    match = std::fabs(colorants[i].x - compare[i].x) < 0.0001
                        && std::fabs(colorants[i].y - compare[i].y) < 0.0001;
                    if (!match) {
                        break;
                    }
                }
                if (match) {
                    primaries = check;
                }
            }
        }

        d->primaries = int(primaries);
    }
    return ColorPrimaries(d->primaries);
}

QString KoColorProfile::getColorPrimariesName(ColorPrimaries primaries)
{
    switch (primaries) {
    case PRIMARIES_ITU_R_BT_709_5:
        return QStringLiteral("Rec. 709");
    case PRIMARIES_ITU_R_BT_470_6_SYSTEM_M:
        return QStringLiteral("BT. 470 System M");
    case PRIMARIES_ITU_R_BT_470_6_SYSTEM_B_G:
        return QStringLiteral("BT. 470 System B, G");
    case PRIMARIES_GENERIC_FILM:
        return QStringLiteral("Generic Film");
    case PRIMARIES_SMPTE_240M:
        return QStringLiteral("SMPTE 240 M");
    case PRIMARIES_ITU_R_BT_2020_2_AND_2100_0:
        return QStringLiteral("Rec. 2020");
    case PRIMARIES_ITU_R_BT_601_6:
        return QStringLiteral("Rec. 601");
    case PRIMARIES_SMPTE_EG_432_1:
        return QStringLiteral("Display P3");
    case PRIMARIES_SMPTE_RP_431_2:
        return QStringLiteral("DCI P3");
    case PRIMARIES_SMPTE_ST_428_1:
        return QStringLiteral("XYZ primaries");
    case PRIMARIES_EBU_Tech_3213_E:
        return QStringLiteral("EBU Tech 3213 E");
    case PRIMARIES_PROPHOTO:
        return QStringLiteral("ProPhoto");
    case PRIMARIES_ADOBE_RGB_1998:
        return QStringLiteral("A98");
    case PRIMARIES_UNSPECIFIED:
        break;
    }
    return QStringLiteral("Unspecified");
}

void KoColorProfile::colorantsForType(ColorPrimaries primaries,
                                      KoColorimetryUtils::xy &whitePoint,
                                      QList<KoColorimetryUtils::xy> &colorants,
                                      const bool prequantized)
{
    KoColorimetryUtils::Colorimetry c = KoColorimetryUtils::Colorimetry::BT709;
    switch (ColorPrimaries(primaries)) {
    case PRIMARIES_UNSPECIFIED:
        break;
    case PRIMARIES_ITU_R_BT_470_6_SYSTEM_M:
        // Unquantized.
        c = KoColorimetryUtils::Colorimetry::PAL_M;
        whitePoint = c.white().toxy();
        colorants = {c.red().toxy(), c.green().toxy(), c.blue().toxy()};
        //Illuminant C
        break;
    case PRIMARIES_ITU_R_BT_470_6_SYSTEM_B_G:
        // Unquantized.
        c = KoColorimetryUtils::Colorimetry::PAL;
        whitePoint = c.white().toxy();
        colorants = {c.red().toxy(), c.green().toxy(), c.blue().toxy()};
        break;
    case PRIMARIES_ITU_R_BT_601_6:
        c = KoColorimetryUtils::Colorimetry::NTSC;
        whitePoint = c.white().toxy();
        colorants = {c.red().toxy(), c.green().toxy(), c.blue().toxy()};
        break;
    case PRIMARIES_SMPTE_240M:
        whitePoint = {0.3127, 0.3290};
        colorants = {{0.630, 0.340}};
        colorants.append({0.310, 0.595});
        colorants.append({0.155, 0.070});
        break;
    case PRIMARIES_GENERIC_FILM:
        c = KoColorimetryUtils::Colorimetry::GenericFilm;
        whitePoint = c.white().toxy();
        colorants = {c.red().toxy(), c.green().toxy(), c.blue().toxy()};
        //Illuminant C
        break;
    case PRIMARIES_ITU_R_BT_2020_2_AND_2100_0:
        //prequantization courtesy of Elle Stone.

        if (prequantized) {
            whitePoint = {0.3127, 0.3290};
            colorants = {{0.708012540607, 0.291993664388}};
            colorants.append({0.169991652439, 0.797007778423});
            colorants.append({0.130997824007, 0.045996550894});
        } else {
            c = KoColorimetryUtils::Colorimetry::BT2020;
            whitePoint = c.white().toxy();
            colorants = {c.red().toxy(), c.green().toxy(), c.blue().toxy()};
        }
        break;
    case PRIMARIES_SMPTE_ST_428_1:
        c = KoColorimetryUtils::Colorimetry::CIEXYZ;
        whitePoint = c.white().toxy();
        colorants = {c.red().toxy(), c.green().toxy(), c.blue().toxy()};
        break;
    case PRIMARIES_SMPTE_RP_431_2:
        c = KoColorimetryUtils::Colorimetry::DCIP3;
        whitePoint = c.white().toxy();
        colorants = {c.red().toxy(), c.green().toxy(), c.blue().toxy()};
        break;
    case PRIMARIES_SMPTE_EG_432_1:
        c = KoColorimetryUtils::Colorimetry::DisplayP3;
        whitePoint = c.white().toxy();
        colorants = {c.red().toxy(), c.green().toxy(), c.blue().toxy()};
        break;
    case PRIMARIES_EBU_Tech_3213_E:
        whitePoint = {0.3127, 0.3290};
        colorants = {{0.63, 0.34}};
        colorants.append({0.295, 0.605});
        colorants.append({0.155, 0.077});
        break;
    case PRIMARIES_PROPHOTO:

        if (prequantized) {
            //prequantization courtesy of Elle Stone.
            whitePoint = {0.3457, 0.3585};
            colorants = {{0.7347, 0.2653}};
            colorants.append({0.1596, 0.8404});
            colorants.append({0.0366, 0.0001});
        } else {
            whitePoint = {0.3457, 0.3585};
            colorants = {{0.734699, 0.265301}};
            colorants.append({0.159597, 0.840403});
            colorants.append({0.036598, 0.000105});
        }

        break;
    case PRIMARIES_ADOBE_RGB_1998:
        if (prequantized) {
            //prequantization courtesy of Elle Stone.
            whitePoint = {0.3127, 0.3290};
            colorants = {{0.639996511, 0.329996864}};
            colorants.append({0.210005295, 0.710004866});
            colorants.append({0.149997606, 0.060003644});
        }  else {
            c = KoColorimetryUtils::Colorimetry::AdobeRGB;
            whitePoint = c.white().toxy();
            colorants = {c.red().toxy(), c.green().toxy(), c.blue().toxy()};
        }
        break;
    case PRIMARIES_ITU_R_BT_709_5:
    default:
        if (prequantized) {
            // Prequantized colorants, courtesy of Elle Stone
            whitePoint = {0.3127, 0.3290};
            colorants = {{0.639998686, 0.330010138}};
            colorants.append({0.300003784, 0.600003357});
            colorants.append({0.150002046, 0.059997204});
        }  else {
            c = KoColorimetryUtils::Colorimetry::BT709;
            whitePoint = c.white().toxy();
            colorants = {c.red().toxy(), c.green().toxy(), c.blue().toxy()};
        }
        break;

    }
}

TransferCharacteristics KoColorProfile::getTransferCharacteristics() const
{
    // Parse from an estimated gamma
    const QVector<double> estimatedTRC = getEstimatedTRC();
    const double error = 0.0001;
    // Make sure the TRC is uniform across all channels
    const bool isUniformTRC = (estimatedTRC[0] == estimatedTRC[1] && estimatedTRC[0] == estimatedTRC[2]);
    if (d->characteristics == TRC_UNSPECIFIED && isUniformTRC && hasTRC()) {
        if (isLinear()) {
            d->characteristics = TRC_LINEAR;
        } else if (std::fabs(estimatedTRC[0] - (461.0 / 256.0)) < error) {
            // ICC v2 u8Fixed8Number calculation
            // Or can be prequantized as 1.80078125, courtesy of Elle Stone
            d->characteristics = TRC_GAMMA_1_8;
        } else if (std::fabs(estimatedTRC[0] - (563.0 / 256.0)) < error) {
            // Or can be prequantized as 2.19921875, courtesy of Elle Stone
            d->characteristics = TRC_A98;
        } else if (std::fabs(estimatedTRC[0] - 1.8) < error) {
            d->characteristics = TRC_GAMMA_1_8;
        } else if (std::fabs(estimatedTRC[0] - 2.2) < error) {
            d->characteristics = TRC_ITU_R_BT_470_6_SYSTEM_M;
        } else if (std::fabs(estimatedTRC[0] - 2.4) < error) {
            d->characteristics = TRC_GAMMA_2_4;
        } else if (std::fabs(estimatedTRC[0] - 2.8) < error) {
            d->characteristics = TRC_ITU_R_BT_470_6_SYSTEM_B_G;
        } else {
            // Escort to curve matching if no gamma is matched
            static constexpr std::array<TransferCharacteristics, 12> trcList = {{TRC_ITU_R_BT_709_5,
                                                                                 TRC_ITU_R_BT_470_6_SYSTEM_M,
                                                                                 TRC_ITU_R_BT_470_6_SYSTEM_B_G,
                                                                                 TRC_SMPTE_240M,
                                                                                 TRC_IEC_61966_2_1,
                                                                                 TRC_LOGARITHMIC_100,
                                                                                 TRC_LOGARITHMIC_100_sqrt10,
                                                                                 TRC_PROPHOTO,
                                                                                 TRC_GAMMA_1_8,
                                                                                 TRC_GAMMA_2_4,
                                                                                 TRC_A98,
                                                                                 TRC_LAB_L}};
            const auto characteristic =
                std::find_if(trcList.begin(), trcList.end(), [&](const TransferCharacteristics &check) -> bool {
                    return compareTRC(check, static_cast<float>(error));
                });
            if (characteristic != trcList.end()) {
                d->characteristics = *characteristic;
            }
        }
    }
    return d->characteristics;
}

void KoColorProfile::setCharacteristics(ColorPrimaries primaries, TransferCharacteristics curve)
{
    d->primaries = int(primaries);
    d->characteristics = curve;
}

QString KoColorProfile::getTransferCharacteristicName(TransferCharacteristics curve)
{
    switch (curve) {
    case TRC_ITU_R_BT_709_5:
    case TRC_ITU_R_BT_601_6:
    case TRC_ITU_R_BT_2020_2_10bit:
        return QString("rec 709 trc");
    case TRC_ITU_R_BT_2020_2_12bit:
        return QString("rec 2020 12bit trc");
    case TRC_ITU_R_BT_470_6_SYSTEM_M:
        return QString("Gamma 2.2");
    case TRC_ITU_R_BT_470_6_SYSTEM_B_G:
        return QString("Gamma 2.8");
    case TRC_SMPTE_240M:
        return QString("SMPTE 240 trc");
    case TRC_LINEAR:
        return QString("Linear");
    case TRC_LOGARITHMIC_100:
        return QString("Logarithmic 100");
    case TRC_LOGARITHMIC_100_sqrt10:
        return QString("Logarithmic 100 sqrt10");
    case TRC_IEC_61966_2_4:
        return QString("IEC 61966 2.4");
    case TRC_ITU_R_BT_1361:
    case TRC_IEC_61966_2_1:
        return QString("sRGB trc");
    case TRC_SMPTE_ST_428_1:
        return QString("SMPTE ST 428");
    case TRC_ITU_R_BT_2100_0_PQ:
        return QString("Perceptual Quantizer");
    case TRC_ITU_R_BT_2100_0_HLG:
        return QString("Hybrid Log Gamma");
    case TRC_GAMMA_1_8:
        return QString("Gamma 1.8");
    case TRC_GAMMA_2_4:
        return QString("Gamma 2.4");
    case TRC_A98:
        return QString("Gamma A98");
    case TRC_PROPHOTO:
        return QString("ProPhoto trc");
    case TRC_LAB_L:
        return QString("Lab L* trc");
    case TRC_UNSPECIFIED:
        break;
    }

    return QString("Unspecified");
}

void KoColorProfile::setName(const QString &name)
{
    d->name = name;
}
void KoColorProfile::setInfo(const QString &info)
{
    d->info = info;
}
void KoColorProfile::setManufacturer(const QString &manufacturer)
{
    d->manufacturer = manufacturer;
}
void KoColorProfile::setCopyright(const QString &copyright)
{
    d->copyright = copyright;
}
