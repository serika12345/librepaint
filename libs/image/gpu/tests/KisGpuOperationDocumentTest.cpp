/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>
#include <QCryptographicHash>
#include <QBuffer>
#include <QDataStream>
#include <limits>
#include <stdexcept>

#include <gpu/KisGpuOperationDocument.h>

using namespace Krita::Image::Gpu;

namespace {
InputSample sample(qreal x, qreal y, qreal pressure = 1, quint64 time = 0)
{
    return {{x, y}, pressure, 0, 0, 0, time};
}

OperationId paint(OperationDocument &document, const QList<InputSample> &samples)
{
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 42);
    if (id.isNull() || document.appendSamples(id, samples) != Result::Success ||
        document.commitStroke(id) != Result::Success) {
        return {};
    }
    return id;
}
}

class KisGpuOperationDocumentTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void inputChunksRetainTheSameOperation();
    void shapePreviewCommitsAsOneHistoryEntry();
    void cancelAndRejectedInputPreserveRedo();
    void documentChangesShareOneHistory();
    void savesPinTheirVersionWhileEditingContinues();
    void historyRetains250Operations();
    void staleAndForeignEditsCannotChangeADocument();
    void frozenOperationsOutliveTheDocument();
    void limitsRejectWholeRequests();
    void closedDocumentPreservesSnapshots();
    void cancelledPreviewsCannotBecomeSavedVersions();
    void emptyAndInvalidRequestsLeaveHistoryUnchanged();
    void recoveryPreservesHistoryAndUncommittedIntent();
    void invalidRecoveryLeavesTheLiveDocumentUntouched();
    void checkedRecoveryRejectsInvalidCountsAndValues();
};

void KisGpuOperationDocumentTest::inputChunksRetainTheSameOperation()
{
    OperationDocument one({1024, 1024}), chunks({1024, 1024});
    QList<InputSample> input;
    for (int i = 0; i < 2049; ++i) {
        input.append({QPointF(i % 1024, (i * 7) % 1024), qreal(i % 101) / 100,
                      qreal(i % 91), -qreal(i % 91), qreal(i % 360), quint64(i)});
    }
    const auto a = one.beginStroke(one.activeLayer(), Brush{}, 193);
    const auto b = chunks.beginStroke(chunks.activeLayer(), Brush{}, 193);
    QCOMPARE(one.appendSamples(a, input), Result::Success);
    for (const auto &point : input) {
        QCOMPARE(chunks.appendSamples(b, {point}), Result::Success);
    }
    QCOMPARE(one.commitStroke(a), Result::Success);
    QCOMPARE(chunks.commitStroke(b), Result::Success);
    const auto left = one.snapshot()->layers.front().operations.back();
    const auto right = chunks.snapshot()->layers.front().operations.back();
    QCOMPARE(left->inputSamples(), right->inputSamples());
    QCOMPARE(left->inputSamples(), input);
    QCOMPARE(left->brush, right->brush);
    QCOMPARE(left->randomSeed, right->randomSeed);
    QCOMPARE(one.undoCount(), 1);
    QCOMPARE(chunks.undoCount(), 1);
}

void KisGpuOperationDocumentTest::shapePreviewCommitsAsOneHistoryEntry()
{
    OperationDocument document({1024, 1024});
    const auto original = document.snapshot();
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 42);
    QCOMPARE(document.appendSamples(id, {sample(1, 2), sample(30, 40)}), Result::Success);
    const auto freehand = document.preview();
    QCOMPARE(document.updateShape(id, {ShapeKind::Line, {1, 2}, {60, 70}, 0}), Result::Success);
    QCOMPARE(document.updateShape(id, {ShapeKind::Ellipse, {1, 2}, {90, 70}, 0.25}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.snapshot(), original);
    QCOMPARE(document.undoCount(), 0);
    QCOMPARE(document.commitStroke(id), Result::Success);
    QCOMPARE(document.snapshot(), preview);
    QCOMPARE(document.undoCount(), 1);
    const auto operation = preview->layers.front().operations.back();
    QCOMPARE(operation->shape.kind, ShapeKind::Ellipse);
    QCOMPARE(operation->inputSamples().size(), 2);
    QCOMPARE(freehand->layers.front().operations.back()->shape.kind, ShapeKind::Freehand);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), original);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), preview);
}

