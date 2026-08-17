/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISRESOURCEUIDESCRIPTOR_H
#define KISRESOURCEUIDESCRIPTOR_H

#include <QString>

#include "kritaresourceui_export.h"

class KRITARESOURCEUI_EXPORT KisResourceUiDescriptor final
{
public:
    explicit KisResourceUiDescriptor(QString resourceType, bool previewEnabled = false);

    const QString &resourceType() const;
    bool previewEnabled() const;

private:
    QString m_resourceType;
    bool m_previewEnabled {false};
};

#endif // KISRESOURCEUIDESCRIPTOR_H
