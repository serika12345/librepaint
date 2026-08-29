/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_pipebrush_parasite.h>

#include <QBuffer>
#include <QTest>

#include <array>
#include <type_traits>

namespace
{
class RejectingDevice final : public QIODevice
{
public:
    RejectingDevice()
    {
        open(QIODevice::WriteOnly);
    }

    int writeCalls{0};

protected:
    qint64 readData(char *, qint64) override
    {
        return -1;
    }

    qint64 writeData(const char *, qint64) override
    {
        ++writeCalls;
        return -1;
    }
};
} // namespace

class KisPipeBrushParasiteContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void exposesStablePlacementAndDefaultState();
    void initResetsDimensionArraysOnly();
    void parsesOneToFourDimensions_data();
    void parsesOneToFourDimensions();
    void parsesSelectionModes_data();
    void parsesSelectionModes();
    void clampsInvalidCountsAndUnknownSelection();
    void sanitizesZeroRankIncrementalAndAngularModes();
    void computesBrushCountsAcrossDimensions();
    void savesCanonicalUtf8AndRoundTripsThroughParser();
    void loadReportsSuccessWithoutReadingDevice();
    void saveReportsSuccessAfterWriteFailure();

private:
    void exposesStablePlacementAndDefaultStateImpl()
    {
        static_assert(KisPipeBrushParasite::MaxDim == 4);
        static_assert(std::extent_v<decltype(KisPipeBrushParasite::rank)> == KisPipeBrushParasite::MaxDim);
        static_assert(std::extent_v<decltype(KisPipeBrushParasite::selection)> == KisPipeBrushParasite::MaxDim);
        static_assert(std::extent_v<decltype(KisPipeBrushParasite::brushesCount)> == KisPipeBrushParasite::MaxDim);
        static_assert(std::extent_v<decltype(KisPipeBrushParasite::index)> == KisPipeBrushParasite::MaxDim);

        QCOMPARE(int(KisPipeBrushParasite::DefaultPlacement), 0);
        QCOMPARE(int(KisPipeBrushParasite::ConstantPlacement), 1);
        QCOMPARE(int(KisPipeBrushParasite::RandomPlacement), 2);

        const KisPipeBrushParasite parasite;
        QCOMPARE(parasite.ncells, 0);
        QCOMPARE(parasite.dim, 0);
        QVERIFY(!parasite.needsMovement);
        QVERIFY(parasite.selectionMode.isEmpty());
        for (int i = 0; i < KisPipeBrushParasite::MaxDim; ++i) {
            QCOMPARE(parasite.rank[i], 0);
            QCOMPARE(parasite.index[i], 0);
            QCOMPARE(parasite.brushesCount[i], 0);
            QCOMPARE(parasite.selection[i], KisParasite::Constant);
        }
    }

    void initResetsDimensionArraysOnlyImpl()
    {
        KisPipeBrushParasite parasite;
        parasite.ncells = 19;
        parasite.dim = 3;
        parasite.needsMovement = true;
        parasite.selectionMode = QStringLiteral("angular");
        for (int i = 0; i < KisPipeBrushParasite::MaxDim; ++i) {
            parasite.rank[i] = i + 1;
            parasite.index[i] = i + 2;
            parasite.brushesCount[i] = i + 3;
            parasite.selection[i] = KisParasite::Angular;
        }

        parasite.init();

        QCOMPARE(parasite.ncells, 19);
        QCOMPARE(parasite.dim, 3);
        QVERIFY(parasite.needsMovement);
        QCOMPARE(parasite.selectionMode, QStringLiteral("angular"));
        for (int i = 0; i < KisPipeBrushParasite::MaxDim; ++i) {
            QCOMPARE(parasite.rank[i], 0);
            QCOMPARE(parasite.index[i], 0);
            QCOMPARE(parasite.brushesCount[i], 0);
            QCOMPARE(parasite.selection[i], KisParasite::Constant);
        }
    }

    void parsesOneToFourDimensionsDataImpl()
    {
        QTest::addColumn<QString>("source");
        QTest::addColumn<int>("expectedDim");
        QTest::addColumn<QList<int>>("expectedBrushesCount");

        QTest::newRow("one") << QStringLiteral("ncells:12 dim:1 rank0:3 sel0:constant") << 1 << QList<int>{4};
        QTest::newRow("two") << QStringLiteral("ncells:24 dim:2 rank0:2 sel0:constant rank1:3 sel1:incremental") << 2
                             << QList<int>{12, 4};
        QTest::newRow("three") << QStringLiteral(
            "ncells:48 dim:3 rank0:2 sel0:constant rank1:3 sel1:incremental "
            "rank2:2 sel2:random")
                               << 3 << QList<int>{24, 8, 4};
        QTest::newRow("four") << QStringLiteral(
            "ncells:96 dim:4 rank0:2 sel0:constant rank1:3 sel1:incremental "
            "rank2:2 sel2:random rank3:2 sel3:pressure")
                              << 4 << QList<int>{48, 16, 8, 4};
    }

    void parsesOneToFourDimensionsImpl()
    {
        QFETCH(QString, source);
        QFETCH(int, expectedDim);
        QFETCH(QList<int>, expectedBrushesCount);

        const KisPipeBrushParasite parasite{QStringView(source)};
        QCOMPARE(parasite.dim, expectedDim);
        QCOMPARE(parasite.ncells,
                 source.mid(source.indexOf(QLatin1Char(':')) + 1).section(QLatin1Char(' '), 0, 0).toInt());
        for (int i = 0; i < expectedDim; ++i) {
            QCOMPARE(parasite.index[i], 0);
            QCOMPARE(parasite.brushesCount[i], expectedBrushesCount.at(i));
        }
    }

    void parsesSelectionModesDataImpl()
    {
        QTest::addColumn<QString>("modeName");
        QTest::addColumn<int>("expectedMode");
        QTest::addColumn<bool>("expectedMovement");

        QTest::newRow("constant") << QStringLiteral("constant") << int(KisParasite::Constant) << false;
        QTest::newRow("incremental") << QStringLiteral("incremental") << int(KisParasite::Incremental) << false;
        QTest::newRow("angular") << QStringLiteral("angular") << int(KisParasite::Angular) << true;
        QTest::newRow("velocity") << QStringLiteral("velocity") << int(KisParasite::Velocity) << false;
        QTest::newRow("random") << QStringLiteral("random") << int(KisParasite::Random) << false;
        QTest::newRow("pressure") << QStringLiteral("pressure") << int(KisParasite::Pressure) << false;
        QTest::newRow("xtilt") << QStringLiteral("xtilt") << int(KisParasite::TiltX) << false;
        QTest::newRow("ytilt") << QStringLiteral("ytilt") << int(KisParasite::TiltY) << false;
    }

    void parsesSelectionModesImpl()
    {
        QFETCH(QString, modeName);
        QFETCH(int, expectedMode);
        QFETCH(bool, expectedMovement);

        const QString source = QStringLiteral("ncells:4 dim:1 rank0:4 sel0:%1").arg(modeName);
        const KisPipeBrushParasite parasite{QStringView(source)};
        QCOMPARE(int(parasite.selection[0]), expectedMode);
        QCOMPARE(parasite.selectionMode, modeName);
        QCOMPARE(parasite.needsMovement, expectedMovement);
        QCOMPARE(parasite.brushesCount[0], 1);
    }

    void clampsInvalidCountsAndUnknownSelectionImpl()
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral(".*ncells out of range:\\s+0.*")));
        const KisPipeBrushParasite parasite(QStringView(u"ncells:0 dim:0 rank0:0 sel0:unsupported"));

        QCOMPARE(parasite.ncells, 1);
        QCOMPARE(parasite.dim, 1);
        QCOMPARE(parasite.rank[0], 0);
        QCOMPARE(parasite.brushesCount[0], 1);
        QCOMPARE(parasite.selection[0], KisParasite::Constant);
        QCOMPARE(parasite.selectionMode, QStringLiteral("unsupported"));
        QVERIFY(!parasite.needsMovement);
    }

    void sanitizesZeroRankIncrementalAndAngularModesImpl()
    {
        KisPipeBrushParasite parasite;
        parasite.dim = 4;
        parasite.rank[0] = 0;
        parasite.rank[1] = 0;
        parasite.rank[2] = 0;
        parasite.rank[3] = 2;
        parasite.selection[0] = KisParasite::Incremental;
        parasite.selection[1] = KisParasite::Angular;
        parasite.selection[2] = KisParasite::Random;
        parasite.selection[3] = KisParasite::Angular;
        parasite.needsMovement = true;

        QTest::ignoreMessage(
            QtWarningMsg,
            QRegularExpression(QStringLiteral(".*PIPE brush has a wrong rank for its selection mode!.*")));
        QTest::ignoreMessage(
            QtWarningMsg,
            QRegularExpression(QStringLiteral(".*PIPE brush has a wrong rank for its selection mode!.*")));
        parasite.sanitize();

        QCOMPARE(parasite.selection[0], KisParasite::Constant);
        QCOMPARE(parasite.selection[1], KisParasite::Constant);
        QCOMPARE(parasite.selection[2], KisParasite::Random);
        QCOMPARE(parasite.selection[3], KisParasite::Angular);
        QVERIFY(parasite.needsMovement);
    }

    void computesBrushCountsAcrossDimensionsImpl()
    {
        KisPipeBrushParasite parasite;
        parasite.ncells = 24;
        parasite.dim = 4;
        parasite.rank[0] = 2;
        parasite.rank[1] = 3;
        parasite.rank[2] = 0;
        parasite.rank[3] = 2;

        parasite.setBrushesCount();

        QCOMPARE(parasite.brushesCount[0], 12);
        QCOMPARE(parasite.brushesCount[1], 4);
        QCOMPARE(parasite.brushesCount[2], 4);
        QCOMPARE(parasite.brushesCount[3], 2);
    }

    void savesCanonicalUtf8AndRoundTripsThroughParserImpl()
    {
        KisPipeBrushParasite parasite;
        parasite.ncells = 24;
        parasite.dim = 4;
        parasite.rank[0] = 2;
        parasite.rank[1] = 3;
        parasite.rank[2] = 2;
        parasite.rank[3] = 2;
        parasite.selection[0] = KisParasite::Constant;
        parasite.selection[1] = KisParasite::Incremental;
        parasite.selection[2] = KisParasite::Angular;
        parasite.selection[3] = KisParasite::TiltY;

        QBuffer buffer;
        QVERIFY(buffer.open(QIODevice::WriteOnly));
        QVERIFY(parasite.saveToDevice(&buffer));
        const QByteArray expected(
            "24 ncells:24 dim:4 rank0:2 sel0:constant rank1:3 sel1:incremental "
            "rank2:2 sel2:angular rank3:2 sel3:ytilt");
        QCOMPARE(buffer.data(), expected);
        QVERIFY(!buffer.data().endsWith('\n'));

        const QString saved = QString::fromUtf8(buffer.data());
        const QString parasiteText = saved.mid(saved.indexOf(QLatin1Char(' ')) + 1);
        const KisPipeBrushParasite loaded{QStringView(parasiteText)};
        QCOMPARE(loaded.ncells, parasite.ncells);
        QCOMPARE(loaded.dim, parasite.dim);
        QCOMPARE(loaded.selectionMode, QStringLiteral("ytilt"));
        QVERIFY(loaded.needsMovement);
        for (int i = 0; i < parasite.dim; ++i) {
            QCOMPARE(loaded.rank[i], parasite.rank[i]);
            QCOMPARE(loaded.selection[i], parasite.selection[i]);
        }
    }

    void loadReportsSuccessWithoutReadingDeviceImpl()
    {
        KisPipeBrushParasite parasite;
        QBuffer buffer;
        buffer.setData(QByteArrayLiteral("unread parasite data"));
        QVERIFY(buffer.open(QIODevice::ReadOnly));
        QVERIFY(buffer.seek(4));
        const qint64 originalPosition = buffer.pos();

        QVERIFY(parasite.loadFromDevice(&buffer));
        QCOMPARE(buffer.pos(), originalPosition);
    }

    void saveReportsSuccessAfterWriteFailureImpl()
    {
        KisPipeBrushParasite parasite;
        parasite.ncells = 1;
        parasite.dim = 1;
        parasite.rank[0] = 1;
        RejectingDevice device;

        QVERIFY(parasite.saveToDevice(&device));
        QVERIFY(device.writeCalls > 0);
    }
};