void KisGpuOperationDocumentTest::cancelAndRejectedInputPreserveRedo()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(2, 3)}).isNull());
    const auto painted = document.snapshot();
    QCOMPARE(document.undo(), Result::Success);
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(1, 1), sample(2, 2, -1)}), Result::InvalidInput);
    QCOMPARE(document.preview(), document.snapshot());
    QCOMPARE(document.appendSamples(id, {sample(5, 5)}), Result::Success);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    QCOMPARE(document.redoCount(), 1);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.snapshot(), painted);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!paint(document, {sample(8, 9)}).isNull());
    QCOMPARE(document.redoCount(), 0);
}

void KisGpuOperationDocumentTest::documentChangesShareOneHistory()
{
    OperationDocument document({64, 64});
    const auto first = document.activeLayer();
    const auto second = document.addLayer(QStringLiteral("Ink"));
    QVERIFY(second != 0);
    QCOMPARE(document.selectLayer(second), Result::Success);
    QVERIFY(!paint(document, {sample(3, 4)}).isNull());
    const auto painted = document.snapshot();
    QCOMPARE(document.setLayerVisible(second, false), Result::Success);
    QCOMPARE(document.undoCount(), 3);
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot(), painted);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(document.snapshot()->layers.back().operations.empty());
    QCOMPARE(document.undo(), Result::Success);
    QCOMPARE(document.snapshot()->layers.size(), 1);
    QCOMPARE(document.activeLayer(), first);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.activeLayer(), second);
    QCOMPARE(document.redo(), Result::Success);
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(!document.snapshot()->layers.back().visible);
    QVERIFY(document.beginStroke(second, Brush{}, 0).isNull());
}

void KisGpuOperationDocumentTest::savesPinTheirVersionWhileEditingContinues()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(1, 2)}).isNull());
    const auto saving = document.snapshot();
    QVERIFY(!paint(document, {sample(4, 5)}).isNull());
    const auto edited = document.snapshot();
    QCOMPARE(document.markSaved(saving), Result::Success);
    QVERIFY(document.isModified());
    QCOMPARE(saving->layers.front().operations.size(), 1);
    QCOMPARE(edited->layers.front().operations.size(), 2);
    QCOMPARE(document.undo(), Result::Success);
    QVERIFY(!document.isModified());
    QCOMPARE(document.redo(), Result::Success);
    QVERIFY(document.isModified());
    OperationDocument other({64, 64});
    QCOMPARE(document.markSaved(other.snapshot()), Result::StaleVersion);
    QCOMPARE(document.snapshot(), edited);
}

void KisGpuOperationDocumentTest::historyRetains250Operations()
{
    OperationDocument document({64, 64});
    for (int i = 0; i < 260; ++i) {
        QVERIFY(!paint(document, {sample(i, i)}).isNull());
    }
    QCOMPARE(document.undoCount(), 250);
    const auto recovered = document.recoveryData();
    QVERIFY(!recovered.isEmpty());
    QCOMPARE(document.restoreRecovery(recovered), Result::Success);
    QCOMPARE(document.undoCount(), 250);
    for (int i = 0; i < 250; ++i) {
        QCOMPARE(document.undo(), Result::Success);
    }
    QCOMPARE(document.undo(), Result::NothingToUndo);
    QCOMPARE(document.snapshot()->layers.front().operations.size(), 10);
    for (int i = 0; i < 250; ++i) {
        QCOMPARE(document.redo(), Result::Success);
    }
    QCOMPARE(document.snapshot()->layers.front().operations.size(), 260);
    QCOMPARE(document.redo(), Result::NothingToRedo);
}

void KisGpuOperationDocumentTest::staleAndForeignEditsCannotChangeADocument()
{
    OperationDocument document({64, 64}), other({64, 64});
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(other.appendSamples(id, {sample(1, 2)}), Result::StaleOperation);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    const auto next = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QVERIFY(next != id);
    QCOMPARE(document.appendSamples(id, {sample(1, 2)}), Result::StaleOperation);
    QCOMPARE(document.commitStroke(id), Result::StaleOperation);
    QCOMPARE(document.undo(), Result::EditInProgress);
    QCOMPARE(document.setLayerVisible(document.activeLayer(), false), Result::EditInProgress);
    QCOMPARE(document.appendSamples(next, {sample(1, 2)}), Result::Success);
    QCOMPARE(document.commitStroke(next), Result::Success);
}

void KisGpuOperationDocumentTest::frozenOperationsOutliveTheDocument()
{
    Snapshot frozen;
    {
        OperationDocument document({64, 64});
        Brush brush;
        brush.rgba = {12, 34, 56, 78};
        const auto id = document.beginStroke(document.activeLayer(), brush, 123);
        brush.rgba = {0, 0, 0, 0};
        QCOMPARE(document.appendSamples(id, {sample(4, 8, 0.5)}), Result::Success);
        frozen = document.preview();
        QCOMPARE(document.commitStroke(id), Result::Success);
    }
    QCOMPARE(frozen->layers.front().operations.back()->brush.rgba[0], 12);
    QCOMPARE(frozen->layers.front().operations.back()->randomSeed, quint64(123));
    QCOMPARE(frozen->layers.front().operations.back()->inputSamples().front().pressure, qreal(0.5));
}

