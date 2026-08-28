/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QHash>
#include <QSignalMapper>
#include <QSignalSpy>
#include <QTest>

#include <type_traits>

#include "kis_paintop_options_model.h"
#include "kis_paintop_options_model_source_p.h"

namespace
{

using OptionState = KisPaintOpOptionsModelSource::OptionState;

struct StateConnection
{
    QSignalMapper *mapper {nullptr};
    int row {-1};
    bool checkedStateConnected {false};
};

QHash<const KisPaintOpOption *, OptionState> optionStates;
QHash<const KisPaintOpOption *, StateConnection> stateConnections;
int setCheckedCount = 0;

KisPaintOpOption *optionPointer(quintptr id)
{
    return reinterpret_cast<KisPaintOpOption *>(id);
}

OptionState state(const QString &objectName,
                  KisPaintOpOption::PaintopCategory category,
                  bool checkable,
                  bool checked,
                  bool enabled)
{
    return {objectName, category, checkable, checked, enabled};
}

void resetSourceState()
{
    optionStates.clear();
    stateConnections.clear();
    setCheckedCount = 0;
}

void emitStateChanged(const KisPaintOpOption *option)
{
    const StateConnection connection = stateConnections.value(option);
    QVERIFY(connection.mapper);
    connection.mapper->mappedInt(connection.row);
}

QModelIndex optionIndex(const KisPaintOpOptionListModel &model,
                        KisPaintOpOption *option,
                        int widgetIndex)
{
    return model.indexOf(KisOptionInfo(option, widgetIndex, QString()));
}

}

namespace KisPaintOpOptionsModelSource
{

OptionState optionState(const KisPaintOpOption *option)
{
    return optionStates.value(option);
}

void setChecked(KisPaintOpOption *option, bool checked)
{
    OptionState value = optionStates.value(option);
    value.checked = checked;
    optionStates.insert(option, value);
    ++setCheckedCount;
}

void connectStateChanges(KisPaintOpOption *option,
                         QSignalMapper *mapper,
                         int row,
                         bool connectCheckedState)
{
    stateConnections.insert(option, {mapper, row, connectCheckedState});
}

}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

class KisPaintOpOptionsModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void optionInfoHasDeterministicValueAndIdentity();
    void categoryNamesRemainStable();
    void addingOptionsBuildsExpandedObservableRows();
    void modelCheckStateUpdatesCheckableOption();
    void optionStateChangesAndExplicitNotificationReachObservers();
};

void KisPaintOpOptionsModelContractTest::init()
{
    resetSourceState();
}

void KisPaintOpOptionsModelContractTest::optionInfoHasDeterministicValueAndIdentity()
{
    static_assert(std::is_base_of_v<BaseOptionCategorizedListModel,
                                    KisPaintOpOptionListModel>);

    KisOptionInfo empty;
    QVERIFY(empty.label.isEmpty());
    QCOMPARE(empty.option, nullptr);
    QCOMPARE(empty.index, -1);
    QCOMPARE(empty, KisOptionInfo());

    KisPaintOpOption *first = optionPointer(1);
    KisPaintOpOption *second = optionPointer(2);
    optionStates.insert(first, state(QStringLiteral("opacity"),
                                     KisPaintOpOption::GENERAL,
                                     true, true, true));
    optionStates.insert(second, optionStates.value(first));

    KisOptionInfo original(first, 7, QStringLiteral("Opacity"));
    KisOptionInfo copy(original);
    QCOMPARE(copy.label, QStringLiteral("Opacity"));
    QCOMPARE(copy.option, first);
    QCOMPARE(copy.index, 7);
    QCOMPARE(copy, original);

    OptionInfoToQStringConverter converter;
    QCOMPARE(converter(copy), QStringLiteral("Opacity"));

    KisOptionInfo sameState(second, 7, QStringLiteral("Different label"));
    QVERIFY(original == sameState);

    optionStates[second].enabled = false;
    QVERIFY(original == sameState);
    optionStates[second].objectName = QStringLiteral("size");
    QVERIFY(!(original == sameState));
    optionStates[second] = optionStates.value(first);
    optionStates[second].category = KisPaintOpOption::COLOR;
    QVERIFY(!(original == sameState));
    optionStates[second] = optionStates.value(first);
    optionStates[second].checkable = false;
    QVERIFY(!(original == sameState));
    optionStates[second] = optionStates.value(first);
    optionStates[second].checked = false;
    QVERIFY(!(original == sameState));
    optionStates[second] = optionStates.value(first);
    sameState.index = 8;
    QVERIFY(!(original == sameState));
    sameState.index = 7;
    sameState.option = nullptr;
    QVERIFY(!(original == sameState));
}

void KisPaintOpOptionsModelContractTest::categoryNamesRemainStable()
{
    QCOMPARE(KisPaintOpOptionListModel::categoryName(KisPaintOpOption::GENERAL),
             QStringLiteral("General"));
    QCOMPARE(KisPaintOpOptionListModel::categoryName(KisPaintOpOption::COLOR),
             QStringLiteral("Color"));
    QCOMPARE(KisPaintOpOptionListModel::categoryName(KisPaintOpOption::TEXTURE),
             QStringLiteral("Texture"));
    QCOMPARE(KisPaintOpOptionListModel::categoryName(KisPaintOpOption::FILTER),
             QStringLiteral("Filter"));
    QCOMPARE(KisPaintOpOptionListModel::categoryName(KisPaintOpOption::MASKING_BRUSH),
             QStringLiteral("Masked Brush"));
}

