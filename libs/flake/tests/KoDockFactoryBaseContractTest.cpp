/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoDockFactoryBase.h"

#include <QStringList>
#include <QTest>

class QDockWidget
{
};

namespace
{
class DockFactoryProbe : public KoDockFactoryBase
{
public:
    DockFactoryProbe(const QString &factoryId,
                     DockPosition position,
                     QDockWidget *widget,
                     int *destructionCount = nullptr)
        : m_factoryId(factoryId)
        , m_position(position)
        , m_widget(widget)
        , m_destructionCount(destructionCount)
    {
    }

    ~DockFactoryProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    QString id() const override
    {
        events.append(QStringLiteral("id"));
        return m_factoryId;
    }

    DockPosition defaultDockPosition() const override
    {
        events.append(QStringLiteral("defaultDockPosition"));
        return m_position;
    }

    QDockWidget *createDockWidget() override
    {
        events.append(QStringLiteral("createDockWidget"));
        return m_widget;
    }

    mutable QStringList events;

private:
    QString m_factoryId;
    DockPosition m_position;
    QDockWidget *m_widget;
    int *m_destructionCount;
};
} // namespace

class KoDockFactoryBaseContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void keepsDockPositionValuesStable();
    void dispatchesFactoryResultsThroughBase();
    void destroysDerivedFactoryThroughBase();
};

void KoDockFactoryBaseContractTest::keepsDockPositionValuesStable()
{
    QCOMPARE(static_cast<int>(KoDockFactoryBase::DockTornOff), 0);
    QCOMPARE(static_cast<int>(KoDockFactoryBase::DockTop), 1);
    QCOMPARE(static_cast<int>(KoDockFactoryBase::DockBottom), 2);
    QCOMPARE(static_cast<int>(KoDockFactoryBase::DockRight), 3);
    QCOMPARE(static_cast<int>(KoDockFactoryBase::DockLeft), 4);
    QCOMPARE(static_cast<int>(KoDockFactoryBase::DockMinimized), 5);
}

void KoDockFactoryBaseContractTest::dispatchesFactoryResultsThroughBase()
{
    QDockWidget widget;
    DockFactoryProbe factory(QString::fromUtf8("補助ドック"), KoDockFactoryBase::DockLeft, &widget);
    KoDockFactoryBase *interface = &factory;

    QCOMPARE(interface->id(), QString::fromUtf8("補助ドック"));
    QCOMPARE(interface->defaultDockPosition(), KoDockFactoryBase::DockLeft);
    QCOMPARE(interface->createDockWidget(), &widget);

    const QStringList expectedEvents = {
        QStringLiteral("id"),
        QStringLiteral("defaultDockPosition"),
        QStringLiteral("createDockWidget"),
    };
    QCOMPARE(factory.events, expectedEvents);
}

void KoDockFactoryBaseContractTest::destroysDerivedFactoryThroughBase()
{
    int destructionCount = 0;
    QDockWidget widget;
    KoDockFactoryBase *factory =
        new DockFactoryProbe(QStringLiteral("lifetime"), KoDockFactoryBase::DockTornOff, &widget, &destructionCount);

    delete factory;

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KoDockFactoryBaseContractTest)

#include "KoDockFactoryBaseContractTest.moc"