void KisGpuOperationDocumentTest::limitsRejectWholeRequests()
{
    Limits limits;
    limits.samplesPerStroke = 2;
    limits.layerCount = 1;
    OperationDocument document({64, 64}, limits);
    const auto initial = document.snapshot();
    QCOMPARE(document.addLayer(QStringLiteral("over limit")), LayerId(0));
    QCOMPARE(document.snapshot(), initial);
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(1, 2)}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.appendSamples(id, {sample(3, 4), sample(5, 6)}), Result::CapacityExceeded);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.appendSamples(id, {sample(std::numeric_limits<qreal>::infinity(), 0)}), Result::InvalidInput);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.commitStroke(id), Result::Success);
    QCOMPARE(document.snapshot(), preview);
}

void KisGpuOperationDocumentTest::closedDocumentPreservesSnapshots()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(1, 2)}).isNull());
    const auto frozen = document.snapshot();
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(8, 9)}), Result::Success);
    document.close();
    QCOMPARE(document.snapshot(), frozen);
    QCOMPARE(document.appendSamples(id, {sample(10, 11)}), Result::Closed);
    QCOMPARE(document.commitStroke(id), Result::Closed);
    QCOMPARE(document.undo(), Result::Closed);
    QVERIFY(document.beginStroke(document.activeLayer(), Brush{}, 0).isNull());
    QCOMPARE(frozen->layers.front().operations.size(), 1);
}

void KisGpuOperationDocumentTest::cancelledPreviewsCannotBecomeSavedVersions()
{
    OperationDocument document({64, 64});
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(1, 2)}), Result::Success);
    const auto abandoned = document.preview();
    QCOMPARE(document.markSaved(abandoned), Result::StaleVersion);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    QCOMPARE(document.markSaved(abandoned), Result::StaleVersion);
    QVERIFY(!document.isModified());
}

void KisGpuOperationDocumentTest::emptyAndInvalidRequestsLeaveHistoryUnchanged()
{
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, OperationDocument(QSize(0, 1)));
    OperationDocument document({64, 64});
    const auto original = document.snapshot();
    Brush invalid;
    invalid.diameter = std::numeric_limits<qreal>::quiet_NaN();
    QVERIFY(document.beginStroke(document.activeLayer(), invalid, 0).isNull());
    QVERIFY(document.beginStroke(0, Brush{}, 0).isNull());
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.commitStroke(id), Result::EmptyOperation);
    QCOMPARE(document.appendSamples(id, {}), Result::Success);
    QCOMPARE(document.appendSamples(id, {sample(1, 2, 1, 2)}), Result::Success);
    const auto preview = document.preview();
    QCOMPARE(document.appendSamples(id, {sample(4, 5, 1, 1)}), Result::InvalidInput);
    QCOMPARE(document.updateShape(id, {ShapeKind::Ellipse, {1, 1}, {1, 2}, 0}), Result::InvalidInput);
    QCOMPARE(document.preview(), preview);
    QCOMPARE(document.cancelStroke(id), Result::Success);
    QCOMPARE(document.setLayerVisible(document.activeLayer(), true), Result::Success);
    QCOMPARE(document.selectLayer(document.activeLayer()), Result::Success);
    QCOMPARE(document.snapshot(), original);
    QCOMPARE(document.undoCount(), 0);
}