void KisPaintOpOptionsModelContractTest::addingOptionsBuildsExpandedObservableRows()
{
    KisPaintOpOption *opacity = optionPointer(1);
    KisPaintOpOption *spacing = optionPointer(2);
    optionStates.insert(opacity, state(QStringLiteral("opacity"),
                                       KisPaintOpOption::GENERAL,
                                       true, false, false));
    optionStates.insert(spacing, state(QStringLiteral("spacing"),
                                       KisPaintOpOption::GENERAL,
                                       false, false, true));

    KisPaintOpOptionListModel model(nullptr);
    model.addPaintOpOption(opacity, 4, QStringLiteral("Opacity"), QStringLiteral("General"));
    model.addPaintOpOption(spacing, 5, QStringLiteral("Spacing"), QStringLiteral("General"));

    const QModelIndex opacityIndex = optionIndex(model, opacity, 4);
    const QModelIndex spacingIndex = optionIndex(model, spacing, 5);
    QVERIFY(opacityIndex.isValid());
    QVERIFY(spacingIndex.isValid());
    QCOMPARE(model.data(opacityIndex).toString(), QStringLiteral("Opacity"));
    QCOMPARE(model.data(opacityIndex, Qt::CheckStateRole).toInt(), int(Qt::Unchecked));
    QVERIFY(model.flags(opacityIndex).testFlag(Qt::ItemIsUserCheckable));
    QVERIFY(!model.flags(opacityIndex).testFlag(Qt::ItemIsEnabled));
    QVERIFY(!model.data(spacingIndex, Qt::CheckStateRole).isValid());
    QVERIFY(!model.flags(spacingIndex).testFlag(Qt::ItemIsUserCheckable));
    QVERIFY(model.flags(spacingIndex).testFlag(Qt::ItemIsEnabled));

    auto *categoryItem = model.categoriesMapper()->fetchCategory(QStringLiteral("General"));
    QVERIFY(categoryItem);
    const QModelIndex categoryIndex = model.index(
        model.categoriesMapper()->rowFromItem(categoryItem));
    QVERIFY(model.data(categoryIndex, BaseOptionCategorizedListModel::ExpandCategoryRole).toBool());
    QVERIFY(stateConnections.value(opacity).checkedStateConnected);
    QVERIFY(!stateConnections.value(spacing).checkedStateConnected);
}

void KisPaintOpOptionsModelContractTest::modelCheckStateUpdatesCheckableOption()
{
    KisPaintOpOption *opacity = optionPointer(1);
    optionStates.insert(opacity, state(QStringLiteral("opacity"),
                                       KisPaintOpOption::GENERAL,
                                       true, false, true));
    KisPaintOpOptionListModel model(nullptr);
    model.addPaintOpOption(opacity, 4, QStringLiteral("Opacity"), QStringLiteral("General"));
    const QModelIndex index = optionIndex(model, opacity, 4);
    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    QVERIFY(!model.setData(QModelIndex(), Qt::Checked, Qt::CheckStateRole));
    QVERIFY(model.setData(index, Qt::Checked, Qt::CheckStateRole));

    QCOMPARE(setCheckedCount, 1);
    QVERIFY(optionStates.value(opacity).checked);
    QCOMPARE(model.data(index, Qt::CheckStateRole).toInt(), int(Qt::Checked));
    QVERIFY(changedSpy.count() >= 1);

    QVERIFY(model.setData(index, QStringLiteral("ignored"), Qt::EditRole));
    QCOMPARE(setCheckedCount, 1);
}

void KisPaintOpOptionsModelContractTest::optionStateChangesAndExplicitNotificationReachObservers()
{
    KisPaintOpOption *opacity = optionPointer(1);
    optionStates.insert(opacity, state(QStringLiteral("opacity"),
                                       KisPaintOpOption::GENERAL,
                                       true, false, true));
    KisPaintOpOptionListModel model(nullptr);
    model.addPaintOpOption(opacity, 4, QStringLiteral("Opacity"), QStringLiteral("General"));
    const QModelIndex index = optionIndex(model, opacity, 4);
    QSignalSpy changedSpy(&model, &QAbstractItemModel::dataChanged);

    optionStates[opacity].checked = true;
    optionStates[opacity].enabled = false;
    emitStateChanged(opacity);

    QCOMPARE(model.data(index, Qt::CheckStateRole).toInt(), int(Qt::Checked));
    QVERIFY(!model.flags(index).testFlag(Qt::ItemIsEnabled));
    QVERIFY(changedSpy.count() >= 1);

    changedSpy.clear();
    model.signalDataChanged(index);
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.at(0).at(0).toModelIndex(), index);
    QCOMPARE(changedSpy.at(0).at(1).toModelIndex(), index);
}

QTEST_MAIN(KisPaintOpOptionsModelContractTest)

#include "KisPaintOpOptionsModelContractTest.moc"
