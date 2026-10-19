/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_LEGACY_RECOVERY_FIXTURE_P_H
#define KIS_GPU_LEGACY_RECOVERY_FIXTURE_P_H

#include <gpu/KisGpuOperationDocument.h>
#include <QCryptographicHash>
#include <QDataStream>
#include <QSet>

namespace LibrePaint::Image::Gpu::Test {
// Version 25 record produced by OperationDocument at a50af7f378: seed42 stroke,
// integer move(-73,19), and seed43 pending input. SHA-256:
// c556bf45c01accefd2338475dcecafc5c023a15683084bd962732b4626d08bc3.
inline QByteArray translatedLayerRecoveryV25()
{
    return QByteArray::fromHex(
        "4c504f520019000000400000004000000000000000030000000000000003000000000000000100000000000000000000"
        "00000000000300000000000000000100000000000000010000000000000001000000ff40240000000000003fb9999999"
        "99999aff00000000000000002a0000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000010000000002402800000000000040220000000000003fe0000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000000000402c00000000000040240000000000003ff00000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002000000"
        "000000000200000000000000010000000000000001ffffffb70000001300000000000000000300000000000000010000"
        "000000000001000000ff40240000000000003fb999999999999aff00000000000000002b000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000001000000000140340000000000004034000000"
        "0000003fe800000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000030000000200000000000000000000000000000000000000000000000000000001000000010000000000"
        "0000010000000c506c61636564206c61796572019192939495969798999a9b9c9d9e9fa000000000ff00000000000000"
        "000000000000000000000000000000000000000100000000000000010000000000000001000000010000000000000001"
        "0000000c506c61636564206c61796572019192939495969798999a9b9c9d9e9fa000000000ff00000000000000000000"
        "000000010000000000000000000000000000000002000000000000000200000000000000010000000100000000000000"
        "010000000c506c61636564206c61796572019192939495969798999a9b9c9d9e9fa000000000ff0000ffffffb7000000"
        "130000000200000000000000010700000000000000010000000000000000ffffffb70000001300000002000000000000"
        "00032a0976cf664a1e4fd84444a22cd6ef4ebc6f88207af62ee01cd323f4d6b0eddc");
}

// Version 23 record produced by insertedForeignLayersSharePixelsAndInputsWithAtomicHistory
// at c8a2448b48. SHA-256: eabdadb389969d53b129f938d265db64dcf87011c6e4e77ca2854f73590acc44.
inline QByteArray positionlessRecoveryV23()
{
    return QByteArray::fromHex(
        "4c504f520017000000400000002000000000000000010000000000000002000000000000000200000000000000000000"
        "0001000000000000000000000117789cedc3410d00000c03a1fa377dfb4e0424b495aaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaea"
        "7b8fd61de300000002010000000000000001000000000000000000000000000000020000000100000000000000000000"
        "00000000000000000000000000000200000000000000010000000000000002000000ff40240000000000003fb9999999"
        "99999aff00000000000000002a0000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000010000000002402000000000000040220000000000003ff0000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000004028000000000000402a0000000000003fe00000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002"
        "00000001000000000000000000000000000000000000000000000000000000010000000100000000000000010000000b"
        "44657374696e6174696f6e013132333435363738393a3b3c3d3e3f4000000000ff000000000000000000000000000000"
        "000000010000000000000001000000000000000200000002000000000000000200000008496e73657274656400212223"
        "2425262728292a2b2c2d2e2f300c22384ead011000000002000000000000000100000000000000010000000b44657374"
        "696e6174696f6e013132333435363738393a3b3c3d3e3f4000000000ff00000000000000ffffffff0000000000000000"
        "4de040889bca17484aa9fb4d7dfa905528b871db3ce5cd2ebfb387bccd65cda6");
}

// Version 24 record produced at fb65cf5473 from one layer at (-73, 19), two
// committed brush samples (seed 42), an undone opacity change to 129 and one
// pending brush sample (seed 43). SHA-256:
// 4d061c9d848f02f72bd2aac38c3b23c09ccc8a88434d0ca466d19988c6d0edbe.
inline QByteArray placedLayerRecoveryV24()
{
    return QByteArray::fromHex(
        "4c504f520018000000400000004000000000000000030000000000000002000000000000000100000000000000010000"
        "00000000000200000000000000000100000000000000010000000000000001000000ff40240000000000003fb9999999"
        "99999aff00000000000000002a0000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000010000000002402800000000000040220000000000003fe0000000000000000000000000000000000000"
        "00000000000000000000000000000000000000010000000000000000402c00000000000040240000000000003ff00000"
        "000000000000000000000000000000000000000000000000000000000000000000000002000000000000000000000000"
        "000000000200000000000000010000000000000001000000ff40240000000000003fb999999999999aff000000000000"
        "00002b000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000"
        "0001403400000000000040340000000000003fe800000000000000000000000000000000000000000000000000000000"
        "000000000000000000030000000000000000000000030000000100000000000000000000000000000000000000000000"
        "0000000000010000000100000000000000010000000c506c61636564206c61796572017172737475767778797a7b7c7d"
        "7e7f8000000000ff0000ffffffb700000013000000000000000000000000000000000100000000000000010000000000"
        "0000010000000100000000000000010000000c506c61636564206c61796572017172737475767778797a7b7c7d7e7f80"
        "00000000ff0000ffffffb700000013000000010000000000000000000000000000000002000000000000000200000000"
        "000000010000000100000000000000010000000c506c61636564206c61796572017172737475767778797a7b7c7d7e7f"
        "8000000000810000ffffffb7000000130000000100000000020000000000000001ff8100000001000000000000000308"
        "a840e7611f42f3c9d0e61e0a7e2f60356d6738e052979c78cb68166eb7f346");
}

// Emits earlier records for the maintained legacy reader contracts.
inline QByteArray legacyRecovery(const OperationDocument &document, quint8 format)
{
    auto payload = document.recoveryData().chopped(32);
    auto versions = document.historyVersions();
    versions.append(document.preview());
    if (format < 24) {
        QSet<QByteArray> records;
        for (const auto &version : versions) {
            if (!version) continue;
            for (const auto &layer : version->layers) {
                Q_ASSERT(layer.position.isNull());
                QByteArray record;
                QDataStream stream(&record, QIODevice::WriteOnly);
                stream.setVersion(QDataStream::Qt_5_15);
                stream.setByteOrder(QDataStream::BigEndian);
                const auto name = layer.name.toUtf8();
                stream << layer.id << quint32(name.size());
                stream.writeRawData(name.constData(), name.size());
                stream << quint8(layer.visible) << layer.uuid;
                for (const auto component : layer.defaultPixel) stream << component;
                stream << layer.opacity << quint8(layer.userLocked)
                       << quint8(quint8(layer.pinnedToTimeline) | ((15u ^ layer.channelWriteMask) << 1));
                const auto positionOffset = record.size();
                stream << qint32(0) << qint32(0);
                if (records.contains(record)) continue;
                records.insert(record);
                qsizetype next = 0;
                for (;;) {
                    const auto found = payload.indexOf(record, next);
                    if (found < 0) break;
                    payload.remove(found + positionOffset, 8);
                    next = found + positionOffset;
                }
            }
        }
    }
    QSet<QPair<quint64, quint64>> seen;
    for (const auto &version : versions) {
        if (!version) continue;
        for (const auto &layer : version->layers) {
            for (const auto &value : layer.operations) {
                const auto paint = std::get_if<PaintSnapshot>(&value);
                if (!paint) continue;
                const auto &operation = **paint;
                const QPair<quint64, quint64> key(operation.id.sequence, operation.version);
                if (seen.contains(key)) continue;
                seen.insert(key);
                Q_ASSERT(format >= 19 || !operation.brush.pixelPerfect);
                Q_ASSERT(!operation.brush.basicSmoothing);
                Q_ASSERT(!operation.brush.weightedSmoothing);
                Q_ASSERT(format >= 18 || operation.shape.kind != ShapeKind::Path);
                Q_ASSERT(format >= 17 || (operation.figureStyle.outline && !operation.figureStyle.fill));
                QByteArray prefix;
                QDataStream stream(&prefix, QIODevice::WriteOnly);
                stream.setVersion(QDataStream::Qt_5_15);
                stream.setByteOrder(QDataStream::BigEndian);
                stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
                stream << quint8(0) << operation.id.sequence << operation.version << operation.layer;
                for (const auto component : operation.brush.rgba) stream << component;
                stream << operation.brush.diameter << operation.brush.spacing << operation.brush.opacity
                       << quint8(quint8(operation.brush.erase) | (quint8(operation.brush.alphaLocked) << 1) |
                                 ((15u ^ operation.brush.channelWriteMask) << 2) |
                                 (quint8(operation.brush.mirrorHorizontal) << 6) | (quint8(operation.brush.mirrorVertical) << 7));
                if (operation.brush.mirrorHorizontal || operation.brush.mirrorVertical)
                    stream << qint32(operation.brush.mirrorCenter.x()) << qint32(operation.brush.mirrorCenter.y());
                stream << operation.randomSeed << quint8(operation.shape.kind)
                       << operation.shape.first.x() << operation.shape.first.y()
                       << operation.shape.second.x() << operation.shape.second.y() << operation.shape.rotation;
                if (operation.shape.kind == ShapeKind::RoundedRectangle)
                    stream << operation.shape.cornerRadii.width() << operation.shape.cornerRadii.height();
                if (operation.shape.kind == ShapeKind::Path) {
                    stream << quint8(operation.shape.windingFill) << quint32(operation.shape.pathCommands.size());
                    for (const auto command : operation.shape.pathCommands) stream << quint8(command);
                }
                const auto position = payload.indexOf(prefix);
                Q_ASSERT(position >= 0 && position == payload.lastIndexOf(prefix));
                const auto styleSize = 1 + (operation.figureStyle.fill ? 4 : 0);
                Q_ASSERT(payload.at(position + prefix.size() + styleSize) == char(operation.brush.pixelPerfect));
                if (format < 21) {
                    const auto samples = position + prefix.size() + styleSize + 1 + 4;
                    for (qsizetype index = operation.sampleCount; index > 0; --index)
                        payload.remove(samples + (index - 1) * 64 + 56, 8);
                }
                if (format < 19) payload.remove(position + prefix.size() + styleSize, 1);
                if (format < 17) payload.remove(position + prefix.size(), styleSize);
            }
        }
    }
    payload[4] = 0;
    payload[5] = char(format);
    payload.append(QCryptographicHash::hash(payload, QCryptographicHash::Sha256));
    return payload;
}
}
#endif
