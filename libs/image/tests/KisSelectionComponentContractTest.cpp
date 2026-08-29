/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_selection_component.h"

#include <QPainterPath>
#include <QSharedPointer>
#include <QTest>

#include <array>
#include <cstddef>

void kisSharedPtrAddReference(KisPaintDevice *)
{
}

bool kisSharedPtrRelease(KisPaintDevice *)
{
    return true;
}

namespace
{

struct alignas(std::max_align_t) OpaqueStorage {
    std::array<std::byte, sizeof(std::max_align_t)> bytes{};
};

template<typename T>
T *opaquePointer(OpaqueStorage &storage)
{
    return reinterpret_cast<T *>(storage.bytes.data());
}

template<typename T>
QSharedPointer<T> nonOwningSharedPointer(T *pointer)
{
    return QSharedPointer<T>(pointer, [](T *) { });
}

class SelectionComponentProbe final : public KisSelectionComponent
{
public:
    explicit SelectionComponentProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~SelectionComponentProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    KisSelectionComponent *clone(KisSelection *selection) override
    {
        ++cloneCalls;
        clonedSelection = selection;
        return cloneResult;
    }

    void renderToProjection(KisPaintDeviceSP projection) override
    {
        ++fullProjectionCalls;
        fullProjection = projection.data();
    }

    void renderToProjection(KisPaintDeviceSP projection, const QRect &rect) override
    {
        ++rectProjectionCalls;
        rectProjection = projection.data();
        projectionRect = rect;
    }

    void moveX(qint32 x) override
    {
        ++moveXCalls;
        xPosition = x;
    }

    void moveY(qint32 y) override
    {
        ++moveYCalls;
        yPosition = y;
    }

    KUndo2Command *transform(const QTransform &transform) override
    {
        ++transformCalls;
        receivedTransform = transform;
        return transformResult;
    }

    bool isEmpty() const override
    {
        ++isEmptyCalls;
        return emptyResult;
    }

    QPainterPath outlineCache() const override
    {
        ++outlineCalls;
        return outlineResult;
    }

    bool outlineCacheValid() const override
    {
        ++outlineValidCalls;
        return outlineValidResult;
    }

    void recalculateOutlineCache() override
    {
        ++recalculateCalls;
    }

    KUndo2Command *resetToEmpty() override
    {
        ++resetCalls;
        return resetResult;
    }

    void setResolutionProxy(KisImageResolutionProxySP resolutionProxy) override
    {
        ++resolutionProxyCalls;
        receivedResolutionProxy = resolutionProxy.data();
    }

    int cloneCalls{0};
    KisSelection *clonedSelection{nullptr};
    KisSelectionComponent *cloneResult{nullptr};
    int fullProjectionCalls{0};
    KisPaintDevice *fullProjection{nullptr};
    int rectProjectionCalls{0};
    KisPaintDevice *rectProjection{nullptr};
    QRect projectionRect;
    int moveXCalls{0};
    qint32 xPosition{0};
    int moveYCalls{0};
    qint32 yPosition{0};
    int transformCalls{0};
    QTransform receivedTransform;
    KUndo2Command *transformResult{nullptr};
    mutable int isEmptyCalls{0};
    bool emptyResult{false};
    mutable int outlineCalls{0};
    QPainterPath outlineResult;
    mutable int outlineValidCalls{0};
    bool outlineValidResult{false};
    int recalculateCalls{0};
    int resetCalls{0};
    KUndo2Command *resetResult{nullptr};
    int resolutionProxyCalls{0};
    KisImageResolutionProxy *receivedResolutionProxy{nullptr};

private:
    int *m_destructionCount{nullptr};
};

class DefaultSelectionComponentProbe final : public KisSelectionComponent
{
public:
    KisSelectionComponent *clone(KisSelection *) override
    {
        return nullptr;
    }

    void renderToProjection(KisPaintDeviceSP) override
    {
    }

    void renderToProjection(KisPaintDeviceSP, const QRect &) override
    {
    }

    bool isEmpty() const override
    {
        return false;
    }

    QPainterPath outlineCache() const override
    {
        return {};
    }

    bool outlineCacheValid() const override
    {
        return false;
    }

    void recalculateOutlineCache() override
    {
    }

    int state{41};
};

} // namespace

class KisSelectionComponentContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructsAndDestroysThroughBase();
    void dispatchesCloneAndProjectionOperations();
    void dispatchesMovementTransformationAndResolution();
    void dispatchesStateQueriesAndReset();
    void defaultOperationsRemainNoOpOrNull();
};

void KisSelectionComponentContractTest::constructsAndDestroysThroughBase()
{
    int destructionCount = 0;
    KisSelectionComponent *component = new SelectionComponentProbe(&destructionCount);

    QCOMPARE(destructionCount, 0);
    delete component;
    QCOMPARE(destructionCount, 1);
}

