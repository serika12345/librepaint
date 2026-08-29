/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisSpinBoxI18nHelper.h"

#include <QDoubleSpinBox>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
template<typename T, typename = void>
struct CanSetSpinBoxText : std::false_type {
};

template<typename T>
struct CanSetSpinBoxText<
    T,
    std::void_t<decltype(KisSpinBoxI18nHelper::setText(std::declval<T *>(), std::declval<QStringView>()))>>
    : std::true_type {
};

static_assert(CanSetSpinBoxText<QSpinBox>::value);
static_assert(CanSetSpinBoxText<QDoubleSpinBox>::value);
static_assert(!CanSetSpinBoxText<KisSelectionPropertySliderBase>::value);
} // namespace

class KisSpinBoxI18nHelperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void spinBoxTemplatesSplitAroundPlaceholder();
    void templateWithoutPlaceholderBecomesSuffix();
    void installUpdatesImmediatelyAndWhenValueChanges();
    void manualUpdateUsesCurrentValueWhileSignalsAreBlocked();
    void updateRejectsMissingAndInvalidHandlerProperty();
    void selectionPropertySliderOverloadRemainsDeleted();
};

void KisSpinBoxI18nHelperContractTest::spinBoxTemplatesSplitAroundPlaceholder()
{
    QSpinBox integerSpinBox;
    const QString integerTemplate = QStringLiteral("about {n} items");

    KisSpinBoxI18nHelper::setText(&integerSpinBox, integerTemplate);

    QCOMPARE(integerSpinBox.prefix(), QStringLiteral("about "));
    QCOMPARE(integerSpinBox.suffix(), QStringLiteral(" items"));

    QDoubleSpinBox doubleSpinBox;
    const QString doubleTemplate = QStringLiteral("scale {n}%");

    KisSpinBoxI18nHelper::setText(&doubleSpinBox, doubleTemplate);

    QCOMPARE(doubleSpinBox.prefix(), QStringLiteral("scale "));
    QCOMPARE(doubleSpinBox.suffix(), QStringLiteral("%"));
}

void KisSpinBoxI18nHelperContractTest::templateWithoutPlaceholderBecomesSuffix()
{
    QSpinBox integerSpinBox;
    integerSpinBox.setPrefix(QStringLiteral("old prefix"));
    integerSpinBox.setSuffix(QStringLiteral("old suffix"));
    const QString integerTemplate = QStringLiteral("items");

    KisSpinBoxI18nHelper::setText(&integerSpinBox, integerTemplate);

    QVERIFY(integerSpinBox.prefix().isEmpty());
    QCOMPARE(integerSpinBox.suffix(), integerTemplate);

    QDoubleSpinBox doubleSpinBox;
    doubleSpinBox.setPrefix(QStringLiteral("old prefix"));
    doubleSpinBox.setSuffix(QStringLiteral("old suffix"));
    const QString doubleTemplate = QStringLiteral("percent");

    KisSpinBoxI18nHelper::setText(&doubleSpinBox, doubleTemplate);

    QVERIFY(doubleSpinBox.prefix().isEmpty());
    QCOMPARE(doubleSpinBox.suffix(), doubleTemplate);
}

void KisSpinBoxI18nHelperContractTest::installUpdatesImmediatelyAndWhenValueChanges()
{
    QSpinBox spinBox;
    spinBox.setValue(3);

    KisSpinBoxI18nHelper::install(&spinBox, [](int value) {
        return QStringLiteral("value %1: {n} units").arg(value);
    });

    QCOMPARE(spinBox.prefix(), QStringLiteral("value 3: "));
    QCOMPARE(spinBox.suffix(), QStringLiteral(" units"));

    spinBox.setValue(7);

    QCOMPARE(spinBox.prefix(), QStringLiteral("value 7: "));
    QCOMPARE(spinBox.suffix(), QStringLiteral(" units"));
}

void KisSpinBoxI18nHelperContractTest::manualUpdateUsesCurrentValueWhileSignalsAreBlocked()
{
    QSpinBox spinBox;
    spinBox.setValue(2);
    KisSpinBoxI18nHelper::install(&spinBox, [](int value) {
        return QStringLiteral("count %1: {n} rows").arg(value);
    });

    const QSignalBlocker blocker(&spinBox);
    spinBox.setValue(9);
    QCOMPARE(spinBox.prefix(), QStringLiteral("count 2: "));

    QVERIFY(KisSpinBoxI18nHelper::update(&spinBox));
    QCOMPARE(spinBox.prefix(), QStringLiteral("count 9: "));
    QCOMPARE(spinBox.suffix(), QStringLiteral(" rows"));
}

void KisSpinBoxI18nHelperContractTest::updateRejectsMissingAndInvalidHandlerProperty()
{
    QSpinBox spinBox;

    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QStringLiteral(
                             "KisSpinBoxI18nHelper::update called with .* but it does not have the property .*")));
    QVERIFY(!KisSpinBoxI18nHelper::update(&spinBox));

    spinBox.setProperty("_kis_KisSpinBoxI18nHelper_handler", QStringLiteral("invalid"));
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QStringLiteral(
                             "KisSpinBoxI18nHelper::update called with .* but its property .* is invalid")));
    QVERIFY(!KisSpinBoxI18nHelper::update(&spinBox));
}

void KisSpinBoxI18nHelperContractTest::selectionPropertySliderOverloadRemainsDeleted()
{
    QVERIFY(!CanSetSpinBoxText<KisSelectionPropertySliderBase>::value);
}

QTEST_MAIN(KisSpinBoxI18nHelperContractTest)

#include "KisSpinBoxI18nHelperContractTest.moc"
