/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_do_something_command.h"

#include <QTest>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

namespace
{

struct LayerProbe {
    LayerProbe *lastCommandLayer = nullptr;
    const void *colorSpaceValue = nullptr;
    const void *lastResetColorSpace = nullptr;
    int commandCalls = 0;
    int colorSpaceCalls = 0;
    int resetCalls = 0;
    int updateCalls = 0;
    int notifyColorSpaceChangedCalls = 0;

    const void *colorSpace()
    {
        ++colorSpaceCalls;
        return colorSpaceValue;
    }

    void resetCache(const void *colorSpace)
    {
        ++resetCalls;
        lastResetColorSpace = colorSpace;
    }

    void update()
    {
        ++updateCalls;
    }

    void notifyColorSpaceChanged()
    {
        ++notifyColorSpaceChangedCalls;
    }
};

template<class LayerType>
struct CommandProbeOperation {
    void operator()(LayerType layer)
    {
        ++layer->commandCalls;
        layer->lastCommandLayer = layer;
    }
};

} // namespace

class KisDoSomethingCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void finalUpdateControlsRepeatedRedoAndUndoAndParentOwnership();
    void resetOperationForwardsCurrentColorSpace();
    void updateOperationCallsLayer();
    void notifyOperationCallsLayer();
};

void KisDoSomethingCommandContractTest::finalUpdateControlsRepeatedRedoAndUndoAndParentOwnership()
{
    LayerProbe undoLayer;
    LayerProbe redoLayer;
    KUndo2Command parent;

    using Command = KisDoSomethingCommand<CommandProbeOperation, LayerProbe *>;
    auto *undoCommand = new Command(&undoLayer, false, &parent);
    auto *redoCommand = new Command(&redoLayer, true, &parent);

    QCOMPARE(parent.childCount(), 2);
    QCOMPARE(parent.child(0), undoCommand);
    QCOMPARE(parent.child(1), redoCommand);

    KUndo2Command *undoBase = undoCommand;
    undoBase->redo();
    undoBase->redo();
    QCOMPARE(undoLayer.commandCalls, 0);
    undoBase->undo();
    undoBase->undo();
    QCOMPARE(undoLayer.commandCalls, 2);
    QCOMPARE(undoLayer.lastCommandLayer, &undoLayer);

    KUndo2Command *redoBase = redoCommand;
    redoBase->undo();
    redoBase->undo();
    QCOMPARE(redoLayer.commandCalls, 0);
    redoBase->redo();
    redoBase->redo();
    QCOMPARE(redoLayer.commandCalls, 2);
    QCOMPARE(redoLayer.lastCommandLayer, &redoLayer);
}

void KisDoSomethingCommandContractTest::resetOperationForwardsCurrentColorSpace()
{
    int colorSpaceToken = 0;
    LayerProbe layer;
    layer.colorSpaceValue = &colorSpaceToken;

    KisDoSomethingCommandOps::ResetOp<LayerProbe *> operation;
    operation(&layer);

    QCOMPARE(layer.colorSpaceCalls, 1);
    QCOMPARE(layer.resetCalls, 1);
    QCOMPARE(layer.lastResetColorSpace, &colorSpaceToken);
}

void KisDoSomethingCommandContractTest::updateOperationCallsLayer()
{
    LayerProbe layer;

    KisDoSomethingCommandOps::UpdateOp<LayerProbe *> operation;
    operation(&layer);

    QCOMPARE(layer.updateCalls, 1);
    QCOMPARE(layer.resetCalls, 0);
    QCOMPARE(layer.notifyColorSpaceChangedCalls, 0);
}

void KisDoSomethingCommandContractTest::notifyOperationCallsLayer()
{
    LayerProbe layer;

    KisDoSomethingCommandOps::NotifyColorSpaceChangedOp<LayerProbe *> operation;
    operation(&layer);

    QCOMPARE(layer.notifyColorSpaceChangedCalls, 1);
    QCOMPARE(layer.resetCalls, 0);
    QCOMPARE(layer.updateCalls, 0);
}

QTEST_GUILESS_MAIN(KisDoSomethingCommandContractTest)

#include "KisDoSomethingCommandContractTest.moc"
