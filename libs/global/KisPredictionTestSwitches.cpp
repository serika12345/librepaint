/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPredictionTestSwitches.h"

#include <QByteArray>
#include <QtGlobal>

namespace
{
const char *TestColorVariable = "LIBREPAINT_PREDICTION_TEST_COLOR";
}

namespace KisPredictionTestSwitches
{
namespace Detail
{
int parseTestColor(const QByteArray &value)
{
    if (value.size() != 6) return -1;

    int rgb = 0;
    for (const char c : value) {
        int digit = -1;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        }
        if (digit < 0) return -1;
        rgb = (rgb << 4) | digit;
    }
    return rgb;
}

int readTestColor()
{
    return parseTestColor(qgetenv(TestColorVariable));
}
}

// The decoration asks for this value for every painted prediction line.
// Reading the environment there would add the environment lock and a string
// allocation to the measured path, so the value is read once per process.
// The test seam is Detail::readTestColor(), which reads the environment again
// on every call.
int testColorRgb()
{
    static const int value = Detail::readTestColor();
    return value;
}
}
