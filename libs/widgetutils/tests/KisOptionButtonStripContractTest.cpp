/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisOptionButtonStrip.h"
#include "KoGroupButton.h"

#include <QAbstractButton>
#include <QHBoxLayout>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

namespace
{

[[noreturn]] void unexpectedAssert(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion '%s' at %s:%d", assertion, file, line);
}

} // namespace

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    unexpectedAssert(assertion, file, line);
}

class KisOptionButtonStripContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void groupButtonPositionsConstructionAndLifetime();
    void stripOwnsItsLayoutAndButtons();
    void addOverloadsPreserveOrderContentAndGrouping();
    void exclusiveSelectionForwardsPointerAndIndexSignals();
};

void KisOptionButtonStripContractTest::groupButtonPositionsConstructionAndLifetime()
{
    QCOMPARE(int(KoGroupButton::NoGroup), 0);
    QCOMPARE(int(KoGroupButton::GroupLeft), 1);
    QCOMPARE(int(KoGroupButton::GroupRight), 2);
    QCOMPARE(int(KoGroupButton::GroupCenter), 3);

    QWidget parent;
    KoGroupButton defaultButton(&parent);
    KoGroupButton positionedButton(KoGroupButton::GroupLeft, &parent);

    QCOMPARE(defaultButton.parentWidget(), &parent);
    QCOMPARE(defaultButton.groupPosition(), KoGroupButton::NoGroup);
    QCOMPARE(positionedButton.parentWidget(), &parent);
    QCOMPARE(positionedButton.groupPosition(), KoGroupButton::GroupLeft);
    QCOMPARE(positionedButton.sizePolicy().horizontalPolicy(), QSizePolicy::Preferred);
    QCOMPARE(positionedButton.sizePolicy().verticalPolicy(), QSizePolicy::Fixed);

    positionedButton.setGroupPosition(KoGroupButton::GroupCenter);
    QCOMPARE(positionedButton.groupPosition(), KoGroupButton::GroupCenter);
    QVERIFY(positionedButton.setProperty("groupPosition", KoGroupButton::GroupRight));
    QCOMPARE(positionedButton.groupPosition(), KoGroupButton::GroupRight);

    QPointer<KoGroupButton> button = new KoGroupButton;
    QAbstractButton *base = button;
    delete base;
    QVERIFY(button.isNull());
}

void KisOptionButtonStripContractTest::stripOwnsItsLayoutAndButtons()
{
    auto *parent = new QWidget;
    QPointer<KisOptionButtonStrip> strip = new KisOptionButtonStrip(parent);
    auto *layout = qobject_cast<QHBoxLayout *>(strip->layout());

    QCOMPARE(strip->parentWidget(), parent);
    QVERIFY(layout);
    QCOMPARE(layout->spacing(), 0);
    QCOMPARE(layout->contentsMargins(), QMargins());
    QCOMPARE(layout->count(), 1);
    QCOMPARE(strip->sizePolicy().horizontalPolicy(), QSizePolicy::Minimum);
    QCOMPARE(strip->sizePolicy().verticalPolicy(), QSizePolicy::Preferred);

    QPointer<KoGroupButton> button = strip->addButton(QStringLiteral("owned"));
    QCOMPARE(button->parentWidget(), strip.data());

    delete parent;
    QVERIFY(strip.isNull());
    QVERIFY(button.isNull());
}

