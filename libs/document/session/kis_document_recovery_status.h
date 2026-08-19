/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_DOCUMENT_RECOVERY_STATUS_H
#define KIS_DOCUMENT_RECOVERY_STATUS_H

#include <kritadocument_export.h>

namespace Krita
{
namespace Document
{

/**
 * Records whether the current document was opened from recovery data.
 *
 * Recovery discovery, file loading, autosave cleanup, presentation, and
 * notifications remain with the document session coordinator.
 */
class KRITADOCUMENT_EXPORT RecoveryStatus
{
public:
    bool isRecovered() const;

    /**
     * Updates the recovery status and returns whether it changed.
     */
    bool setRecovered(bool recovered);

private:
    bool m_recovered = false;
};

}
}

#endif
