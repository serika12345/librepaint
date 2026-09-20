/*
 *  SPDX-FileCopyrightText: 2019 Tusooa Zhu <tusooa@vista.aero>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisSignalAutoConnectionTest.h"

#include <kis_signal_auto_connection.h>

#include <QScopedPointer>
#include <QTest>

void KisSignalAutoConnectionTest::testDirectConnectionLifetime()
{
    QScopedPointer<TestClass> sender(new TestClass());
    QScopedPointer<TestClass> receiver(new TestClass());

    {
        KisSignalAutoConnectionSP connection(
            new KisSignalAutoConnection(sender.data(), &TestClass::sigTest1, receiver.data(), &TestClass::slotTest1));
        QVERIFY(!connection.isNull());
        Q_EMIT sender->sigTest1();
        QCOMPARE(receiver->m_test1CallCount, 1);
    }

    Q_EMIT sender->sigTest1();
    QCOMPARE(receiver->m_test1CallCount, 1);
}

void KisSignalAutoConnectionTest::testMacroConnection()
{
    QScopedPointer<TestClass> test1(new TestClass());
    QScopedPointer<TestClass> test2(new TestClass());
    KisSignalAutoConnectionsStore conn;
    conn.addConnection(test1.data(), SIGNAL(sigTest1()), test2.data(), SLOT(slotTest1()));
    Q_EMIT test1->sigTest1();
    QVERIFY(test2->m_test1Called);
    test2->m_test1Called = false;
    conn.clear();
    Q_EMIT test1->sigTest1();
    QVERIFY(test2->m_test1Called == false);
}

void KisSignalAutoConnectionTest::testMemberFunctionConnection()
{
    QScopedPointer<TestClass> test1(new TestClass());
    QScopedPointer<TestClass> test2(new TestClass());
    KisSignalAutoConnectionsStore conn;
    QVERIFY(conn.isEmpty());
    conn.addConnection(test1.data(), &TestClass::sigTest1, test2.data(), &TestClass::slotTest1);
    QVERIFY(!conn.isEmpty());
    Q_EMIT test1->sigTest1();
    QVERIFY(test2->m_test1Called);
    test2->m_test1Called = false;
    conn.clear();
    QVERIFY(conn.isEmpty());
    Q_EMIT test1->sigTest1();
    QVERIFY(test2->m_test1Called == false);
}

void KisSignalAutoConnectionTest::testUniqueConnection()
{
    QScopedPointer<TestClass> sender(new TestClass());
    QScopedPointer<TestClass> receiver(new TestClass());
    KisSignalAutoConnectionsStore connections;

    connections.addUniqueConnection(sender.data(), &TestClass::sigTest1, receiver.data(), &TestClass::slotTest1);
    connections.addUniqueConnection(sender.data(), &TestClass::sigTest1, receiver.data(), &TestClass::slotTest1);
    Q_EMIT sender->sigTest1();

    QCOMPARE(receiver->m_test1CallCount, 1);
}

void KisSignalAutoConnectionTest::testOverloadConnection()
{
    QScopedPointer<TestClass> test1(new TestClass());
    QScopedPointer<TestClass> test2(new TestClass());
    KisSignalAutoConnectionsStore conn;
    conn.addConnection(test1.data(),
                       QOverload<const QString &, const QString &>::of(&TestClass::sigTest2),
                       test2.data(),
                       QOverload<const QString &, const QString &>::of(&TestClass::slotTest2));
    conn.addConnection(test1.data(), SIGNAL(sigTest2(int)), test2.data(), SLOT(slotTest2(int)));
    Q_EMIT test1->sigTest2("foo", "bar");
    QVERIFY(test2->m_str1 == "foo");
    QVERIFY(test2->m_str2 == "bar");
    Q_EMIT test1->sigTest2(5);
    QVERIFY(test2->m_number == 5);
    conn.clear();
    Q_EMIT test1->sigTest2("1", "2");
    QVERIFY(test2->m_str1 == "foo");
    QVERIFY(test2->m_str2 == "bar");
    conn.addConnection(test1.data(),
                       SIGNAL(sigTest2(const QString &, const QString &)),
                       test2.data(),
                       SLOT(slotTest2(const QString &)));
    Q_EMIT test1->sigTest2("3", "4");
    QVERIFY(test2->m_str1 == "3");
    QVERIFY(test2->m_str2 == "");
}

void KisSignalAutoConnectionTest::testSignalToSignalConnection()
{
    QScopedPointer<TestClass> test1(new TestClass());
    QScopedPointer<TestClass> test2(new TestClass());
    KisSignalAutoConnectionsStore conn;
    conn.addConnection(test1.data(),
                       QOverload<int>::of(&TestClass::sigTest2),
                       test2.data(),
                       QOverload<int>::of(&TestClass::sigTest2));
    conn.addConnection(test2.data(), SIGNAL(sigTest2(int)), test2.data(), SLOT(slotTest2(int)));
    Q_EMIT test1->sigTest2(10);
    QVERIFY(test2->m_number == 10);
    conn.clear();
    conn.addConnection(test1.data(), SIGNAL(sigTest2(int)), test2.data(), SIGNAL(sigTest2(int)));
    conn.addConnection(test2.data(),
                       QOverload<int>::of(&TestClass::sigTest2),
                       test2.data(),
                       QOverload<int>::of(&TestClass::slotTest2));
    Q_EMIT test1->sigTest2(50);
    QVERIFY(test2->m_number == 50);
}

void KisSignalAutoConnectionTest::testDestroyedObject()
{
    QScopedPointer<TestClass> test1(new TestClass());
    QScopedPointer<TestClass> test2(new TestClass());
    KisSignalAutoConnectionsStore conn;
    conn.addConnection(test1.data(),
                       QOverload<int>::of(&TestClass::sigTest2),
                       test2.data(),
                       QOverload<int>::of(&TestClass::slotTest2));
    Q_EMIT test1->sigTest2(10);
    QVERIFY(test2->m_number == 10);
    test2.reset(0);
    conn.clear();
}

TestClass::TestClass(QObject *parent)
    : QObject(parent)
    , m_test1Called(false)
    , m_test1CallCount(0)
    , m_str1()
    , m_str2()
    , m_number(0)
{
}

TestClass::~TestClass()
{
}

void TestClass::slotTest1()
{
    m_test1Called = true;
    ++m_test1CallCount;
}

void TestClass::slotTest2(const QString &arg1, const QString &arg2)
{
    m_str1 = arg1;
    m_str2 = arg2;
}

void TestClass::slotTest2(const QString &arg)
{
    m_str1 = arg;
    m_str2 = QString();
}

void TestClass::slotTest2(int arg)
{
    m_number = arg;
}

QTEST_GUILESS_MAIN(KisSignalAutoConnectionTest)
