/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisInputActionGroup.h>

#include <QTest>

class InputActionGroupsMaskState final : public KisInputActionGroupsMaskInterface
{
public:
    explicit InputActionGroupsMaskState(bool *destroyed = nullptr)
        : m_destroyed(destroyed)
    {
    }

    ~InputActionGroupsMaskState() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    KisInputActionGroupsMask inputActionGroupsMask() const override
    {
        return m_mask;
    }

    void setInputActionGroupsMask(KisInputActionGroupsMask mask) override
    {
        m_mask = mask;
    }

private:
    KisInputActionGroupsMask m_mask {AllActionGroup};
    bool *m_destroyed;
};

class KisInputActionGroupContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void groupBitsRemainStable();
    void guardRestoresPreviousMask();
    void interfaceDestructionIsPolymorphic();
};

void KisInputActionGroupContractTest::groupBitsRemainStable()
{
    QCOMPARE(int(NoActionGroup), 0x0);
    QCOMPARE(int(ViewTransformActionGroup), 0x1);
    QCOMPARE(int(ModifyingActionGroup), 0x2);
    QCOMPARE(int(ToolInvoactionActionGroup), 0x4);
    QCOMPARE(int(AllActionGroup), 0x7);

    const KisInputActionGroupsMask editing =
        ModifyingActionGroup | ToolInvoactionActionGroup;
    QVERIFY(editing.testFlag(ModifyingActionGroup));
    QVERIFY(editing.testFlag(ToolInvoactionActionGroup));
    QVERIFY(!editing.testFlag(ViewTransformActionGroup));
}

void KisInputActionGroupContractTest::guardRestoresPreviousMask()
{
    const KisInputActionGroupsMaskInterface::SharedInterface state =
        QSharedPointer<InputActionGroupsMaskState>::create();

    QCOMPARE(state->inputActionGroupsMask(), KisInputActionGroupsMask(AllActionGroup));
    {
        KisInputActionGroupsMaskGuard outerGuard(state, ViewTransformActionGroup);
        QCOMPARE(state->inputActionGroupsMask(),
                 KisInputActionGroupsMask(ViewTransformActionGroup));
        {
            KisInputActionGroupsMaskGuard innerGuard(state, ModifyingActionGroup);
            QCOMPARE(state->inputActionGroupsMask(),
                     KisInputActionGroupsMask(ModifyingActionGroup));
        }
        QCOMPARE(state->inputActionGroupsMask(),
                 KisInputActionGroupsMask(ViewTransformActionGroup));
    }
    QCOMPARE(state->inputActionGroupsMask(), KisInputActionGroupsMask(AllActionGroup));
}

void KisInputActionGroupContractTest::interfaceDestructionIsPolymorphic()
{
    bool destroyed = false;
    {
        const KisInputActionGroupsMaskInterface::SharedInterface state =
            QSharedPointer<InputActionGroupsMaskState>::create(&destroyed);
        QVERIFY(!destroyed);
    }

    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(KisInputActionGroupContractTest)

#include "KisInputActionGroupContractTest.moc"
