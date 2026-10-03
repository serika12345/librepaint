/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisSensorPackInterface.h"

#include <QString>

KisSensorPackInterface::~KisSensorPackInterface()
{
}

int KisSensorPackInterface::calcActiveSensorLength(const QString &) const
{
    return -1;
}
