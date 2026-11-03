/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "BuiltinLcmsProfile_p.h"
#include "IccColorProfile.h"
#include <QScopeGuard>
#include <QtEndian>
#include <algorithm>
#include <array>

IccColorProfile *createBuiltinLcmsProfile(cmsHPROFILE profile)
{
    if (!profile) return new IccColorProfile(QByteArray());
    const auto closeProfile = qScopeGuard([&] { cmsCloseProfile(profile); });
    const auto serialize = [](cmsHPROFILE native) {
        cmsUInt32Number size = 0;
        if (!cmsSaveProfileToMem(native, nullptr, &size)) return QByteArray();
        QByteArray bytes(qsizetype(size), '\0');
        if (!cmsSaveProfileToMem(native, bytes.data(), &size)) return QByteArray();
        bytes.resize(size);
        return bytes;
    };
    auto bytes = serialize(profile);
    if (bytes.size() < 128) return new IccColorProfile(QByteArray());

    // A fixed definition epoch and no platform preference keep embedded standard
    // profiles identical across process starts and operating systems. ICC.1:2022
    // sections 4.2, 7.2.8, 7.2.10 and 7.2.18 define these header fields and identity.
    constexpr std::array<quint16, 6> definitionDate{2000, 1, 1, 0, 0, 0};
    for (size_t i = 0; i < definitionDate.size(); ++i)
        qToBigEndian(definitionDate[i], bytes.data() + 24 + i * 2);
    qToBigEndian<quint32>(0, bytes.data() + 40);
    std::fill_n(bytes.data() + 84, 16, char(0));

    cmsHPROFILE canonical = cmsOpenProfileFromMem(bytes.constData(), cmsUInt32Number(bytes.size()));
    if (!canonical) return new IccColorProfile(QByteArray());
    const auto closeCanonical = qScopeGuard([&] { cmsCloseProfile(canonical); });
    if (!cmsMD5computeID(canonical)) return new IccColorProfile(QByteArray());
    return new IccColorProfile(serialize(canonical));
}
