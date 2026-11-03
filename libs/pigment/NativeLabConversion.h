/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef LIBREPAINT_NATIVE_LAB_CONVERSION_H
#define LIBREPAINT_NATIVE_LAB_CONVERSION_H

#include "KoColorConversionStages.h"
#include <QSharedPointer>
#include <cmath>

namespace LibrePaint::Pigment::Color {
/** Original native RGB-to-Lab16 definition used by colour difference. The
 * integer table is the converter's own table. Linear RGB retains the original
 * binary64 matrix followed by the native float-stage XYZ-to-Lab conversion.
 * Alpha is copied at the input's native precision and is independent of RGB.
 */
struct NativeLabConversion
{
    using IntegerClut = KoColorConversionStages::Clut;
    struct LinearRgb { KoColorConversionStages::Matrix matrix; };
    KoColorConversionStages::Encoding input;
    std::variant<IntegerClut, LinearRgb> definition;
    static constexpr quint64 MaximumClutWords = 16 * 1024 * 1024;

    bool isValid() const
    {
        if (input.iccProfile.isEmpty() || (input.depth != KoColorConversionStages::Encoding::U8 &&
                                          input.depth != KoColorConversionStages::Encoding::U16)) return false;
        if (const auto *clut = std::get_if<IntegerClut>(&definition)) {
            quint64 words = 3;
            for (const auto count : clut->grid) {
                if (count < 2 || count > 255) return false;
                words *= count;
                if (words > MaximumClutWords) return false;
            }
            return quint64(clut->table.size()) == words;
        }
        const auto &matrix = std::get<LinearRgb>(definition).matrix;
        for (const auto value : matrix.coefficients) if (!std::isfinite(value)) return false;
        for (const auto value : matrix.offset) if (!std::isfinite(value)) return false;
        return true;
    }
    qsizetype tableByteCount() const
    {
        const auto *clut = std::get_if<IntegerClut>(&definition);
        return clut ? clut->table.size() * qsizetype(sizeof(quint16)) : 0;
    }
};
using NativeLabConversionSP = QSharedPointer<const NativeLabConversion>;
struct NativeLabConversionResult
{
    enum Code { Success, Unsupported, InvalidData, CapacityExceeded } code = Unsupported;
    NativeLabConversionSP value;
    QString diagnostic;
};
}
#endif
