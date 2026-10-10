/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <KoOptimizedCompositeOpOver32.h>
#include <KoCompositeOpCopy2.h>
#include <KoBgrColorSpaceTraits.h>
#include <cstring>

/** Production center-sample dulling and masked Copy provide the GPU comparison values. */
class KisBrushMixingContractTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void matchesFixedDullingAndCopyResults() {
        QFile file(QStringLiteral(MIXING_FIXTURE));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto fixture = QJsonDocument::fromJson(file.readAll()).object();
        QCOMPARE(fixture["schema"].toInt(), 1);
        using Over = OverCompositor32<quint8, quint32, false, true>;
        KoCompositeOp::ParameterInfo params;
        Over::ParamsWrapper overParams(params);
        int index = 0;
        for (const auto &value : fixture["cases"].toArray()) {
            const auto row = value.toArray();
            alignas(4) quint8 picked[4], mixed[4], result[4];
            const auto source = QByteArray::fromHex(row[0].toString().toLatin1());
            const auto destination = QByteArray::fromHex(row[1].toString().toLatin1());
            std::memcpy(picked, source.constData(), 4);
            std::memcpy(mixed, destination.constData(), 4);
            std::memcpy(result, destination.constData(), 4);
            const float opacity = float(.8 * (double(row[2].toInt()) / 255) * (double(row[3].toInt()) / 255));
            Over::compositeOnePixelScalar<false, xsimd::current_arch>(picked, mixed, nullptr, opacity, overParams);
            result[3] = KoCompositeOpCopy2<KoBgrU8Traits>::composeColorChannels<false, true>(
                mixed, mixed[3], result, result[3], quint8(row[4].toInt()), quint8(255), params.channelFlags);
            const auto actual = QByteArray(reinterpret_cast<const char *>(result), 4);
            const auto expected = QByteArray::fromHex(row[5].toString().toLatin1());
            QVERIFY2(actual == expected, qPrintable(QString("case %1: actual %2 expected %3").arg(index)
                .arg(QString::fromLatin1(actual.toHex()), QString::fromLatin1(expected.toHex()))));
            ++index;
        }
        QCOMPARE(index, 480);
    }
};
QTEST_GUILESS_MAIN(KisBrushMixingContractTest)
#include "KisBrushMixingContractTest.moc"
