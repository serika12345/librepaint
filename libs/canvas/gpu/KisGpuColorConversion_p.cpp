/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuColorConversion_p.h"
#include <QtEndian>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
namespace LibrePaint::Canvas::Gpu::Detail {
namespace {
bool sameEncoding(const KoColorConversionStages::Encoding &a,
                  const KoColorConversionStages::Encoding &b)
{
    return a.depth == b.depth && a.iccProfile == b.iccProfile;
}
}
bool displayEncodingMatches(const KoColorConversionStages::Encoding &encoding,
                            const Image::Gpu::Snapshot &snapshot)
{
    if (!snapshot) return false;
    const bool depthMatches =
        (encoding.depth == KoColorConversionStages::Encoding::U8 &&
         snapshot->format == Image::Gpu::PixelFormat::Rgba8) ||
        (encoding.depth == KoColorConversionStages::Encoding::U16 &&
         snapshot->format == Image::Gpu::PixelFormat::Rgba16);
    // A version that states an ICC must match. A version without one leaves the
    // encoding to the image color space the conversion was built from.
    return depthMatches &&
           (snapshot->colorProfile.isEmpty() || encoding.iccProfile == snapshot->colorProfile);
}
bool sameDisplayConversion(const KoColorConversionStages &a, const KoColorConversionStages &b)
{
    if (!sameEncoding(a.input, b.input) || !sameEncoding(a.output, b.output) ||
        a.stages.size() != b.stages.size()) return false;
    for (qsizetype i = 0; i < a.stages.size(); ++i) {
        if (a.stages[i].index() != b.stages[i].index()) return false;
        if (const auto *matrix = std::get_if<KoColorConversionStages::Matrix>(&a.stages[i])) {
            const auto &other = std::get<KoColorConversionStages::Matrix>(b.stages[i]);
            if (matrix->coefficients != other.coefficients || matrix->offset != other.offset) return false;
        } else if (const auto *clut = std::get_if<KoColorConversionStages::Clut>(&a.stages[i])) {
            const auto &other = std::get<KoColorConversionStages::Clut>(b.stages[i]);
            if (clut->grid != other.grid || clut->table != other.table) return false;
        } else {
            const auto &first = std::get<KoColorConversionStages::Curves>(a.stages[i]);
            const auto &second = std::get<KoColorConversionStages::Curves>(b.stages[i]);
            for (int c = 0; c < 3; ++c) {
                if (first.rgb[c].index() != second.rgb[c].index()) return false;
                if (const auto *curve = std::get_if<KoColorConversionStages::ParametricCurve>(&first.rgb[c])) {
                    const auto &other = std::get<KoColorConversionStages::ParametricCurve>(second.rgb[c]);
                    if (curve->functionType != other.functionType || curve->parameters != other.parameters) return false;
                } else if (std::get<KoColorConversionStages::Table16Curve>(first.rgb[c]).values !=
                           std::get<KoColorConversionStages::Table16Curve>(second.rgb[c]).values) return false;
            }
        }
    }
    return true;
}
bool packDisplayConversion(const KoColorConversionStages &value, quint64 maximumBytes,
                           QByteArray &output, QString &diagnostic)
{
    using Value = KoColorConversionStages;
    const auto validDepth = [](Value::Encoding::Depth depth) {
        return depth == Value::Encoding::U8 || depth == Value::Encoding::U16;
    };
    if (!validDepth(value.input.depth) || !validDepth(value.output.depth)) {
        diagnostic = QStringLiteral("Invalid display encoding depth");
        return false;
    }
    // Word offsets and the actual device storage binding bound the allocation.
    quint64 words = 1;
    for (const auto &stage : value.stages) {
        if (std::holds_alternative<Value::Matrix>(stage)) words += 14;
        else if (const auto *clut = std::get_if<Value::Clut>(&stage)) {
            quint64 nodes = 1;
            for (quint32 points : clut->grid) {
                if (points < 2 || points > 255) {
                    diagnostic = QStringLiteral("Invalid display CLUT grid");
                    return false;
                }
                nodes *= points;
            }
            if (clut->table.size() != qsizetype(nodes * 3)) {
                diagnostic = QStringLiteral("Invalid display CLUT table size");
                return false;
            }
            words += 6 + (quint64(clut->table.size()) + 1) / 2;
        } else {
            words += 5;
            for (const auto &curve : std::get<Value::Curves>(stage).rgb) {
                if (const auto *parametric = std::get_if<Value::ParametricCurve>(&curve))
                    words += 2 + quint64(parametric->parameters.size());
                else words += 2 + quint64(std::get<Value::Table16Curve>(curve).values.size());
            }
        }
        if (words > maximumBytes / 4 || words > quint64(std::numeric_limits<qint32>::max()) / 4) {
            diagnostic = QStringLiteral("Display stages exceed GPU storage capacity");
            return false;
        }
    }
    try {
        QByteArray bytes;
        bytes.reserve(qsizetype(words * 4));
        const auto append = [&bytes](quint32 word) {
            const auto little = qToLittleEndian(word);
            bytes.append(reinterpret_cast<const char *>(&little), 4);
        };
        const auto appendFloat = [&append](double value) {
            const float narrow = float(value);
            if (!std::isfinite(value) || !std::isfinite(narrow)) return false;
            quint32 word;
            std::memcpy(&word, &narrow, 4);
            append(word);
            return true;
        };
        append(quint32(value.stages.size()));
        for (const auto &stage : value.stages) {
            const quint32 start = bytes.size() / 4;
            if (const auto *matrix = std::get_if<Value::Matrix>(&stage)) {
                append(1); append(start + 14);
                for (double item : matrix->coefficients)
                    if (!appendFloat(item)) { diagnostic = QStringLiteral("Nonfinite display matrix"); return false; }
                for (double item : matrix->offset)
                    if (!appendFloat(item)) { diagnostic = QStringLiteral("Nonfinite display offset"); return false; }
            } else if (const auto *clut = std::get_if<Value::Clut>(&stage)) {
                const quint64 tableWords = (quint64(clut->table.size()) + 1) / 2;
                append(2);
                append(quint32(start + 6 + tableWords));
                for (quint32 points : clut->grid) append(points);
                append(quint32(tableWords));
                for (qsizetype i = 0; i < clut->table.size(); i += 2) {
                    quint32 word = clut->table[i];
                    if (i + 1 < clut->table.size()) word |= quint32(clut->table[i + 1]) << 16;
                    append(word);
                }
            } else {
                const auto &curves = std::get<Value::Curves>(stage);
                quint32 next = start + 5;
                append(0);
                quint32 end = next;
                for (const auto &curve : curves.rgb)
                    end += 2 + (std::holds_alternative<Value::ParametricCurve>(curve)
                        ? std::get<Value::ParametricCurve>(curve).parameters.size()
                        : std::get<Value::Table16Curve>(curve).values.size());
                append(end);
                for (const auto &curve : curves.rgb) {
                    append(next);
                    next += 2 + (std::holds_alternative<Value::ParametricCurve>(curve)
                        ? std::get<Value::ParametricCurve>(curve).parameters.size()
                        : std::get<Value::Table16Curve>(curve).values.size());
                }
                for (const auto &curve : curves.rgb) {
                    if (const auto *p = std::get_if<Value::ParametricCurve>(&curve)) {
                        const int type = p->functionType;
                        const int magnitude = type >= -5 && type <= 5 ? std::abs(type) : 0;
                        const int counts[] = {0, 1, 3, 4, 5, 7};
                        if (!magnitude || p->parameters.size() != counts[magnitude] ||
                            !(p->parameters[0] > 0) ||
                            (magnitude >= 2 && std::abs(p->parameters[1]) < 0.0001) ||
                            (type < 0 && p->parameters[0] < 0.0001)) {
                            diagnostic = QStringLiteral("Invalid parametric display curve"); return false;
                        }
                        append(quint32(type)); append(quint32(p->parameters.size()));
                        for (double item : p->parameters)
                            if (!appendFloat(item)) { diagnostic = QStringLiteral("Nonfinite display curve"); return false; }
                    } else {
                        const auto &table = std::get<Value::Table16Curve>(curve).values;
                        if (table.size() < 2) {
                            diagnostic = QStringLiteral("Invalid sampled display curve"); return false;
                        }
                        append(0); append(quint32(table.size()));
                        for (quint16 item : table) append(item);
                    }
                }
            }
        }
        if (quint64(bytes.size()) > maximumBytes) {
            diagnostic = QStringLiteral("Display stages exceed GPU storage capacity"); return false;
        }
        output = std::move(bytes);
        return true;
    } catch (const std::bad_alloc &) {
        diagnostic = QStringLiteral("Cannot allocate GPU display stages");
        return false;
    }
}
}
