/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisUpdateOutlineJobSelectionAccess_p.h"
#include "kis_update_outline_job.h"

#include <QTest>
#include <QVector>

namespace
{

struct SelectionToken {
    int references = 0;
};

enum class OperationType {
    RecalculateOutline,
    RecalculateThumbnail,
    NotifyChanged,
};

struct SelectionOperation {
    OperationType type;
    KisSelection *selection;
    QColor maskColor;
};

QVector<SelectionOperation> operations;

KisSelection *selectionPointer(SelectionToken *token)
{
    return reinterpret_cast<KisSelection *>(token);
}

SelectionToken *selectionToken(KisSelection *selection)
{
    return reinterpret_cast<SelectionToken *>(selection);
}

void compareOperation(int index, OperationType type, KisSelection *selection, const QColor &maskColor = QColor())
{
    const SelectionOperation &operation = operations.at(index);
    QVERIFY(operation.type == type);
    QCOMPARE(operation.selection, selection);
    QCOMPARE(operation.maskColor, maskColor);
}

class OtherSpontaneousJob : public KisSpontaneousJob
{
public:
    bool overrides(const KisSpontaneousJob *) override
    {
        return false;
    }

    void run() override
    {
    }

    int levelOfDetail() const override
    {
        return 0;
    }

    QString debugName() const override
    {
        return QStringLiteral("OtherSpontaneousJob");
    }
};

} // namespace

void kisSharedPtrAddReference(KisSelection *selection)
{
    ++selectionToken(selection)->references;
}

bool kisSharedPtrRelease(KisSelection *selection)
{
    --selectionToken(selection)->references;
    return true;
}

void KisUpdateOutlineJobSelectionAccess::recalculateOutlineCache(KisSelection *selection)
{
    operations.append({OperationType::RecalculateOutline, selection, QColor()});
}

void KisUpdateOutlineJobSelectionAccess::recalculateThumbnailImage(KisSelection *selection, const QColor &maskColor)
{
    operations.append({OperationType::RecalculateThumbnail, selection, maskColor});
}

void KisUpdateOutlineJobSelectionAccess::notifySelectionChanged(KisSelection *selection)
{
    operations.append({OperationType::NotifyChanged, selection, QColor()});
}

class KisUpdateOutlineJobContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionRetainsSelectionAndReportsSchedulingMetadata();
    void runWithoutThumbnailRefreshesOutlineThenNotifies();
    void runWithThumbnailPreservesColorAndOperationOrder();
    void overrideDecisionUsesConcreteJobType();
};

void KisUpdateOutlineJobContractTest::constructionRetainsSelectionAndReportsSchedulingMetadata()
{
    SelectionToken token;
    KisSelectionSP selection(selectionPointer(&token));

    {
        KisUpdateOutlineJob job(selection, false, QColor(Qt::transparent));
        selection.clear();

        QCOMPARE(token.references, 1);
        QCOMPARE(job.levelOfDetail(), 0);
        QCOMPARE(job.debugName(), QStringLiteral("KisUpdateOutlineJob"));
        QVERIFY(!job.isExclusive());
    }

    QCOMPARE(token.references, 0);
}

void KisUpdateOutlineJobContractTest::runWithoutThumbnailRefreshesOutlineThenNotifies()
{
    operations.clear();
    SelectionToken token;
    KisSelection *const selectionAddress = selectionPointer(&token);
    KisUpdateOutlineJob job(KisSelectionSP(selectionAddress), false, QColor(Qt::magenta));

    job.run();

    QCOMPARE(operations.size(), 2);
    compareOperation(0, OperationType::RecalculateOutline, selectionAddress);
    compareOperation(1, OperationType::NotifyChanged, selectionAddress);
}

void KisUpdateOutlineJobContractTest::runWithThumbnailPreservesColorAndOperationOrder()
{
    operations.clear();
    SelectionToken token;
    KisSelection *const selectionAddress = selectionPointer(&token);
    const QColor maskColor(17, 83, 149, 211);
    KisUpdateOutlineJob job(KisSelectionSP(selectionAddress), true, maskColor);

    job.run();

    QCOMPARE(operations.size(), 3);
    compareOperation(0, OperationType::RecalculateOutline, selectionAddress);
    compareOperation(1, OperationType::RecalculateThumbnail, selectionAddress, maskColor);
    compareOperation(2, OperationType::NotifyChanged, selectionAddress);
}

void KisUpdateOutlineJobContractTest::overrideDecisionUsesConcreteJobType()
{
    SelectionToken firstToken;
    SelectionToken secondToken;
    KisUpdateOutlineJob first(KisSelectionSP(selectionPointer(&firstToken)), false, QColor());
    KisUpdateOutlineJob otherSelection(KisSelectionSP(selectionPointer(&secondToken)), true, QColor(Qt::black));
    OtherSpontaneousJob otherType;

    QVERIFY(first.overrides(&otherSelection));
    QVERIFY(!first.overrides(&otherType));
    QVERIFY(!first.overrides(nullptr));
}

QTEST_GUILESS_MAIN(KisUpdateOutlineJobContractTest)

#include "KisUpdateOutlineJobContractTest.moc"
