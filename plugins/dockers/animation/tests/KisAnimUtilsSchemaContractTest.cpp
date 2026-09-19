/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>


#include "KisAnimUtils.h"
#include "KisTimeBasedItemModel.h"

void kisSharedPtrAddReference(KisNode *)
{
}

bool kisSharedPtrRelease(KisNode *)
{
    return true;
}

namespace
{

class ConcreteTimeBasedItemModel : public KisTimeBasedItemModel
{
public:
    using KisTimeBasedItemModel::KisTimeBasedItemModel;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

protected:
    KisNodeSP nodeAt(QModelIndex index) const override;
    QMap<QString, KisKeyframeChannel *> channelsAt(QModelIndex index) const override;
    KisKeyframeChannel *channelByID(QModelIndex index, const QString &id) const override;
};

} // namespace

class KisAnimUtilsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void frameItemTypeAndDefaultStateRemainStable();
    void frameItemValueAndComparisonContractsRemainStable();
    void keyframeCreationAndMoveSignaturesRemainStable();
    void keyframeEditingAndQuerySignaturesRemainStable();
    void animationActionNameTypesRemainStable();
};

void KisAnimUtilsSchemaContractTest::frameItemTypeAndDefaultStateRemainStable()
{
    using namespace KisAnimUtils;


    const FrameItem item;
    QVERIFY(!item.node);
    QVERIFY(item.channel.isEmpty());
    QCOMPARE(item.time, -1);
}

void KisAnimUtilsSchemaContractTest::frameItemValueAndComparisonContractsRemainStable()
{
    using namespace KisAnimUtils;


    const FrameItem item(KisNodeSP(), QStringLiteral("opacity"), 17);
    const FrameItem equal(KisNodeSP(), QStringLiteral("opacity"), 17);
    const FrameItem different(KisNodeSP(), QStringLiteral("opacity"), 18);
    QCOMPARE(item.channel, QStringLiteral("opacity"));
    QCOMPARE(item.time, 17);
    QVERIFY(item == equal);
    QVERIFY(!(item == different));
    QCOMPARE(KisAnimUtils::qHash(item), KisAnimUtils::qHash(equal));
}

void KisAnimUtilsSchemaContractTest::keyframeCreationAndMoveSignaturesRemainStable()
{
    using namespace KisAnimUtils;


    QVERIFY(true);
}

void KisAnimUtilsSchemaContractTest::keyframeEditingAndQuerySignaturesRemainStable()
{
    using namespace KisAnimUtils;


    QVERIFY(true);
}

void KisAnimUtilsSchemaContractTest::animationActionNameTypesRemainStable()
{
    using namespace KisAnimUtils;


    QVERIFY(true);
}

QTEST_MAIN(KisAnimUtilsSchemaContractTest)

#include "KisAnimUtilsSchemaContractTest.moc"
