/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_crop_saved_extra_data.h"

#include <QTest>

class KisNode
{
public:
    explicit KisNode(int &destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~KisNode()
    {
        ++m_destructionCount;
    }

    void addReference()
    {
        ++m_references;
    }

    bool releaseReference()
    {
        --m_references;
        if (!m_references) {
            delete this;
            return false;
        }
        return true;
    }

    int referenceCount() const
    {
        return m_references;
    }

private:
    int &m_destructionCount;
    int m_references = 0;
};

void kisSharedPtrAddReference(KisNode *node)
{
    node->addReference();
}

bool kisSharedPtrRelease(KisNode *node)
{
    return node->releaseReference();
}

KUndo2CommandExtraData::~KUndo2CommandExtraData() = default;

class KisCropSavedExtraDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typesRectsAndDefaultNodeArePreserved();
    void cropNodeIsStronglyOwned();
    void cloneIsIndependentAndPreservesValues();
};

void KisCropSavedExtraDataContractTest::typesRectsAndDefaultNodeArePreserved()
{
    QCOMPARE(static_cast<int>(KisCropSavedExtraData::CROP_IMAGE), 0);
    QCOMPARE(static_cast<int>(KisCropSavedExtraData::RESIZE_IMAGE), 1);
    QCOMPARE(static_cast<int>(KisCropSavedExtraData::CROP_LAYER), 2);

    const QRect expectedRect(10, 20, 31, 47);
    const KisCropSavedExtraData::Type types[] = {
        KisCropSavedExtraData::CROP_IMAGE,
        KisCropSavedExtraData::RESIZE_IMAGE,
        KisCropSavedExtraData::CROP_LAYER,
    };

    for (const KisCropSavedExtraData::Type type : types) {
        KisCropSavedExtraData data(type, expectedRect);

        QCOMPARE(data.type(), type);
        QCOMPARE(data.cropRect(), expectedRect);
        QVERIFY(data.cropNode().isNull());
    }
}

void KisCropSavedExtraDataContractTest::cropNodeIsStronglyOwned()
{
    int destructionCount = 0;
    KisNode *const nodeAddress = new KisNode(destructionCount);

    {
        KisNodeSP callerNode(nodeAddress);
        QCOMPARE(nodeAddress->referenceCount(), 1);

        KisCropSavedExtraData data(KisCropSavedExtraData::CROP_LAYER, QRect(3, 5, 7, 11), callerNode);
        QCOMPARE(nodeAddress->referenceCount(), 2);

        callerNode.clear();
        QCOMPARE(nodeAddress->referenceCount(), 1);
        QCOMPARE(data.cropNode().data(), nodeAddress);
        QCOMPARE(nodeAddress->referenceCount(), 1);
        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

void KisCropSavedExtraDataContractTest::cloneIsIndependentAndPreservesValues()
{
    int destructionCount = 0;
    KisNode *const nodeAddress = new KisNode(destructionCount);
    const QRect expectedRect(13, 17, 19, 23);
    KUndo2CommandExtraData *cloneBase = nullptr;

    {
        KisNodeSP callerNode(nodeAddress);
        KisCropSavedExtraData original(KisCropSavedExtraData::RESIZE_IMAGE, expectedRect, callerNode);

        cloneBase = original.clone();
        auto *const clone = dynamic_cast<KisCropSavedExtraData *>(cloneBase);
        QVERIFY(clone);
        QVERIFY(clone != &original);

        callerNode.clear();
        QCOMPARE(nodeAddress->referenceCount(), 2);
        QCOMPARE(clone->type(), KisCropSavedExtraData::RESIZE_IMAGE);
        QCOMPARE(clone->cropRect(), expectedRect);
        QCOMPARE(clone->cropNode().data(), nodeAddress);
    }

    QCOMPARE(destructionCount, 0);
    QCOMPARE(nodeAddress->referenceCount(), 1);

    auto *const clone = dynamic_cast<KisCropSavedExtraData *>(cloneBase);
    QVERIFY(clone);
    QCOMPARE(clone->type(), KisCropSavedExtraData::RESIZE_IMAGE);
    QCOMPARE(clone->cropRect(), expectedRect);
    QCOMPARE(clone->cropNode().data(), nodeAddress);

    delete cloneBase;
    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisCropSavedExtraDataContractTest)

#include "KisCropSavedExtraDataContractTest.moc"