void KisGpuOperationDocumentTest::recoveryPreservesHistoryAndUncommittedIntent()
{
    OperationDocument source({128, 256});
    QVERIFY(!paint(source, {sample(3, 4, 0.5, 1), sample(5, 6, 0.8, 2)}).isNull());
    QCOMPARE(source.markSaved(source.snapshot()), Result::Success);
    const auto layer = source.addLayer(QStringLiteral("線画 🖌"));
    QVERIFY(!paint(source, {sample(7, 8)}).isNull());
    QCOMPARE(source.undo(), Result::Success);
    const auto pending = source.beginStroke(layer, Brush{}, 903);
    QCOMPARE(source.appendSamples(pending, {sample(9, 10), sample(15, 20)}), Result::Success);
    QCOMPARE(source.updateShape(pending, {ShapeKind::Ellipse, {9, 10}, {15, 20}, 0.3}), Result::Success);
    const QByteArray recovery = source.recoveryData();
    QVERIFY(!recovery.isEmpty());

    OperationDocument restored({1, 1});
    const auto previous = restored.beginStroke(restored.activeLayer(), Brush{}, 0);
    QCOMPARE(restored.restoreRecovery(recovery), Result::Success);
    QCOMPARE(restored.recoveryData(), recovery);
    qInfo() << "recovery SHA-256:" << QCryptographicHash::hash(recovery, QCryptographicHash::Sha256).toHex();
    QCOMPARE(restored.snapshot()->size, QSize(128, 256));
    QCOMPARE(restored.undoCount(), source.undoCount());
    QCOMPARE(restored.redoCount(), source.redoCount());
    QCOMPARE(restored.snapshot()->layers.back().name, QStringLiteral("線画 🖌"));
    QCOMPARE(restored.preview()->layers.back().operations.back()->shape.kind, ShapeKind::Ellipse);
    QCOMPARE(restored.preview()->layers.back().operations.back()->randomSeed, quint64(903));
    QCOMPARE(restored.preview()->layers.back().operations.back()->inputSamples(),
             source.preview()->layers.back().operations.back()->inputSamples());
    QCOMPARE(restored.appendSamples(previous, {sample(0, 0)}), Result::StaleOperation);
    QCOMPARE(restored.appendSamples(pending, {sample(0, 0)}), Result::StaleOperation);
    const auto recoveredEdit = restored.activeOperation();
    QVERIFY(!recoveredEdit.isNull());
    QCOMPARE(restored.cancelStroke(recoveredEdit), Result::Success);
    QCOMPARE(restored.redo(), Result::Success);
    QCOMPARE(restored.snapshot()->layers.back().operations.back()->inputSamples(), QList<InputSample>{sample(7, 8)});
    QCOMPARE(restored.undo(), Result::Success);
    QCOMPARE(restored.undo(), Result::Success);
    QVERIFY(!restored.isModified());
}

void KisGpuOperationDocumentTest::invalidRecoveryLeavesTheLiveDocumentUntouched()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(1, 2)}).isNull());
    const auto id = document.beginStroke(document.activeLayer(), Brush{}, 0);
    QCOMPARE(document.appendSamples(id, {sample(3, 4)}), Result::Success);
    const auto committed = document.snapshot();
    const auto preview = document.preview();
    const auto good = document.recoveryData();
    QVERIFY(!good.isEmpty());
    QList<QByteArray> damaged{{}, good.first(good.size() - 1), QByteArray(256, char(0xff))};
    auto flipped = good;
    flipped[flipped.size() / 2] ^= 1;
    damaged.append(flipped);
    for (const auto &bytes : damaged) {
        QCOMPARE(document.restoreRecovery(bytes), Result::InvalidRecovery);
        QCOMPARE(document.snapshot(), committed);
        QCOMPARE(document.preview(), preview);
        QCOMPARE(document.activeOperation(), id);
    }
    Limits tiny;
    tiny.recoveryBytes = 64;
    OperationDocument constrained({1, 1}, tiny);
    QCOMPARE(constrained.restoreRecovery(good), Result::CapacityExceeded);
    QVERIFY(constrained.recoveryData().isEmpty());
}

void KisGpuOperationDocumentTest::checkedRecoveryRejectsInvalidCountsAndValues()
{
    OperationDocument document({64, 64});
    QVERIFY(!paint(document, {sample(1, 2)}).isNull());
    const auto original = document.snapshot();
    const auto good = document.recoveryData();
    // Format 1's fixed header ends at byte 50; a sample's pressure follows
    // the 99-byte operation record and its two 64-bit coordinates.
    for (const bool invalidCount : {true, false}) {
        auto payload = good.first(good.size() - 32);
        QBuffer buffer(&payload);
        QVERIFY(buffer.open(QIODevice::ReadWrite));
        QVERIFY(buffer.seek(invalidCount ? 46 : 165));
        QDataStream stream(&buffer);
        stream.setVersion(QDataStream::Qt_5_15);
        stream.setByteOrder(QDataStream::BigEndian);
        stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
        if (invalidCount) stream << std::numeric_limits<quint32>::max();
        else stream << std::numeric_limits<qreal>::quiet_NaN();
        buffer.close();
        payload.append(QCryptographicHash::hash(payload, QCryptographicHash::Sha256));
        QCOMPARE(document.restoreRecovery(payload), Result::InvalidRecovery);
        QCOMPARE(document.snapshot(), original);
    }
}

QTEST_GUILESS_MAIN(KisGpuOperationDocumentTest)
#include "KisGpuOperationDocumentTest.moc"