void KisSelectionComponentContractTest::dispatchesCloneAndProjectionOperations()
{
    SelectionComponentProbe component;
    DefaultSelectionComponentProbe cloneResult;
    KisSelectionComponent *base = &component;
    OpaqueStorage selectionStorage;
    OpaqueStorage projectionStorage;
    KisSelection *selection = opaquePointer<KisSelection>(selectionStorage);
    KisPaintDevice *projectionPointer = opaquePointer<KisPaintDevice>(projectionStorage);
    const KisPaintDeviceSP projection(projectionPointer);
    const QRect rect(-17, 23, 31, 47);
    component.cloneResult = &cloneResult;

    QCOMPARE(base->clone(selection), &cloneResult);
    QCOMPARE(component.cloneCalls, 1);
    QCOMPARE(component.clonedSelection, selection);

    base->renderToProjection(projection);
    QCOMPARE(component.fullProjectionCalls, 1);
    QCOMPARE(component.rectProjectionCalls, 0);
    QCOMPARE(component.fullProjection, projectionPointer);

    base->renderToProjection(projection, rect);
    QCOMPARE(component.fullProjectionCalls, 1);
    QCOMPARE(component.rectProjectionCalls, 1);
    QCOMPARE(component.rectProjection, projectionPointer);
    QCOMPARE(component.projectionRect, rect);
}

void KisSelectionComponentContractTest::dispatchesMovementTransformationAndResolution()
{
    SelectionComponentProbe component;
    KisSelectionComponent *base = &component;
    OpaqueStorage commandStorage;
    OpaqueStorage resolutionStorage;
    KUndo2Command *command = opaquePointer<KUndo2Command>(commandStorage);
    KisImageResolutionProxy *resolution = opaquePointer<KisImageResolutionProxy>(resolutionStorage);
    const KisImageResolutionProxySP resolutionProxy = nonOwningSharedPointer(resolution);
    const QTransform transform(1.25, -0.5, 0.75, 2.5, 17.0, -23.0);
    component.transformResult = command;

    base->moveX(-71);
    base->moveY(89);
    QCOMPARE(component.moveXCalls, 1);
    QCOMPARE(component.xPosition, -71);
    QCOMPARE(component.moveYCalls, 1);
    QCOMPARE(component.yPosition, 89);

    QCOMPARE(base->transform(transform), command);
    QCOMPARE(component.transformCalls, 1);
    QCOMPARE(component.receivedTransform, transform);

    base->setResolutionProxy(resolutionProxy);
    QCOMPARE(component.resolutionProxyCalls, 1);
    QCOMPARE(component.receivedResolutionProxy, resolution);
}

void KisSelectionComponentContractTest::dispatchesStateQueriesAndReset()
{
    SelectionComponentProbe component;
    KisSelectionComponent *base = &component;
    OpaqueStorage commandStorage;
    KUndo2Command *command = opaquePointer<KUndo2Command>(commandStorage);
    QPainterPath outline;
    outline.moveTo(2.5, -3.75);
    outline.cubicTo(7.0, 11.0, -13.0, 17.0, 19.0, 23.0);
    component.emptyResult = true;
    component.outlineResult = outline;
    component.outlineValidResult = true;
    component.resetResult = command;

    QVERIFY(base->isEmpty());
    QCOMPARE(component.isEmptyCalls, 1);
    QCOMPARE(base->outlineCache(), outline);
    QCOMPARE(component.outlineCalls, 1);
    QVERIFY(base->outlineCacheValid());
    QCOMPARE(component.outlineValidCalls, 1);

    base->recalculateOutlineCache();
    QCOMPARE(component.recalculateCalls, 1);
    QCOMPARE(base->resetToEmpty(), command);
    QCOMPARE(component.resetCalls, 1);
}

void KisSelectionComponentContractTest::defaultOperationsRemainNoOpOrNull()
{
    DefaultSelectionComponentProbe component;
    KisSelectionComponent *base = &component;
    OpaqueStorage resolutionStorage;
    KisImageResolutionProxy *resolution = opaquePointer<KisImageResolutionProxy>(resolutionStorage);
    const KisImageResolutionProxySP resolutionProxy = nonOwningSharedPointer(resolution);

    base->moveX(-113);
    base->moveY(127);
    QVERIFY(base->transform(QTransform::fromScale(2.0, 3.0)) == nullptr);
    QVERIFY(base->resetToEmpty() == nullptr);
    base->setResolutionProxy(resolutionProxy);

    QCOMPARE(component.state, 41);
    QVERIFY(!base->isEmpty());
    QVERIFY(base->outlineCache().isEmpty());
    QVERIFY(!base->outlineCacheValid());
}

QTEST_GUILESS_MAIN(KisSelectionComponentContractTest)

#include "KisSelectionComponentContractTest.moc"
