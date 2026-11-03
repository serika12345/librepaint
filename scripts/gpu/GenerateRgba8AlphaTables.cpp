/*
 * SPDX-FileCopyrightText: 2014 Nicholas Guttenberg <ngutten@gmail.com>
 * SPDX-FileCopyrightText: 2012 José Luis Vergara <pentalis@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <KoColorSpaceMaths.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <cstdio>

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    if (argc != 3) return 2;
    QByteArray bytes(256 * 256 * 4, '\0');
    for (int source = 0; source < 256; ++source) for (int destination = 0; destination < 256; ++destination) {
        quint8 alpha = destination, weight = 0;
        if (source && destination != 255) {
            using namespace Arithmetic;
            const float dA = scale<float>(quint8(destination));
            const float sA = scale<float>(quint8(source));
            const float w = 1.0 / (1.0 + exp(-40.0 * (dA - sA)));
            float a = dA * w + sA * (1.0 - w);
            if (a < 0.f) a = 0.f;
            if (a > 1.f) a = 1.f;
            if (a < dA) a = dA;
            const float fakeOpacity = 1.f - (1.f - a) / (1.f - dA + 1e-16f);
            alpha = scale<quint8>(a);
            weight = scale<quint8>(fakeOpacity);
        }
        const auto at = (source * 256 + destination) * 4;
        bytes[at] = char(alpha);
        bytes[at + 1] = char(weight);
    }
    const QDir directory(QString::fromLocal8Bit(argv[1]));
    if (!directory.mkpath(QStringLiteral("."))) return 3;
    QFile output(directory.filePath(QStringLiteral("rgba8-greater-alpha.bin")));
    if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size()) return 4;
    output.close();
    const QJsonObject manifest{
        {QStringLiteral("formatVersion"), 1},
        {QStringLiteral("purpose"), QStringLiteral("Exact Greater RGBA8 destination alpha and quantized color weight")},
        {QStringLiteral("sourceRevision"), QString::fromLatin1(argv[2])},
        {QStringLiteral("source"), QStringLiteral("libs/pigment/compositeops/KoCompositeOpGreater.h")},
        {QStringLiteral("sourceAuthor"), QStringLiteral("Nicholas Guttenberg, Jose Luis Vergara, and LibrePaint/Krita contributors")},
        {QStringLiteral("sourceLicense"), QStringLiteral("LGPL-2.0-or-later")},
        {QStringLiteral("packing"), QStringLiteral("RGBA8Uint 256x256; x=destination alpha, y=applied source alpha; R=resulting alpha, G=color weight, BA=0")},
        {QStringLiteral("bytes"), bytes.size()},
        {QStringLiteral("sha256"), QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())}
    };
    QFile description(directory.filePath(QStringLiteral("rgba8-greater-alpha.json")));
    const auto json = QJsonDocument(manifest).toJson();
    if (!description.open(QIODevice::WriteOnly) || description.write(json) != json.size()) return 5;
    std::printf("Generated Greater alpha pairs, %lld bytes\n", static_cast<long long>(bytes.size()));
    return 0;
}
