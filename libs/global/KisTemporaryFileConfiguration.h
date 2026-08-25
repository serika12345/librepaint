/*
 *  SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KISTEMPORARYFILECONFIGURATION_H
#define KISTEMPORARYFILECONFIGURATION_H

#include "kritaglobal_export.h"

class KConfigGroup;
class QString;

namespace KritaUtils
{

QString KRITAGLOBAL_EXPORT writableTemporaryFileLocation(KConfigGroup config,
                                                         const QString &suffix,
                                                         const QString &configKey,
                                                         bool requestDefault);

QString KRITAGLOBAL_EXPORT writableSwapFileLocation(KConfigGroup config,
                                                    bool requestDefault);

void KRITAGLOBAL_EXPORT normalizeSwapFileLocation(KConfigGroup config);

}

#endif // KISTEMPORARYFILECONFIGURATION_H
