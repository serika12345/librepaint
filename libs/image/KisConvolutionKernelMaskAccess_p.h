/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISCONVOLUTIONKERNELMASKACCESS_P_H
#define KISCONVOLUTIONKERNELMASKACCESS_P_H

#include <QtGlobal>

class KisMaskGenerator;

namespace KisConvolutionKernelMaskAccess
{

qreal width(const KisMaskGenerator *generator);
qreal height(const KisMaskGenerator *generator);
quint8 valueAt(const KisMaskGenerator *generator, qreal x, qreal y);

} // namespace KisConvolutionKernelMaskAccess

#endif // KISCONVOLUTIONKERNELMASKACCESS_P_H
