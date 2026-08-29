/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisEmbeddedTextureData.h"

#include <QFileInfo>
#include <kis_properties_configuration.h>

bool KisEmbeddedTextureData::read(const KisPropertiesConfiguration *setting)
{
    md5Base64 = setting->getString("Texture/Pattern/PatternMD5");
    md5sum = setting->getString("Texture/Pattern/PatternMD5Sum");
    fileName = QFileInfo(setting->getString("Texture/Pattern/PatternFileName")).fileName();
    name = setting->getString("Texture/Pattern/Name");
    patternBase64 = setting->getString("Texture/Pattern/Pattern");

    return true;
}

void KisEmbeddedTextureData::write(KisPropertiesConfiguration *setting) const
{
    setting->setProperty("Texture/Pattern/PatternMD5", md5Base64);
    setting->setProperty("Texture/Pattern/PatternMD5Sum", md5sum);
    setting->setProperty("Texture/Pattern/PatternFileName", fileName);
    setting->setProperty("Texture/Pattern/Name", name);
}
