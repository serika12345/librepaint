/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoAbstractCanvasResourceInterface.h"

#include <QSignalSpy>
#include <QTest>
#include <QVariant>

namespace
{
class ResourceProbe : public KoAbstractCanvasResourceInterface
{
public:
    ResourceProbe(int key, const QVariant &initialValue, int *destructionCount = nullptr)
        : KoAbstractCanvasResourceInterface(key)
        , m_value(initialValue)
        , m_destructionCount(destructionCount)
    {
    }

    ResourceProbe(int key, const QVariant &initialValue, const QString &debugTag, int *destructionCount = nullptr)
        : KoAbstractCanvasResourceInterface(key, debugTag)
        , m_value(initialValue)
        , m_destructionCount(destructionCount)
    {
    }

    ~ResourceProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    QVariant value() const override
    {
        ++valueCallCount;
        return m_value;
    }

    void setValue(const QVariant value) override
    {
        ++setValueCallCount;
        m_value = value;
    }

    void emitExternalChange(const QVariant &value)
    {
        Q_EMIT sigResourceChangedExternal(key(), value);
    }

    mutable int valueCallCount = 0;
    int setValueCallCount = 0;

private:
    QVariant m_value;
    int *m_destructionCount;
};
} // namespace

class KoAbstractCanvasResourceInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void storesKeyAndDispatchesValueAccess();
    void deliversExternalChangeSignalArgumentsInOrder();
    void sharedAliasOwnsThroughBaseLifetime();
};

void KoAbstractCanvasResourceInterfaceContractTest::storesKeyAndDispatchesValueAccess()
{
    ResourceProbe resource(73, 11, QString::fromUtf8("外部資源"));
    KoAbstractCanvasResourceInterface *interface = &resource;

    QCOMPARE(interface->key(), 73);
    QCOMPARE(interface->value(), QVariant(11));
    QCOMPARE(resource.valueCallCount, 1);

    interface->setValue(QStringLiteral("updated"));
    QCOMPARE(resource.setValueCallCount, 1);
    QCOMPARE(interface->value(), QVariant(QStringLiteral("updated")));
    QCOMPARE(resource.valueCallCount, 2);
}

void KoAbstractCanvasResourceInterfaceContractTest::deliversExternalChangeSignalArgumentsInOrder()
{
    ResourceProbe resource(-19, QVariant());
    QSignalSpy spy(&resource, &KoAbstractCanvasResourceInterface::sigResourceChangedExternal);

    resource.emitExternalChange(QStringLiteral("first"));
    resource.emitExternalChange(42);

    QCOMPARE(spy.size(), 2);
    QCOMPARE(spy.at(0).at(0), QVariant(-19));
    QCOMPARE(spy.at(0).at(1), QVariant(QStringLiteral("first")));
    QCOMPARE(spy.at(1).at(0), QVariant(-19));
    QCOMPARE(spy.at(1).at(1), QVariant(42));
}

void KoAbstractCanvasResourceInterfaceContractTest::sharedAliasOwnsThroughBaseLifetime()
{
    int destructionCount = 0;
    KoAbstractCanvasResourceInterfaceSP owner(
        new ResourceProbe(5, QStringLiteral("value"), QStringLiteral("shared"), &destructionCount));
    KoAbstractCanvasResourceInterfaceSP copy = owner;

    QVERIFY(!owner->parent());
    QCOMPARE(copy->key(), 5);
    owner.clear();
    QCOMPARE(destructionCount, 0);
    QCOMPARE(copy->value(), QVariant(QStringLiteral("value")));

    copy.clear();
    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KoAbstractCanvasResourceInterfaceContractTest)

#include "KoAbstractCanvasResourceInterfaceContractTest.moc"
