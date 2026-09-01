/*
 *  SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "psd_resource_block.h"

#include <QBuffer>
#include <QDataStream>

bool RESN_INFO_1005::interpretBlock(QByteArray data)
{
    dbgFile << "Reading RESN_INFO_1005";

    // the resolution we set on the image should be dpi; we can also set the unit on the KisDocument.
    QDataStream ds(data);
    ds.setByteOrder(QDataStream::BigEndian);

    ds >> hRes >> hResUnit >> widthUnit >> vRes >> vResUnit >> heightUnit;

    /* Resolution always recorded as pixels / inch in a fixed point implied
       decimal int32 with 16 bits before point and 16 after (i.e. cast as
       double and divide resolution by 2^16 */
    dbgFile << "hres" << hRes << "vres" << vRes;

    hRes = hRes / 65536.0;
    vRes = vRes / 65536.0;

    dbgFile << hRes << hResUnit << widthUnit << vRes << vResUnit << heightUnit;

    return ds.atEnd();
}

bool RESN_INFO_1005::createBlock(QByteArray &data)
{
    dbgFile << "Writing RESN_INFO_1005";
    QBuffer buf(&data);

    startBlock(buf, PSDImageResourceSection::RESN_INFO, 16);

    // Convert to 16.16 fixed point
    Fixed h = hRes * 65536.0 + 0.5;
    dbgFile << "h" << h << "hRes" << hRes;
    psdwrite(buf, h);
    psdwrite(buf, hResUnit);
    psdwrite(buf, widthUnit);

    // Convert to 16.16 fixed point
    Fixed v = vRes * 65536.0 + 0.5;
    dbgFile << "v" << v << "vRes" << vRes;
    psdwrite(buf, v);
    psdwrite(buf, vResUnit);
    psdwrite(buf, heightUnit);

    buf.close();

    return true;
}

bool ICC_PROFILE_1039::interpretBlock(QByteArray data)
{
    dbgFile << "Reading ICC_PROFILE_1039";

    icc = data;

    return true;
}

bool ICC_PROFILE_1039::createBlock(QByteArray &data)
{
    dbgFile << "Writing ICC_PROFILE_1039";
    if (icc.size() == 0) {
        error = "ICC_PROFILE_1039: Trying to save an empty profile";
        return false;
    }
    QBuffer buf(&data);
    startBlock(buf, PSDImageResourceSection::ICC_PROFILE, icc.size());
    buf.write(icc.constData(), icc.size());
    buf.close();

    return true;
}