void KisPipeBrushParasiteContractTest::exposesStablePlacementAndDefaultState()
{
    exposesStablePlacementAndDefaultStateImpl();
}

void KisPipeBrushParasiteContractTest::initResetsDimensionArraysOnly()
{
    initResetsDimensionArraysOnlyImpl();
}

void KisPipeBrushParasiteContractTest::parsesOneToFourDimensions_data()
{
    parsesOneToFourDimensionsDataImpl();
}

void KisPipeBrushParasiteContractTest::parsesOneToFourDimensions()
{
    parsesOneToFourDimensionsImpl();
}

void KisPipeBrushParasiteContractTest::parsesSelectionModes_data()
{
    parsesSelectionModesDataImpl();
}

void KisPipeBrushParasiteContractTest::parsesSelectionModes()
{
    parsesSelectionModesImpl();
}

void KisPipeBrushParasiteContractTest::clampsInvalidCountsAndUnknownSelection()
{
    clampsInvalidCountsAndUnknownSelectionImpl();
}

void KisPipeBrushParasiteContractTest::sanitizesZeroRankIncrementalAndAngularModes()
{
    sanitizesZeroRankIncrementalAndAngularModesImpl();
}

void KisPipeBrushParasiteContractTest::computesBrushCountsAcrossDimensions()
{
    computesBrushCountsAcrossDimensionsImpl();
}

void KisPipeBrushParasiteContractTest::savesCanonicalUtf8AndRoundTripsThroughParser()
{
    savesCanonicalUtf8AndRoundTripsThroughParserImpl();
}

void KisPipeBrushParasiteContractTest::loadReportsSuccessWithoutReadingDevice()
{
    loadReportsSuccessWithoutReadingDeviceImpl();
}

void KisPipeBrushParasiteContractTest::saveReportsSuccessAfterWriteFailure()
{
    saveReportsSuccessAfterWriteFailureImpl();
}

QTEST_GUILESS_MAIN(KisPipeBrushParasiteContractTest)

#include "KisPipeBrushParasiteContractTest.moc"
