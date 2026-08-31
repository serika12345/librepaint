/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_processing_information.h"

#include "KisProcessingInformationPaintDeviceOwnership_p.h"
#include "kis_shared_ptr.h"

#include <QTest>

namespace
{

struct ReferenceToken {
    int references = 0;
};

KisPaintDevice *paintDevicePointer(ReferenceToken *token)
{
    return reinterpret_cast<KisPaintDevice *>(token);
}

KisSelection *selectionPointer(ReferenceToken *token)
{
    return reinterpret_cast<KisSelection *>(token);
}

ReferenceToken *referenceToken(KisPaintDevice *device)
{
    return reinterpret_cast<ReferenceToken *>(device);
}

ReferenceToken *referenceToken(KisSelection *selection)
{
    return reinterpret_cast<ReferenceToken *>(selection);
}

KisPaintDeviceSP sharedPaintDevice(ReferenceToken *token)
{
    return KisPaintDeviceSP(paintDevicePointer(token));
}

KisSelectionSP sharedSelection(ReferenceToken *token)
{
    return KisSelectionSP(selectionPointer(token));
}

void compareConstInformation(const KisConstProcessingInformation &information,
                             ReferenceToken *device,
                             ReferenceToken *selection,
                             const QPoint &topLeft)
{
    QCOMPARE(information.paintDevice().data(), paintDevicePointer(device));
    QCOMPARE(information.selection().data(), selectionPointer(selection));
    QCOMPARE(information.topLeft(), topLeft);
}

template<typename Information>
void assignInformation(Information &target, const Information &source)
{
    target = source;
}

} // namespace

void kisSharedPtrAddReference(KisPaintDevice *device)
{
    ++referenceToken(device)->references;
}

bool kisSharedPtrRelease(KisPaintDevice *device)
{
    --referenceToken(device)->references;
    return true;
}

void kisSharedPtrAddReference(KisSelection *selection)
{
    ++referenceToken(selection)->references;
}

bool kisSharedPtrRelease(KisSelection *selection)
{
    --referenceToken(selection)->references;
    return true;
}

class KisProcessingInformationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constConstructionCopiesValuesAndRetainsOwners();
    void constCopyAndAssignmentOwnIndependentState();
    void mutableAndConstViewsShareTheCapturedValues();
    void mutableCopyAndAssignmentOwnIndependentState();
};

void KisProcessingInformationContractTest::constConstructionCopiesValuesAndRetainsOwners()
{
    ReferenceToken device;
    ReferenceToken selection;
    const QPoint expectedTopLeft(13, -8);

    {
        KisPaintDeviceSP callerDevice = sharedPaintDevice(&device);
        KisSelectionSP callerSelection = sharedSelection(&selection);
        QPoint callerTopLeft = expectedTopLeft;
        KisConstProcessingInformation information(callerDevice, callerTopLeft, callerSelection);

        callerTopLeft = QPoint(101, 103);
        callerDevice.clear();
        callerSelection.clear();

        QVERIFY(device.references > 0);
        QVERIFY(selection.references > 0);
        compareConstInformation(information, &device, &selection, expectedTopLeft);
    }

    QCOMPARE(device.references, 0);
    QCOMPARE(selection.references, 0);
}

void KisProcessingInformationContractTest::constCopyAndAssignmentOwnIndependentState()
{
    ReferenceToken firstDevice;
    ReferenceToken firstSelection;
    ReferenceToken secondDevice;
    ReferenceToken secondSelection;
    const QPoint firstTopLeft(5, 7);
    const QPoint secondTopLeft(-11, 17);

    {
        KisConstProcessingInformation original(sharedPaintDevice(&firstDevice),
                                               firstTopLeft,
                                               sharedSelection(&firstSelection));
        KisConstProcessingInformation copied(original);
        KisConstProcessingInformation assigned(sharedPaintDevice(&secondDevice),
                                               secondTopLeft,
                                               sharedSelection(&secondSelection));
        assigned = original;
        assignInformation(assigned, assigned);

        KisConstProcessingInformation replacement(sharedPaintDevice(&secondDevice),
                                                  secondTopLeft,
                                                  sharedSelection(&secondSelection));
        original = replacement;

        compareConstInformation(original, &secondDevice, &secondSelection, secondTopLeft);
        compareConstInformation(copied, &firstDevice, &firstSelection, firstTopLeft);
        compareConstInformation(assigned, &firstDevice, &firstSelection, firstTopLeft);
    }

    QCOMPARE(firstDevice.references, 0);
    QCOMPARE(firstSelection.references, 0);
    QCOMPARE(secondDevice.references, 0);
    QCOMPARE(secondSelection.references, 0);
}

void KisProcessingInformationContractTest::mutableAndConstViewsShareTheCapturedValues()
{
    ReferenceToken device;
    ReferenceToken selection;
    const QPoint expectedTopLeft(19, -23);

    {
        KisPaintDeviceSP callerDevice = sharedPaintDevice(&device);
        KisSelectionSP callerSelection = sharedSelection(&selection);
        KisProcessingInformation information(callerDevice, expectedTopLeft, callerSelection);
        const KisConstProcessingInformation &constView = information;

        callerDevice.clear();
        callerSelection.clear();

        QVERIFY(device.references > 0);
        QVERIFY(selection.references > 0);
        QCOMPARE(information.paintDevice().data(), paintDevicePointer(&device));
        compareConstInformation(constView, &device, &selection, expectedTopLeft);
    }

    QCOMPARE(device.references, 0);
    QCOMPARE(selection.references, 0);
}

void KisProcessingInformationContractTest::mutableCopyAndAssignmentOwnIndependentState()
{
    ReferenceToken firstDevice;
    ReferenceToken firstSelection;
    ReferenceToken secondDevice;
    ReferenceToken secondSelection;
    const QPoint firstTopLeft(29, 31);
    const QPoint secondTopLeft(-37, 41);

    {
        KisProcessingInformation original(sharedPaintDevice(&firstDevice),
                                          firstTopLeft,
                                          sharedSelection(&firstSelection));
        KisProcessingInformation copied(original);
        KisProcessingInformation assigned(sharedPaintDevice(&secondDevice),
                                          secondTopLeft,
                                          sharedSelection(&secondSelection));
        assigned = original;
        assignInformation(assigned, assigned);

        KisProcessingInformation replacement(sharedPaintDevice(&secondDevice),
                                             secondTopLeft,
                                             sharedSelection(&secondSelection));
        original = replacement;

        QCOMPARE(original.paintDevice().data(), paintDevicePointer(&secondDevice));
        compareConstInformation(original, &secondDevice, &secondSelection, secondTopLeft);
        QCOMPARE(copied.paintDevice().data(), paintDevicePointer(&firstDevice));
        compareConstInformation(copied, &firstDevice, &firstSelection, firstTopLeft);
        QCOMPARE(assigned.paintDevice().data(), paintDevicePointer(&firstDevice));
        compareConstInformation(assigned, &firstDevice, &firstSelection, firstTopLeft);
    }

    QCOMPARE(firstDevice.references, 0);
    QCOMPARE(firstSelection.references, 0);
    QCOMPARE(secondDevice.references, 0);
    QCOMPARE(secondSelection.references, 0);
}

QTEST_GUILESS_MAIN(KisProcessingInformationContractTest)

#include "KisProcessingInformationContractTest.moc"
