/*
 * SPDX-FileCopyrightText: 2026 LibrePaint Contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisDirtyStateSaverTest.h"

#include "KisDirtyStateSaver.h"

#include <QTest>

namespace
{
class TestResource
{
public:
    explicit TestResource(bool dirty)
        : m_dirty(dirty)
    {
    }

    bool isDirty() const
    {
        return m_dirty;
    }

    void setDirty(bool dirty)
    {
        m_dirty = dirty;
    }

private:
    bool m_dirty;
};
} // namespace

void KisDirtyStateSaverTest::restoresInitialDirtyState_data()
{
    QTest::addColumn<bool>("initialState");

    QTest::newRow("initially-clean") << false;
    QTest::newRow("initially-dirty") << true;
}

void KisDirtyStateSaverTest::restoresInitialDirtyState()
{
    QFETCH(bool, initialState);

    TestResource resource(initialState);
    {
        KisDirtyStateSaver<TestResource *> saver(&resource);
        resource.setDirty(!initialState);
        QCOMPARE(resource.isDirty(), !initialState);
    }

    QCOMPARE(resource.isDirty(), initialState);
}

QTEST_GUILESS_MAIN(KisDirtyStateSaverTest)
