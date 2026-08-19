/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_document_recovery_status.h"

namespace Krita
{
namespace Document
{

bool RecoveryStatus::isRecovered() const
{
    return m_recovered;
}

bool RecoveryStatus::setRecovered(bool recovered)
{
    if (m_recovered == recovered) {
        return false;
    }

    m_recovered = recovered;
    return true;
}

}
}
