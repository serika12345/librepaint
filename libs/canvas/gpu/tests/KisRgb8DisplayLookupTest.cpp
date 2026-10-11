/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisRgb8DisplayLookup.h"
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>
#include <KoTestConfig.h>
#include <QFile>
#include <QStandardPaths>
#include <QtTest>
#include <random>

namespace Lookup = KisRgb8DisplayLookup;
using Conversion = KoColorConversionTransformation;

class KisRgb8DisplayLookupTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void profileValuesMatchDirectConversion_data() {
        QTest::addColumn<QString>("profileFile");
        QTest::addColumn<bool>("optimized");
        for (const auto &profile : {"ClayRGB-elle-V4-g22.icc", "Rec2020-elle-V2-g10.icc", "sRGB-elle-V4-g22.icc"}) {
            for (bool optimized : {false, true})
                QTest::newRow(qPrintable(QString::fromLatin1(profile) + (optimized ? "-optimized" : "-exact")))
                    << QString::fromLatin1(profile) << optimized;
        }
    }
    void profileValuesMatchDirectConversion() {
        QFETCH(QString, profileFile);
        QFETCH(bool, optimized);
        auto *registry = KoColorSpaceRegistry::instance();
        const auto *source = registry->rgb8(registry->p709SRGBProfile());
        QFile profile(QStringLiteral(KRITA_SOURCE_DIR "/krita/data/profiles/elles-icc-profiles/") + profileFile);
        QVERIFY(profile.open(QIODevice::ReadOnly));
        const auto *destinationProfile = registry->createColorProfile(RGBAColorModelID.id(),
            Integer8BitsColorDepthID.id(), profile.readAll());
        QVERIFY(destinationProfile);
        const auto *destination = registry->rgb8(destinationProfile);
        QVERIFY(source && destination);
        auto flags = Conversion::ConversionFlags(Conversion::BlackpointCompensation | Conversion::CopyAlpha);
        flags |= optimized ? Conversion::HighQuality : Conversion::NoOptimization;
        const std::atomic_bool cancelled{false};
        const auto result = Lookup::build(source, destination, Conversion::IntentRelativeColorimetric, flags, cancelled);
        QCOMPARE(result.status, Lookup::Status::Ready);
        QVERIFY(result.values);
        QCOMPARE(result.values->size(), qsizetype(256 * 256 * 256 * 4));
        constexpr int pixels = 65536;
        QByteArray input(pixels * 4, 0), expected(pixels * 4, 0);
        std::mt19937 random(718993);
        for (int i = 0; i < pixels * 4; ++i) input[i] = char(random() & 255);
        std::unique_ptr<Conversion> converter(source->createColorConverter(destination,
            Conversion::IntentRelativeColorimetric, flags));
        QVERIFY(converter && converter->isValid());
        converter->transform(reinterpret_cast<const quint8 *>(input.constData()),
            reinterpret_cast<quint8 *>(expected.data()), pixels);
        for (int i = 0; i < pixels; ++i) {
            const auto byte = [&](int channel) { return quint8(input.at(i * 4 + channel)); };
            const qsizetype offset = (byte(2) + 256 * byte(1) + 65536 * byte(0)) * 4;
            const auto rgba = result.values->mid(offset, 4);
            QCOMPARE(quint8(rgba.at(0)), quint8(expected.at(i * 4 + 2)));
            QCOMPARE(quint8(rgba.at(1)), quint8(expected.at(i * 4 + 1)));
            QCOMPARE(quint8(rgba.at(2)), quint8(expected.at(i * 4)));
        }
    }
    void identityCancellationAndUnsupportedColorDepth() {
        auto *registry = KoColorSpaceRegistry::instance();
        const auto *source = registry->rgb8(registry->p709SRGBProfile());
        QVERIFY(source);
        std::atomic_bool cancelled{false};
        const auto intent = Conversion::IntentRelativeColorimetric;
        const auto flags = Conversion::internalConversionFlags();
        const auto identity = Lookup::build(source, source, intent, flags, cancelled);
        QCOMPARE(identity.status, Lookup::Status::Ready);
        QVERIFY(!identity.values);
        const auto invalid = Lookup::build(source, registry->rgb16(), intent, flags, cancelled);
        QCOMPARE(invalid.status, Lookup::Status::InvalidColorSpace);
        QVERIFY(!invalid.values);
        cancelled.store(true);
        const auto stopped = Lookup::build(source, source, intent, flags, cancelled);
        QCOMPARE(stopped.status, Lookup::Status::Cancelled);
        QVERIFY(!stopped.values);
    }
};
int main(int argc, char **argv)
{
    QStandardPaths::setTestModeEnabled(true);
    qputenv("EXTRA_RESOURCE_DIRS", QByteArray(KRITA_RESOURCE_DIRS_FOR_TESTS));
    qputenv("KRITA_PLUGIN_PATH", QByteArray(KRITA_PLUGINS_DIR_FOR_TESTS));
    QCoreApplication app(argc, argv);
    KisRgb8DisplayLookupTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisRgb8DisplayLookupTest.moc"
