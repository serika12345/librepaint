/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisInputAction.h>
#include <kis_abstract_shortcut.h>

#include <QEvent>
#include <QTest>

class FakeInputAction final : public KisInputAction
{
public:
    ~FakeInputAction() override
    {
        if (destroyed) {
            *destroyed = true;
        }
    }

    void activate(int shortcut) override { activatedShortcut = shortcut; }
    void deactivate(int shortcut) override { deactivatedShortcut = shortcut; }
    void begin(int shortcut, QEvent *event) override
    {
        begunShortcut = shortcut;
        beginEvent = event;
    }
    void end(QEvent *event) override { endEvent = event; }
    void inputEvent(QEvent *event) override { receivedEvent = event; }

    bool supportsHiResInputEvents(int shortcut) const override
    {
        hiResRequestedShortcut = shortcut;
        return supportsHiRes;
    }
    KisInputActionGroup inputActionGroup(int shortcut) const override
    {
        requestedShortcut = shortcut;
        return group;
    }
    int priority() const override { return priorityValue; }
    bool canIgnoreModifiers() const override { return ignoresModifiers; }
    bool isAvailable() const override { return available; }

    KisInputActionGroup group {ModifyingActionGroup};
    bool available {true};
    bool supportsHiRes {false};
    bool ignoresModifiers {false};
    int priorityValue {17};
    mutable int requestedShortcut {-1};
    mutable int hiResRequestedShortcut {-1};
    int activatedShortcut {-1};
    int deactivatedShortcut {-1};
    int begunShortcut {-1};
    QEvent *beginEvent {nullptr};
    QEvent *endEvent {nullptr};
    QEvent *receivedEvent {nullptr};
    bool *destroyed {nullptr};
};

class TestShortcut final : public KisAbstractShortcut
{
public:
    TestShortcut(KisInputAction *action, int index, int shortcutPriority = 23)
        : KisAbstractShortcut(action, index)
        , m_priority(shortcutPriority)
    {
    }

    ~TestShortcut() override
    {
        if (destroyed) {
            *destroyed = true;
        }
    }

    int priority() const override { return m_priority; }

    bool hasSameKeys(const QSet<Qt::Key> &first, const QSet<Qt::Key> &second)
    {
        return compareKeys(first, second);
    }

    bool *destroyed {nullptr};

private:
    int m_priority;
};

class KisAbstractShortcutContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void inputActionDispatchesLifecycleEvents();
    void inputActionReportsMatcherProperties();
    void inputActionDestructionIsPolymorphic();
    void retainsBorrowedActionIndexAndPriority();
    void replacesBorrowedAction();
    void availabilityRequiresMatchingGroupAndAvailableAction();
    void destructionIsPolymorphic();
    void keyComparisonIgnoresOrder();
};

void KisAbstractShortcutContractTest::inputActionDispatchesLifecycleEvents()
{
    FakeInputAction action;
    KisInputAction *inputAction = &action;
    QEvent event(QEvent::User);

    inputAction->activate(2);
    inputAction->begin(3, &event);
    inputAction->inputEvent(&event);
    inputAction->end(&event);
    inputAction->deactivate(4);

    QCOMPARE(action.activatedShortcut, 2);
    QCOMPARE(action.begunShortcut, 3);
    QCOMPARE(action.beginEvent, &event);
    QCOMPARE(action.receivedEvent, &event);
    QCOMPARE(action.endEvent, &event);
    QCOMPARE(action.deactivatedShortcut, 4);
}

void KisAbstractShortcutContractTest::inputActionReportsMatcherProperties()
{
    FakeInputAction action;
    action.supportsHiRes = true;
    action.group = ToolInvoactionActionGroup;
    action.priorityValue = 29;
    action.ignoresModifiers = true;
    action.available = false;
    const KisInputAction *inputAction = &action;

    QVERIFY(inputAction->supportsHiResInputEvents(6));
    QCOMPARE(action.hiResRequestedShortcut, 6);
    QCOMPARE(inputAction->inputActionGroup(7), ToolInvoactionActionGroup);
    QCOMPARE(action.requestedShortcut, 7);
    QCOMPARE(inputAction->priority(), 29);
    QVERIFY(inputAction->canIgnoreModifiers());
    QVERIFY(!inputAction->isAvailable());
}

void KisAbstractShortcutContractTest::inputActionDestructionIsPolymorphic()
{
    bool destroyed = false;
    KisInputAction *inputAction = new FakeInputAction;
    static_cast<FakeInputAction *>(inputAction)->destroyed = &destroyed;

    delete inputAction;

    QVERIFY(destroyed);
}

void KisAbstractShortcutContractTest::retainsBorrowedActionIndexAndPriority()
{
    FakeInputAction action;
    TestShortcut shortcut(&action, 7, 31);

    QCOMPARE(shortcut.action(), &action);
    QCOMPARE(shortcut.shortcutIndex(), 7);
    QCOMPARE(shortcut.priority(), 31);
}

void KisAbstractShortcutContractTest::replacesBorrowedAction()
{
    FakeInputAction first;
    FakeInputAction second;
    TestShortcut shortcut(&first, 3);

    shortcut.setAction(&second);

    QCOMPARE(shortcut.action(), &second);
}

void KisAbstractShortcutContractTest::availabilityRequiresMatchingGroupAndAvailableAction()
{
    FakeInputAction action;
    TestShortcut shortcut(&action, 5);

    QVERIFY(shortcut.isAvailable(ModifyingActionGroup));
    QCOMPARE(action.requestedShortcut, 5);
    QVERIFY(shortcut.isAvailable(AllActionGroup));
    QVERIFY(!shortcut.isAvailable(ViewTransformActionGroup));

    action.available = false;
    QVERIFY(!shortcut.isAvailable(ModifyingActionGroup));
}

void KisAbstractShortcutContractTest::destructionIsPolymorphic()
{
    FakeInputAction action;
    bool destroyed = false;
    KisAbstractShortcut *shortcut = new TestShortcut(&action, 0);
    static_cast<TestShortcut *>(shortcut)->destroyed = &destroyed;

    delete shortcut;

    QVERIFY(destroyed);
}

void KisAbstractShortcutContractTest::keyComparisonIgnoresOrder()
{
    FakeInputAction action;
    TestShortcut shortcut(&action, 0);

    QVERIFY(shortcut.hasSameKeys({Qt::Key_Control, Qt::Key_A},
                                 {Qt::Key_A, Qt::Key_Control}));
    QVERIFY(!shortcut.hasSameKeys({Qt::Key_Control, Qt::Key_A},
                                  {Qt::Key_Control, Qt::Key_B}));
    QVERIFY(!shortcut.hasSameKeys({Qt::Key_Control},
                                  {Qt::Key_Control, Qt::Key_A}));
}

QTEST_GUILESS_MAIN(KisAbstractShortcutContractTest)

#include "KisAbstractShortcutContractTest.moc"
