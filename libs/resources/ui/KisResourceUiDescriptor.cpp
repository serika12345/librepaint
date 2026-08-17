/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisResourceUiDescriptor.h"

#include <utility>

KisResourceUiDescriptor::KisResourceUiDescriptor(QString resourceType, bool previewEnabled)
    : m_resourceType(std::move(resourceType))
    , m_previewEnabled(previewEnabled)
{
}

const QString &KisResourceUiDescriptor::resourceType() const
{
    return m_resourceType;
}

bool KisResourceUiDescriptor::previewEnabled() const
{
    return m_previewEnabled;
}
