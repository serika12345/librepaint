/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_keyframe.h"

#include <QTest>
#include <QWeakPointer>

#include <memory>

namespace
{

class KeyframeProbe final : public KisKeyframe
{
public:
    explicit KeyframeProbe(int *destructionCount = nullptr, int *duplicateDestructionCount = nullptr)
        : m_destructionCount(destructionCount)
        , m_duplicateDestructionCount(duplicateDestructionCount)
    {
    }

    ~KeyframeProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    KisKeyframeSP duplicate(KisKeyframeChannel *newChannel = nullptr) override
    {
        ++duplicateCalls;
        receivedChannel = newChannel;
        return KisKeyframeSP(new KeyframeProbe(m_duplicateDestructionCount));
    }

    int duplicateCalls{0};
    KisKeyframeChannel *receivedChannel{nullptr};

private:
    int *m_destructionCount{nullptr};
    int *m_duplicateDestructionCount{nullptr};
};

} // namespace

class KisKeyframeContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultAndAssignedColorLabelsArePreserved();
    void duplicateDispatchesDefaultNullChannel();
    void duplicateDispatchesBorrowedChannelAndReturnsSharedOwnership();
    void baseOwnershipDestroysDerivedKeyframe();
};

void KisKeyframeContractTest::defaultAndAssignedColorLabelsArePreserved()
{
    KeyframeProbe keyframe;

    QCOMPARE(keyframe.colorLabel(), 0);

    keyframe.setColorLabel(7);

    QCOMPARE(keyframe.colorLabel(), 7);
}

void KisKeyframeContractTest::duplicateDispatchesDefaultNullChannel()
{
    KeyframeProbe keyframe;
    KisKeyframe &base = keyframe;

    const KisKeyframeSP duplicate = base.duplicate();

    QCOMPARE(keyframe.duplicateCalls, 1);
    QVERIFY(!keyframe.receivedChannel);
    QVERIFY(!duplicate.isNull());
}

void KisKeyframeContractTest::duplicateDispatchesBorrowedChannelAndReturnsSharedOwnership()
{
    int duplicateDestructions = 0;
    int channelSentinel = 0;
    auto *borrowedChannel = reinterpret_cast<KisKeyframeChannel *>(&channelSentinel);
    KeyframeProbe keyframe(nullptr, &duplicateDestructions);
    KisKeyframe &base = keyframe;

    KisKeyframeSP duplicate = base.duplicate(borrowedChannel);
    KisKeyframeSP secondOwner = duplicate;
    const QWeakPointer<KisKeyframe> observer = duplicate.toWeakRef();

    QCOMPARE(keyframe.duplicateCalls, 1);
    QCOMPARE(keyframe.receivedChannel, borrowedChannel);

    duplicate.clear();
    QVERIFY(!observer.isNull());
    QCOMPARE(duplicateDestructions, 0);

    secondOwner.clear();
    QVERIFY(observer.isNull());
    QCOMPARE(duplicateDestructions, 1);
}

void KisKeyframeContractTest::baseOwnershipDestroysDerivedKeyframe()
{
    int destructions = 0;
    std::unique_ptr<KisKeyframe> keyframe = std::make_unique<KeyframeProbe>(&destructions);

    keyframe.reset();

    QCOMPARE(destructions, 1);
}

QTEST_GUILESS_MAIN(KisKeyframeContractTest)

#include "KisKeyframeContractTest.moc"
