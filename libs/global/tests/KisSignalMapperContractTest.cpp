/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisSignalMapper.h"

#include <QPointer>
#include <QTest>

#include <memory>

class SignalTrigger : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void triggered();
};

class KisSignalMapperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void explicitMapEmitsEveryMappingAndSupportsReverseLookup();
    void senderDrivenMapUsesTheEmittingObject();
    void removalAndSenderDestructionClearMappings();
    void parentOwnsMapperLifetime();
};

void KisSignalMapperContractTest::explicitMapEmitsEveryMappingAndSupportsReverseLookup()
{
    KisSignalMapper mapper;
    QObject sender;
    QObject objectValue;
    QObject widgetStorage;
    QWidget *widgetValue = reinterpret_cast<QWidget *>(&widgetStorage);

    int mappedInt = 0;
    QString mappedText;
    QWidget *mappedWidget = nullptr;
    QObject *mappedObject = nullptr;
    connect(&mapper,
            static_cast<void (KisSignalMapper::*)(int)>(&KisSignalMapper::mapped),
            this,
            [&mappedInt](int value) {
                mappedInt = value;
            });
    connect(&mapper,
            static_cast<void (KisSignalMapper::*)(const QString &)>(&KisSignalMapper::mapped),
            this,
            [&mappedText](const QString &value) {
                mappedText = value;
            });
    connect(&mapper,
            static_cast<void (KisSignalMapper::*)(QWidget *)>(&KisSignalMapper::mapped),
            this,
            [&mappedWidget](QWidget *value) {
                mappedWidget = value;
            });
    connect(&mapper,
            static_cast<void (KisSignalMapper::*)(QObject *)>(&KisSignalMapper::mapped),
            this,
            [&mappedObject](QObject *value) {
                mappedObject = value;
            });

    mapper.setMapping(&sender, 41);
    mapper.setMapping(&sender, QStringLiteral("mapped-text"));
    mapper.setMapping(&sender, widgetValue);
    mapper.setMapping(&sender, &objectValue);

    QCOMPARE(mapper.mapping(41), &sender);
    QCOMPARE(mapper.mapping(QStringLiteral("mapped-text")), &sender);
    QCOMPARE(mapper.mapping(widgetValue), &sender);
    QCOMPARE(mapper.mapping(&objectValue), &sender);

    mapper.map(&sender);
    QCOMPARE(mappedInt, 41);
    QCOMPARE(mappedText, QStringLiteral("mapped-text"));
    QCOMPARE(mappedWidget, widgetValue);
    QCOMPARE(mappedObject, &objectValue);
}

void KisSignalMapperContractTest::senderDrivenMapUsesTheEmittingObject()
{
    KisSignalMapper mapper;
    SignalTrigger sender;
    int mappedInt = 0;
    connect(&mapper,
            static_cast<void (KisSignalMapper::*)(int)>(&KisSignalMapper::mapped),
            this,
            [&mappedInt](int value) {
                mappedInt = value;
            });
    connect(&sender,
            &SignalTrigger::triggered,
            &mapper,
            static_cast<void (KisSignalMapper::*)()>(&KisSignalMapper::map));
    mapper.setMapping(&sender, 73);

    Q_EMIT sender.triggered();

    QCOMPARE(mappedInt, 73);
}

void KisSignalMapperContractTest::removalAndSenderDestructionClearMappings()
{
    KisSignalMapper mapper;
    QObject sender;
    int mappedCount = 0;
    connect(&mapper,
            static_cast<void (KisSignalMapper::*)(int)>(&KisSignalMapper::mapped),
            this,
            [&mappedCount](int) {
                mappedCount++;
            });

    mapper.setMapping(&sender, 17);
    mapper.removeMappings(&sender);
    QCOMPARE(mapper.mapping(17), nullptr);
    mapper.map(&sender);
    QCOMPARE(mappedCount, 0);

    auto transientSender = std::make_unique<QObject>();
    mapper.setMapping(transientSender.get(), 29);
    QCOMPARE(mapper.mapping(29), transientSender.get());
    transientSender.reset();
    QCOMPARE(mapper.mapping(29), nullptr);
}

void KisSignalMapperContractTest::parentOwnsMapperLifetime()
{
    auto owner = std::make_unique<QObject>();
    QPointer<KisSignalMapper> mapper = new KisSignalMapper(owner.get());
    QCOMPARE(mapper->parent(), owner.get());

    owner.reset();

    QVERIFY(mapper.isNull());
}

QTEST_GUILESS_MAIN(KisSignalMapperContractTest)

#include "KisSignalMapperContractTest.moc"
