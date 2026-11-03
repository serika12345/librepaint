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
    QByteArray bytes(2 * 65536 * 4, '\0');
    for (quint32 word = 0; word < 65536; ++word) {
        const volatile double angle = Arithmetic::pi * Arithmetic::scale<qreal>(quint16(word));
        const double value = std::cos(double(angle));
        quint64 bits;
        std::memcpy(&bits, &value, sizeof(bits));
        qToLittleEndian<quint32>(quint32(bits), bytes.data() + word * 4);
        qToLittleEndian<quint32>(quint32(bits >> 32), bytes.data() + (65536 + word) * 4);
    }
    const QDir directory(QString::fromLocal8Bit(argv[1]));
    if (!directory.mkpath(QStringLiteral("."))) return 3;
    QFile output(directory.filePath(QStringLiteral("rgba16-cosine.bin")));
    if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size()) return 4;
    output.close();
    const QJsonObject manifest{
        {QStringLiteral("formatVersion"), 1},
        {QStringLiteral("purpose"), QStringLiteral("Native binary64 cosine values for 16-bit interpolation painting, projection and implicit defaults")},
        {QStringLiteral("sourceRevision"), QString::fromLatin1(argv[2])},
        {QStringLiteral("source"), QStringLiteral("libs/pigment/compositeops/KoCompositeOpFunctions.h")},
        {QStringLiteral("sourceAuthor"), QStringLiteral("Silvio Heinrich and LibrePaint/Krita contributors")},
        {QStringLiteral("sourceLicense"), QStringLiteral("LGPL-2.1-or-later")},
        {QStringLiteral("packing"), QStringLiteral("RGBA8Uint 256x256 array; x=word&255, y=word>>8; layer0=low binary64 word, layer1=high binary64 word; little-endian bytes")},
        {QStringLiteral("modes"), QJsonArray{QStringLiteral("interpolation"), QStringLiteral("interpolation 2x")}},
        {QStringLiteral("bytes"), bytes.size()},
        {QStringLiteral("sha256"), QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())}
    };
    QFile description(directory.filePath(QStringLiteral("rgba16-cosine.json")));
    const auto json = QJsonDocument(manifest).toJson();
    if (!description.open(QIODevice::WriteOnly) || description.write(json) != json.size()) return 5;
    std::printf("Generated 65536 cosine values, %lld bytes\n", static_cast<long long>(bytes.size()));
    return 0;
}
