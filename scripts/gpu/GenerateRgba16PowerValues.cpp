/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <compositeops/KoCompositeOpFunctions.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>
#include <cmath>
#include <cstdio>
#include <cstring>

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    if (argc != 3) return 2;
    QByteArray bytes(8 * 65536 * 4, '\0');
    for (quint32 word = 0; word < 65536; ++word) {
        const double normalized = Arithmetic::scale<qreal>(quint16(word));
        const volatile double inverse = 1.0 - normalized;
        const volatile double twice = 2.0 * normalized;
        const volatile double centered = twice - 1.0;
        const double values[] = {std::pow(normalized, 2.875), std::pow(double(inverse), 2.875),
                                 std::pow(std::abs(double(centered)), 2.875), std::pow(double(word), 2.3333333333333333)};
        for (quint32 curve = 0; curve < 4; ++curve) {
            quint64 bits;
            std::memcpy(&bits, &values[curve], sizeof(bits));
            qToLittleEndian<quint32>(quint32(bits), bytes.data() + (curve * 2 * 65536 + word) * 4);
            qToLittleEndian<quint32>(quint32(bits >> 32), bytes.data() + ((curve * 2 + 1) * 65536 + word) * 4);
        }
    }
    const QDir directory(QString::fromLocal8Bit(argv[1]));
    if (!directory.mkpath(QStringLiteral("."))) return 3;
    QFile output(directory.filePath(QStringLiteral("rgba16-power.bin")));
    if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size()) return 4;
    output.close();
    const QJsonObject manifest{
        {QStringLiteral("formatVersion"), 1},
        {QStringLiteral("purpose"), QStringLiteral("Native binary64 inner powers for 16-bit Super Light and P-Norm A painting, projection and implicit defaults")},
        {QStringLiteral("sourceRevision"), QString::fromLatin1(argv[2])},
        {QStringLiteral("source"), QStringLiteral("libs/pigment/compositeops/KoCompositeOpFunctions.h")},
        {QStringLiteral("sourceAuthor"), QStringLiteral("Silvio Heinrich and LibrePaint/Krita contributors")},
        {QStringLiteral("sourceLicense"), QStringLiteral("LGPL-2.1-or-later")},
        {QStringLiteral("packing"), QStringLiteral("RGBA8Uint 256x256 array; x=word&255, y=word>>8; layers0/1=pow(normalized,2.875), layers2/3=pow(1-normalized,2.875), layers4/5=pow(abs(2*normalized-1),2.875); layers6/7=pow(word,2.3333333333333333); low/high binary64 words; little-endian bytes")},
        {QStringLiteral("modes"), QJsonArray{QStringLiteral("super_light"), QStringLiteral("pnorm_a")}},
        {QStringLiteral("bytes"), bytes.size()},
        {QStringLiteral("sha256"), QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())}
    };
    QFile description(directory.filePath(QStringLiteral("rgba16-power.json")));
    const auto json = QJsonDocument(manifest).toJson();
    if (!description.open(QIODevice::WriteOnly) || description.write(json) != json.size()) return 5;
    std::printf("Generated 4x65536 inner power values, %lld bytes\n", static_cast<long long>(bytes.size()));
    return 0;
}
