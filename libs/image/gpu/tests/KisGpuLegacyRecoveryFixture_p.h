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
// Emits earlier records for the maintained legacy reader contracts.
inline QByteArray legacyRecovery(const OperationDocument &document, quint8 format)
{
    auto payload = document.recoveryData().chopped(32);
    auto versions = document.historyVersions();
    versions.append(document.preview());
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
