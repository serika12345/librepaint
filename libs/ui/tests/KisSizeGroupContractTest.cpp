/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/kis_size_group.h"

#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QSignalSpy>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

namespace
{

class HintWidget final : public QWidget
{
public:
    explicit HintWidget(const QSize &hint, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_hint(hint)
    {
    }

    QSize sizeHint() const override
    {
        return m_hint;
    }

private:
    QSize m_hint;
};

class SizeGroupProbe final : public KisSizeGroup
{
public:
    explicit SizeGroupProbe(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~SizeGroupProbe() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

void verifyBoxLayoutMode(KisSizeGroup::mode mode, const QSize &firstExpected, const QSize &secondExpected)
{
    QWidget container;
    QHBoxLayout layout(&container);
    HintWidget first(QSize(30, 20), &container);
    HintWidget second(QSize(50, 40), &container);
    layout.addWidget(&first);
    layout.addWidget(&second);

    KisSizeGroup group(nullptr, mode);
    group.addWidget(&first);
    group.addWidget(&second);

    QTRY_COMPARE(layout.itemAt(0)->sizeHint(), firstExpected);
    QTRY_COMPARE(layout.itemAt(1)->sizeHint(), secondExpected);
}

} // namespace

class KisSizeGroupContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultModeEnumsParentAndVirtualLifetime();
    void settersReturnConfiguredModeAndHiddenPolicy();
    void boxLayoutSynchronizesAxesAndRestoresRemovedWidget();
    void gridAndFormLayoutsPreservePositionsWhileSynchronizing();
    void hiddenWidgetsAreExcludedAndReincluded();
};

void KisSizeGroupContractTest::defaultModeEnumsParentAndVirtualLifetime()
{
    QCOMPARE(static_cast<int>(KisSizeGroup::KIS_SIZE_GROUP_NONE), 0);
    QCOMPARE(static_cast<int>(KisSizeGroup::KIS_SIZE_GROUP_HORIZONTAL), 1);
    QCOMPARE(static_cast<int>(KisSizeGroup::KIS_SIZE_GROUP_VERTICAL), 2);
    QCOMPARE(static_cast<int>(KisSizeGroup::KIS_SIZE_GROUP_BOTH), 3);

    QObject *parent = new QObject;
    KisSizeGroup *ownedGroup = new KisSizeGroup(parent);
    QSignalSpy destroyedSpy(ownedGroup, &QObject::destroyed);
    QCOMPARE(ownedGroup->parent(), parent);
    QCOMPARE(ownedGroup->getMode(), KisSizeGroup::KIS_SIZE_GROUP_HORIZONTAL);
    QVERIFY(!ownedGroup->isIgnoreHidden());
    delete parent;
    QCOMPARE(destroyedSpy.count(), 1);

    bool destroyed = false;
    std::unique_ptr<KisSizeGroup> polymorphicGroup(new SizeGroupProbe(&destroyed));
    polymorphicGroup.reset();
    QVERIFY(destroyed);
}

void KisSizeGroupContractTest::settersReturnConfiguredModeAndHiddenPolicy()
{
    KisSizeGroup group(nullptr, KisSizeGroup::KIS_SIZE_GROUP_VERTICAL, true);
    QCOMPARE(group.getMode(), KisSizeGroup::KIS_SIZE_GROUP_VERTICAL);
    QVERIFY(group.isIgnoreHidden());

    group.setMode(KisSizeGroup::KIS_SIZE_GROUP_BOTH);
    QCOMPARE(group.getMode(), KisSizeGroup::KIS_SIZE_GROUP_BOTH);
    group.setMode(KisSizeGroup::KIS_SIZE_GROUP_NONE);
    QCOMPARE(group.getMode(), KisSizeGroup::KIS_SIZE_GROUP_NONE);

    group.setIgnoreHidden(false);
    QVERIFY(!group.isIgnoreHidden());
}

void KisSizeGroupContractTest::boxLayoutSynchronizesAxesAndRestoresRemovedWidget()
{
    verifyBoxLayoutMode(KisSizeGroup::KIS_SIZE_GROUP_NONE, QSize(30, 20), QSize(50, 40));
    verifyBoxLayoutMode(KisSizeGroup::KIS_SIZE_GROUP_HORIZONTAL, QSize(50, 20), QSize(50, 40));
    verifyBoxLayoutMode(KisSizeGroup::KIS_SIZE_GROUP_VERTICAL, QSize(30, 40), QSize(50, 40));
    verifyBoxLayoutMode(KisSizeGroup::KIS_SIZE_GROUP_BOTH, QSize(50, 40), QSize(50, 40));

    QWidget container;
    QHBoxLayout layout(&container);
    HintWidget first(QSize(30, 20), &container);
    HintWidget second(QSize(50, 40), &container);
    layout.addWidget(&first);
    layout.addWidget(&second);

    KisSizeGroup group(nullptr, KisSizeGroup::KIS_SIZE_GROUP_BOTH);
    group.addWidget(&first);
    group.addWidget(&second);
    QTRY_COMPARE(layout.itemAt(0)->sizeHint(), QSize(50, 40));

    group.removeWidget(&first);
    QTRY_COMPARE(layout.itemAt(layout.indexOf(&first))->sizeHint(), QSize(30, 20));
    QCOMPARE(layout.itemAt(layout.indexOf(&second))->sizeHint(), QSize(50, 40));
}

void KisSizeGroupContractTest::gridAndFormLayoutsPreservePositionsWhileSynchronizing()
{
    QWidget gridContainer;
    QGridLayout grid(&gridContainer);
    HintWidget gridFirst(QSize(30, 20), &gridContainer);
    HintWidget gridSecond(QSize(50, 40), &gridContainer);
    grid.addWidget(&gridFirst, 1, 2, 1, 2);
    grid.addWidget(&gridSecond, 3, 4);

    KisSizeGroup gridGroup(nullptr, KisSizeGroup::KIS_SIZE_GROUP_BOTH);
    gridGroup.addWidget(&gridFirst);
    gridGroup.addWidget(&gridSecond);
    QTRY_COMPARE(grid.itemAt(grid.indexOf(&gridFirst))->sizeHint(), QSize(50, 40));
    QTRY_COMPARE(grid.itemAt(grid.indexOf(&gridSecond))->sizeHint(), QSize(50, 40));

    int row = -1;
    int column = -1;
    int rowSpan = -1;
    int columnSpan = -1;
    grid.getItemPosition(grid.indexOf(&gridFirst), &row, &column, &rowSpan, &columnSpan);
    QCOMPARE(row, 1);
    QCOMPARE(column, 2);
    QCOMPARE(rowSpan, 1);
    QCOMPARE(columnSpan, 2);

    QWidget formContainer;
    QFormLayout form(&formContainer);
    HintWidget formFirst(QSize(30, 20), &formContainer);
    HintWidget formSecond(QSize(50, 40), &formContainer);
    form.addRow(QStringLiteral("First"), &formFirst);
    form.addRow(QStringLiteral("Second"), &formSecond);

    KisSizeGroup formGroup(nullptr, KisSizeGroup::KIS_SIZE_GROUP_HORIZONTAL);
    formGroup.addWidget(&formFirst);
    formGroup.addWidget(&formSecond);
    QTRY_COMPARE(form.itemAt(form.indexOf(&formFirst))->sizeHint(), QSize(50, 20));
    QTRY_COMPARE(form.itemAt(form.indexOf(&formSecond))->sizeHint(), QSize(50, 40));

    QFormLayout::ItemRole role = QFormLayout::LabelRole;
    form.getItemPosition(form.indexOf(&formFirst), &row, &role);
    QCOMPARE(row, 0);
    QCOMPARE(role, QFormLayout::FieldRole);
}

void KisSizeGroupContractTest::hiddenWidgetsAreExcludedAndReincluded()
{
    QWidget container;
    QVBoxLayout layout(&container);
    HintWidget first(QSize(30, 20), &container);
    HintWidget second(QSize(50, 40), &container);
    layout.addWidget(&first);
    layout.addWidget(&second);
    container.show();
    QTRY_VERIFY(first.isVisible());
    QTRY_VERIFY(second.isVisible());

    KisSizeGroup group(nullptr, KisSizeGroup::KIS_SIZE_GROUP_HORIZONTAL, true);
    group.addWidget(&first);
    group.addWidget(&second);
    QTRY_COMPARE(layout.itemAt(layout.indexOf(&first))->sizeHint(), QSize(50, 20));

    second.hide();
    QTRY_COMPARE(layout.itemAt(layout.indexOf(&first))->sizeHint(), QSize(30, 20));

    second.show();
    QTRY_COMPARE(layout.itemAt(layout.indexOf(&first))->sizeHint(), QSize(50, 20));
}

QTEST_MAIN(KisSizeGroupContractTest)

#include "KisSizeGroupContractTest.moc"
