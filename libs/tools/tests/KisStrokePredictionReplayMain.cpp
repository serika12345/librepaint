/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePredictionReplay.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QTextStream>

#include <cstdio>

namespace
{
void printUsage()
{
    std::fprintf(stderr,
                 "usage: KisStrokePredictionReplay --input FILE --output FILE\n"
                 "         [--predictor linear] [--horizon-ms 16.7] [--anchor-ms 24] [--diameter 10]\n"
                 "         [--cap-diameters 3] [--decel on|off]\n"
                 "         [--expiry fixed|gap] [--gap-reset-ms 20]\n"
                 "         [--frame-ms 16.667] [--pipeline-ms 0]\n");
}
} // namespace

int main(int argc, char **argv)
{
    KisStrokePredictionReplaySettings settings;
    QString inputPath;
    QString outputPath;

    for (int i = 1; i < argc; ++i) {
        const QString argument = QString::fromLocal8Bit(argv[i]);
        if (i + 1 >= argc) {
            std::fprintf(stderr, "missing value for %s\n", argv[i]);
            printUsage();
            return 2;
        }
        const QString value = QString::fromLocal8Bit(argv[++i]);

        bool ok = true;
        if (argument == QStringLiteral("--input")) {
            inputPath = value;
        } else if (argument == QStringLiteral("--output")) {
            outputPath = value;
        } else if (argument == QStringLiteral("--predictor")) {
            if (value == QStringLiteral("linear")) {
                settings.predictor = value;
            } else {
                ok = false;
            }
        } else if (argument == QStringLiteral("--horizon-ms")) {
            settings.horizonMs = value.toDouble(&ok);
        } else if (argument == QStringLiteral("--anchor-ms")) {
            settings.anchorMs = value.toDouble(&ok);
        } else if (argument == QStringLiteral("--diameter")) {
            settings.diameter = value.toDouble(&ok);
        } else if (argument == QStringLiteral("--cap-diameters")) {
            settings.capDiameters = value.toDouble(&ok);
        } else if (argument == QStringLiteral("--decel")) {
            if (value == QStringLiteral("on")) {
                settings.decel = true;
            } else if (value == QStringLiteral("off")) {
                settings.decel = false;
            } else {
                ok = false;
            }
        } else if (argument == QStringLiteral("--expiry")) {
            if (value == QStringLiteral("fixed")) {
                settings.gapExpiry = false;
            } else if (value == QStringLiteral("gap")) {
                settings.gapExpiry = true;
            } else {
                ok = false;
            }
        } else if (argument == QStringLiteral("--gap-reset-ms")) {
            settings.gapResetMs = value.toDouble(&ok);
        } else if (argument == QStringLiteral("--frame-ms")) {
            settings.frameMs = value.toDouble(&ok);
        } else if (argument == QStringLiteral("--pipeline-ms")) {
            settings.pipelineMs = value.toDouble(&ok);
        } else {
            std::fprintf(stderr, "unknown argument %s\n", argv[i - 1]);
            printUsage();
            return 2;
        }

        if (!ok) {
            std::fprintf(stderr, "bad value for %s\n", argv[i - 1]);
            return 2;
        }
    }

    if (inputPath.isEmpty() || outputPath.isEmpty()) {
        printUsage();
        return 2;
    }

    QVector<KisStrokePredictionReplayStroke> strokes;
    QString error;
    if (!loadKisStrokePredictionReplayInput(inputPath, &strokes, &error)) {
        std::fprintf(stderr, "%s\n", qPrintable(error));
        return 1;
    }

    const KisStrokePredictionReplayReport report =
        runKisStrokePredictionReplay(strokes, settings);

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        std::fprintf(stderr, "cannot write %s\n", qPrintable(outputPath));
        return 1;
    }

    QTextStream stream(&file);
    stream << QJsonDocument(kisStrokePredictionReplayReportJson(report, settings))
                      .toJson(QJsonDocument::Indented);

    return 0;
}
