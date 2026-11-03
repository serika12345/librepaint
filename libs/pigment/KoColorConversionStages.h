/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef KO_COLOR_CONVERSION_STAGES_H
#define KO_COLOR_CONVERSION_STAGES_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <array>
#include <variant>

/** Owned display conversion in normalized logical RGB. Alpha is independent. */
struct KoColorConversionStages
{
    struct Encoding {
        enum Depth { U8, U16 };
        Depth depth = U8;
        QByteArray iccProfile;
    };
    struct ParametricCurve {
        int functionType = 1;
        QVector<double> parameters;
    };
    struct Table16Curve {
        QVector<quint16> values;
    };
    using Curve = std::variant<ParametricCurve, Table16Curve>;
    struct Curves {
        std::array<Curve, 3> rgb;
    };
    struct Matrix {
        std::array<double, 9> coefficients{};
        std::array<double, 3> offset{};
    };
    /** Owned 16-bit 3x3 CLUT in LCMS node order: three output words per node. */
    struct Clut {
        std::array<quint32, 3> grid{};
        QVector<quint16> table;
    };
    using Stage = std::variant<Curves, Matrix, Clut>;
    Encoding input;
    Encoding output;
    QVector<Stage> stages;
};

struct KoColorConversionStagesResult
{
    enum Status { Success, Unsupported, InvalidData, CapacityExceeded };
    Status status = Unsupported;
    KoColorConversionStages value;
    QString diagnostic;
};

#endif
