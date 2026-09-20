/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_composite_progress_proxy.h"

#include <QStringList>
#include <QTest>

namespace
{

class RecordingProgressProxy : public KoProgressProxy
{
public:
    RecordingProgressProxy(const QString &name, int maximum, QStringList *events, bool *destroyed = nullptr)
        : m_name(name)
        , m_maximum(maximum)
        , m_events(events)
        , m_destroyed(destroyed)
    {
    }

    ~RecordingProgressProxy() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    int maximum() const override
    {
        return m_maximum;
    }

    void setValue(int value) override
    {
        m_events->append(QStringLiteral("%1:value:%2").arg(m_name).arg(value));
    }

    void setRange(int minimum, int maximum) override
    {
        m_maximum = maximum;
        m_events->append(QStringLiteral("%1:range:%2:%3").arg(m_name).arg(minimum).arg(maximum));
    }

    void setFormat(const QString &format) override
    {
        m_events->append(QStringLiteral("%1:format:%2").arg(m_name, format));
    }

    void setAutoNestedName(const QString &name) override
    {
        m_events->append(QStringLiteral("%1:name:%2").arg(m_name, name));
    }

private:
    const QString m_name;
    int m_maximum;
    QStringList *m_events;
    bool *m_destroyed;
};

} // namespace

class KisCompositeProgressProxyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyStateAndBorrowedLifetime();
    void distinctProxiesReceiveEachUpdateOnceInRegistrationOrder();
    void removeBalancesDuplicateRegistrationsAndChangesMaximumSource();
};

void KisCompositeProgressProxyContractTest::emptyStateAndBorrowedLifetime()
{
    QStringList events;
    bool destroyed = false;
    RecordingProgressProxy proxy(QStringLiteral("borrowed"), 100, &events, &destroyed);

    {
        KisCompositeProgressProxy composite;
        KoProgressProxy *base = &composite;

        QCOMPARE(base->maximum(), 0);
        composite.addProxy(&proxy);
        QCOMPARE(base->maximum(), 100);
    }

    QVERIFY(!destroyed);
    proxy.setValue(7);
    QCOMPARE(events, QStringList({QStringLiteral("borrowed:value:7")}));
}

void KisCompositeProgressProxyContractTest::distinctProxiesReceiveEachUpdateOnceInRegistrationOrder()
{
    QStringList events;
    RecordingProgressProxy first(QStringLiteral("first"), 100, &events);
    RecordingProgressProxy second(QStringLiteral("second"), 100, &events);
    KisCompositeProgressProxy composite;

    composite.addProxy(&first);
    composite.addProxy(&second);
    composite.addProxy(&first);

    composite.setValue(37);
    composite.setRange(4, 96);
    composite.setFormat(QStringLiteral("frame %v of %m"));
    composite.setAutoNestedName(QStringLiteral("Export"));

    const QStringList expected{
        QStringLiteral("first:value:37"),
        QStringLiteral("second:value:37"),
        QStringLiteral("first:range:4:96"),
        QStringLiteral("second:range:4:96"),
        QStringLiteral("first:format:frame %v of %m"),
        QStringLiteral("second:format:frame %v of %m"),
        QStringLiteral("first:name:Export"),
        QStringLiteral("second:name:Export"),
    };
    QCOMPARE(events, expected);
}

void KisCompositeProgressProxyContractTest::removeBalancesDuplicateRegistrationsAndChangesMaximumSource()
{
    QStringList events;
    RecordingProgressProxy first(QStringLiteral("first"), 100, &events);
    RecordingProgressProxy second(QStringLiteral("second"), 250, &events);
    RecordingProgressProxy unregistered(QStringLiteral("unregistered"), 400, &events);
    KisCompositeProgressProxy composite;

    composite.addProxy(&first);
    composite.addProxy(&first);
    composite.addProxy(&second);
    QCOMPARE(composite.maximum(), 100);

    composite.removeProxy(&first);
    QCOMPARE(composite.maximum(), 100);
    composite.setValue(1);
    QCOMPARE(events, QStringList({QStringLiteral("first:value:1"), QStringLiteral("second:value:1")}));

    events.clear();
    composite.removeProxy(&first);
    QCOMPARE(composite.maximum(), 250);
    composite.setValue(2);
    QCOMPARE(events, QStringList({QStringLiteral("second:value:2")}));

    events.clear();
    composite.removeProxy(&unregistered);
    QCOMPARE(composite.maximum(), 250);
    composite.setValue(3);
    QCOMPARE(events, QStringList({QStringLiteral("second:value:3")}));

    events.clear();
    composite.removeProxy(&second);
    QCOMPARE(composite.maximum(), 0);
    composite.setValue(4);
    QVERIFY(events.isEmpty());
}

QTEST_GUILESS_MAIN(KisCompositeProgressProxyContractTest)

#include "KisCompositeProgressProxyContractTest.moc"
