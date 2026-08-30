/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoShapeConfigWidgetBase.h"

#include <QPointer>
#include <QSignalSpy>
#include <QStringList>
#include <QTest>

class KoCanvasResourceProvider
{
};

class KoShape
{
};

class KoUnit
{
};

class KUndo2Command
{
};

namespace
{
class ShapeConfigWidgetProbe : public KoShapeConfigWidgetBase
{
public:
    explicit ShapeConfigWidgetProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~ShapeConfigWidgetProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void open(KoShape *shape) override
    {
        events.append(QStringLiteral("open"));
        openedShape = shape;
    }

    void save() override
    {
        events.append(QStringLiteral("save"));
    }

    void setUnit(const KoUnit &unit) override
    {
        events.append(QStringLiteral("setUnit"));
        openedUnit = &unit;
        ++unitCallCount;
    }

    bool showOnShapeCreate() override
    {
        events.append(QStringLiteral("showOnShapeCreate"));
        return true;
    }

    bool showOnShapeSelect() override
    {
        events.append(QStringLiteral("showOnShapeSelect"));
        return false;
    }

    KUndo2Command *createCommand() override
    {
        events.append(QStringLiteral("createCommand"));
        return commandResult;
    }

    KoCanvasResourceProvider *resourceManager() const
    {
        return m_resourceManager;
    }

    void invokeBaseSetUnit(const KoUnit &unit)
    {
        KoShapeConfigWidgetBase::setUnit(unit);
    }

    bool baseShowOnShapeCreate()
    {
        return KoShapeConfigWidgetBase::showOnShapeCreate();
    }

    bool baseShowOnShapeSelect()
    {
        return KoShapeConfigWidgetBase::showOnShapeSelect();
    }

    KUndo2Command *baseCreateCommand()
    {
        return KoShapeConfigWidgetBase::createCommand();
    }

    void emitPropertyChanged()
    {
        Q_EMIT propertyChanged();
    }

    void emitAccept()
    {
        Q_EMIT accept();
    }

    QStringList events;
    KoShape *openedShape = nullptr;
    const KoUnit *openedUnit = nullptr;
    KUndo2Command *commandResult = nullptr;
    int unitCallCount = 0;

private:
    int *m_destructionCount;
};
} // namespace

class KoShapeConfigWidgetBaseContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void keepsDefaultsAndBorrowedResourceManager();
    void dispatchesConfigurationOperationsThroughBase();
    void deliversPropertyAndAcceptSignalsInOrder();
    void destroysDerivedWidgetThroughQWidgetBase();
};

void KoShapeConfigWidgetBaseContractTest::keepsDefaultsAndBorrowedResourceManager()
{
    ShapeConfigWidgetProbe widget;
    KoUnit unit;

    QVERIFY(!widget.parent());
    QCOMPARE(widget.resourceManager(), nullptr);
    QVERIFY(!widget.baseShowOnShapeCreate());
    QVERIFY(widget.baseShowOnShapeSelect());
    QCOMPARE(widget.baseCreateCommand(), nullptr);

    widget.invokeBaseSetUnit(unit);
    QCOMPARE(widget.unitCallCount, 0);
    QCOMPARE(widget.openedUnit, nullptr);

    KoCanvasResourceProvider resourceManager;
    widget.setResourceManager(&resourceManager);
    QCOMPARE(widget.resourceManager(), &resourceManager);

    widget.setResourceManager(nullptr);
    QCOMPARE(widget.resourceManager(), nullptr);
}

void KoShapeConfigWidgetBaseContractTest::dispatchesConfigurationOperationsThroughBase()
{
    ShapeConfigWidgetProbe widget;
    KoShape shape;
    KoUnit unit;
    KUndo2Command command;
    widget.commandResult = &command;
    KoShapeConfigWidgetBase *interface = &widget;

    interface->open(&shape);
    interface->setUnit(unit);
    interface->save();
    QVERIFY(interface->showOnShapeCreate());
    QVERIFY(!interface->showOnShapeSelect());
    QCOMPARE(interface->createCommand(), &command);

    QCOMPARE(widget.openedShape, &shape);
    QCOMPARE(widget.openedUnit, &unit);
    QCOMPARE(widget.unitCallCount, 1);
    const QStringList expectedEvents = {
        QStringLiteral("open"),
        QStringLiteral("setUnit"),
        QStringLiteral("save"),
        QStringLiteral("showOnShapeCreate"),
        QStringLiteral("showOnShapeSelect"),
        QStringLiteral("createCommand"),
    };
    QCOMPARE(widget.events, expectedEvents);
}

void KoShapeConfigWidgetBaseContractTest::deliversPropertyAndAcceptSignalsInOrder()
{
    ShapeConfigWidgetProbe widget;
    QSignalSpy propertySpy(&widget, &KoShapeConfigWidgetBase::propertyChanged);
    QSignalSpy acceptSpy(&widget, &KoShapeConfigWidgetBase::accept);
    QStringList deliveryOrder;
    connect(&widget, &KoShapeConfigWidgetBase::propertyChanged, this, [&deliveryOrder]() {
        deliveryOrder.append(QStringLiteral("propertyChanged"));
    });
    connect(&widget, &KoShapeConfigWidgetBase::accept, this, [&deliveryOrder]() {
        deliveryOrder.append(QStringLiteral("accept"));
    });

    widget.emitPropertyChanged();
    widget.emitAccept();
    widget.emitPropertyChanged();

    QCOMPARE(propertySpy.size(), 2);
    QCOMPARE(acceptSpy.size(), 1);
    const QStringList expectedOrder = {
        QStringLiteral("propertyChanged"),
        QStringLiteral("accept"),
        QStringLiteral("propertyChanged"),
    };
    QCOMPARE(deliveryOrder, expectedOrder);
}

void KoShapeConfigWidgetBaseContractTest::destroysDerivedWidgetThroughQWidgetBase()
{
    int destructionCount = 0;
    QWidget *widget = new ShapeConfigWidgetProbe(&destructionCount);
    QPointer<QWidget> guard(widget);

    delete widget;

    QCOMPARE(destructionCount, 1);
    QVERIFY(guard.isNull());
}

QTEST_MAIN(KoShapeConfigWidgetBaseContractTest)

#include "KoShapeConfigWidgetBaseContractTest.moc"
