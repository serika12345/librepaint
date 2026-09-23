/*
 *  SPDX-FileCopyrightText: 2010 Cyrille Berger <cberger@cberger.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "KoDummyColorProfile.h"
#include "KoColorProfile.h"
#include "KoColorProfileConstants.h"
#include "KoColorimetryUtils.h"
#include <optional>
#include <qcontainerfwd.h>
#include <QtGlobal>
#include <qstringview.h>

KoDummyColorProfile::KoDummyColorProfile()
{
    setName("default");
}

KoDummyColorProfile::~KoDummyColorProfile()
{
}

KoColorProfile* KoDummyColorProfile::clone() const
{
    return new KoDummyColorProfile();
}

bool KoDummyColorProfile::valid() const
{
    return true;
}
float KoDummyColorProfile::version() const
{
    return 0.0;
}
bool KoDummyColorProfile::isSuitableForOutput() const
{
    return true;
}
bool KoDummyColorProfile::isSuitableForInput() const
{
    return true;
}
bool KoDummyColorProfile::isSuitableForWorkspace() const
{
    return true;
}

bool KoDummyColorProfile::isSuitableForPrinting() const
{
    return true;
}

bool KoDummyColorProfile::isSuitableForDisplay() const
{
    return true;
}
bool KoDummyColorProfile::supportsPerceptual() const
{
    return true;
}
bool KoDummyColorProfile::supportsSaturation() const
{
    return true;
}
bool KoDummyColorProfile::supportsAbsolute() const
{
    return true;
}
bool KoDummyColorProfile::supportsRelative() const
{
    return true;
}
bool KoDummyColorProfile::hasColorants() const
{
    return true;
}
bool KoDummyColorProfile::hasTRC() const
{
    return true;
}
bool KoDummyColorProfile::isLinear() const
{
    return true;
}
QVector<KoColorimetryUtils::XYZ> KoDummyColorProfile::getColorantsXYZ() const
{
    QVector<KoColorimetryUtils::XYZ> sRGBStandardAdaptedColorants = {
        KoColorimetryUtils::XYZ{0.43603516, 0.22248840, 0.01391602},
        KoColorimetryUtils::XYZ{0.38511658, 0.71690369, 0.09706116},
        KoColorimetryUtils::XYZ{0.14305115, 0.06060791, 0.71392822}
    };
    return sRGBStandardAdaptedColorants;
}

QVector<KoColorimetryUtils::xyY> KoDummyColorProfile::getColorantsxyY() const
{
    QVector<KoColorimetryUtils::xyY> result;

    QVector<KoColorimetryUtils::XYZ> srgb = getColorantsXYZ();

    Q_FOREACH (KoColorimetryUtils::XYZ c, srgb) {
        result.append(c.toxyY());
    }

    return result;
}

KoColorimetryUtils::XYZ KoDummyColorProfile::getWhitePointXYZ() const
{
    return KoColorimetryUtils::XYZ{0.9642, 1.0000, 0.8249};
}

KoColorimetryUtils::xyY KoDummyColorProfile::getWhitePointxyY() const
{
    return KoColorimetryUtils::xyY{0.34773, 0.35952, 1.0};
}

std::optional<double> KoDummyColorProfile::hdrReferenceWhite() const
{
    return std::nullopt;
}

QVector <double> KoDummyColorProfile::getEstimatedTRC() const

{
    QVector<double> Dummy(3);
    Dummy.fill(2.2);
    return Dummy;
}

bool KoDummyColorProfile::compareTRC(TransferCharacteristics, float) const
{
    return false;
}

void KoDummyColorProfile::linearizeFloatValue(QVector <double> & ) const
{
}

void KoDummyColorProfile::delinearizeFloatValue(QVector <double> & ) const
{
}
void KoDummyColorProfile::linearizeFloatValueFast(QVector <double> & ) const
{
}

void KoDummyColorProfile::delinearizeFloatValueFast(QVector <double> & ) const
{
}
bool KoDummyColorProfile::operator==(const KoColorProfile& rhs) const
{
    return dynamic_cast<const KoDummyColorProfile*>(&rhs);
}

QByteArray KoDummyColorProfile::uniqueId() const
{
    return QByteArray();
}