void KisOptionButtonStripContractTest::addOverloadsPreserveOrderContentAndGrouping()
{
    KisOptionButtonStrip strip;
    QCOMPARE(strip.button(-1), nullptr);
    QCOMPARE(strip.button(0), nullptr);
    QVERIFY(strip.buttons().isEmpty());
    QCOMPARE(strip.checkedButton(), nullptr);
    QCOMPARE(strip.checkedButtonIndex(), -1);

    QPixmap pixmap(4, 3);
    pixmap.fill(Qt::red);
    KoGroupButton *first = strip.addButton(QIcon(pixmap), QStringLiteral("icon"));
    KoGroupButton *second = strip.addButton(QStringLiteral("text"));
    KoGroupButton *third = strip.addButton();

    QCOMPARE(strip.buttons(), QList<KoGroupButton *>({first, second, third}));
    QCOMPARE(strip.button(0), first);
    QCOMPARE(strip.button(1), second);
    QCOMPARE(strip.button(2), third);
    QCOMPARE(strip.button(3), nullptr);

    QVERIFY(first->isCheckable());
    QVERIFY(!first->icon().isNull());
    QCOMPARE(first->text(), QStringLiteral("icon"));
    QCOMPARE(first->minimumSize(), QSize(28, 28));
    QCOMPARE(second->text(), QStringLiteral("text"));
    QVERIFY(second->icon().isNull());
    QVERIFY(third->text().isEmpty());
    QVERIFY(third->icon().isNull());

    QCOMPARE(first->groupPosition(), KoGroupButton::GroupLeft);
    QCOMPARE(second->groupPosition(), KoGroupButton::GroupCenter);
    QCOMPARE(third->groupPosition(), KoGroupButton::GroupRight);

    auto *layout = qobject_cast<QHBoxLayout *>(strip.layout());
    QCOMPARE(layout->itemAt(0)->widget(), first);
    QCOMPARE(layout->itemAt(1)->widget(), second);
    QCOMPARE(layout->itemAt(2)->widget(), third);
}

void KisOptionButtonStripContractTest::exclusiveSelectionForwardsPointerAndIndexSignals()
{
    qRegisterMetaType<KoGroupButton *>();

    KisOptionButtonStrip strip;
    KoGroupButton *first = strip.addButton(QStringLiteral("first"));
    KoGroupButton *second = strip.addButton(QStringLiteral("second"));
    KoGroupButton *third = strip.addButton(QStringLiteral("third"));
    QSignalSpy pointerSpy(&strip, QOverload<KoGroupButton *, bool>::of(&KisOptionButtonStrip::buttonToggled));
    QSignalSpy indexSpy(&strip, QOverload<int, bool>::of(&KisOptionButtonStrip::buttonToggled));

    QVERIFY(pointerSpy.isValid());
    QVERIFY(indexSpy.isValid());
    QVERIFY(strip.exclusive());

    first->setChecked(true);
    QCOMPARE(strip.checkedButton(), first);
    QCOMPARE(strip.checkedButtonIndex(), 0);
    QCOMPARE(pointerSpy.size(), 1);
    QCOMPARE(pointerSpy.at(0).at(0).value<KoGroupButton *>(), first);
    QCOMPARE(pointerSpy.at(0).at(1).toBool(), true);
    QCOMPARE(indexSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(indexSpy.at(0).at(1).toBool(), true);

    second->setChecked(true);
    QVERIFY(!first->isChecked());
    QVERIFY(second->isChecked());
    QCOMPARE(strip.checkedButton(), second);
    QCOMPARE(strip.checkedButtonIndex(), 1);
    QCOMPARE(pointerSpy.last().at(0).value<KoGroupButton *>(), second);
    QCOMPARE(pointerSpy.last().at(1).toBool(), true);
    QCOMPARE(indexSpy.last().at(0).toInt(), 1);
    QCOMPARE(indexSpy.last().at(1).toBool(), true);

    strip.setExclusive(false);
    QVERIFY(!strip.exclusive());
    third->setChecked(true);
    QVERIFY(second->isChecked());
    QVERIFY(third->isChecked());
    QCOMPARE(pointerSpy.last().at(0).value<KoGroupButton *>(), third);
    QCOMPARE(indexSpy.last().at(0).toInt(), 2);
}

QTEST_MAIN(KisOptionButtonStripContractTest)

#include "KisOptionButtonStripContractTest.moc"
