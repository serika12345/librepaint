/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISNODEUUIDINFONODEACCESS_P_H
#define KISNODEUUIDINFONODEACCESS_P_H

#include "kis_types.h"

#include <QString>
#include <QUuid>

namespace KisNodeUuidInfoNodeAccess
{

QUuid uuid(KisNode *node);
QString name(KisNode *node);
KisNodeSP firstChild(KisNode *node);
KisNodeSP nextSibling(KisNode *node);

} // namespace KisNodeUuidInfoNodeAccess

#endif // KISNODEUUIDINFONODEACCESS_P_H
