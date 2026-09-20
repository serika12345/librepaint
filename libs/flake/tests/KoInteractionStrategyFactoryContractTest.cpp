/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoInteractionStrategyFactory.h"

#include <QPainter>
#include <QStringList>
#include <QTest>

class KoInteractionStrategy
{
};

class KoPointerEvent
{
};

class KoViewConverter
{
};

class KoColorDisplayRendererInterface
{
};

namespace
{

class FactoryProbe final : public KoInteractionStrategyFactory
{
public:
    FactoryProbe(int priority,
                 const QString &id,
                 KoInteractionStrategy *strategyResult = nullptr,
                 int *destructionCount = nullptr)
        : KoInteractionStrategyFactory(priority, id)
        , m_strategyResult(strategyResult)
        , m_destructionCount(destructionCount)
    {
    }

    ~FactoryProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    KoInteractionStrategy *createStrategy(KoPointerEvent *event) override
    {
        calls << QStringLiteral("createStrategy");
        createEvent = event;
        return m_strategyResult;
    }

    bool hoverEvent(KoPointerEvent *event) override
    {
        calls << QStringLiteral("hoverEvent");
        hoverEventArgument = event;
        return true;
    }

    bool paintOnHover(QPainter &painter,
                      const KoViewConverter &converter,
                      const KoColorDisplayRendererInterface *displayRendererInterface) override
    {
        calls << QStringLiteral("paintOnHover");
        painterArgument = &painter;
        converterArgument = &converter;
        displayRendererArgument = displayRendererInterface;
        return false;
    }

    bool tryUseCustomCursor() override
    {
        calls << QStringLiteral("tryUseCustomCursor");
        return true;
    }

    QStringList calls;
    KoPointerEvent *createEvent = nullptr;
    KoPointerEvent *hoverEventArgument = nullptr;
    QPainter *painterArgument = nullptr;
    const KoViewConverter *converterArgument = nullptr;
    const KoColorDisplayRendererInterface *displayRendererArgument = nullptr;

private:
    KoInteractionStrategy *m_strategyResult = nullptr;
    int *m_destructionCount = nullptr;
};

} // namespace

class KoInteractionStrategyFactoryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void storesIdentityPriorityAndOrdersSharedFactories();
    void dispatchesInteractionArgumentsAndResultsThroughBase();
    void sharedAliasAndBaseOwnershipDestroyDerivedFactory();
};

void KoInteractionStrategyFactoryContractTest::storesIdentityPriorityAndOrdersSharedFactories()
{
    const KoInteractionStrategyFactorySP lower(new FactoryProbe(-11, QStringLiteral("ペンα")));
    const KoInteractionStrategyFactorySP higher(new FactoryProbe(27, QStringLiteral("選択🎨")));
    const KoInteractionStrategyFactorySP equalPriority(new FactoryProbe(27, QStringLiteral("同順位")));

    QCOMPARE(lower->id(), QStringLiteral("ペンα"));
    QCOMPARE(lower->priority(), -11);
    QCOMPARE(higher->id(), QStringLiteral("選択🎨"));
    QCOMPARE(higher->priority(), 27);

    QVERIFY(KoInteractionStrategyFactory::compareLess(lower, higher));
    QVERIFY(!KoInteractionStrategyFactory::compareLess(higher, lower));
    QVERIFY(!KoInteractionStrategyFactory::compareLess(higher, equalPriority));
    QVERIFY(!KoInteractionStrategyFactory::compareLess(equalPriority, higher));
}

void KoInteractionStrategyFactoryContractTest::dispatchesInteractionArgumentsAndResultsThroughBase()
{
    KoInteractionStrategy strategy;
    KoPointerEvent event;
    QPainter painter;
    KoViewConverter converter;
    KoColorDisplayRendererInterface displayRenderer;
    FactoryProbe probe(3, QStringLiteral("dispatch"), &strategy);
    KoInteractionStrategyFactory *base = &probe;

    QCOMPARE(base->createStrategy(&event), &strategy);
    QVERIFY(base->hoverEvent(&event));
    QVERIFY(!base->paintOnHover(painter, converter, &displayRenderer));
    QVERIFY(base->tryUseCustomCursor());

    QCOMPARE(probe.createEvent, &event);
    QCOMPARE(probe.hoverEventArgument, &event);
    QCOMPARE(probe.painterArgument, &painter);
    QCOMPARE(probe.converterArgument, &converter);
    QCOMPARE(probe.displayRendererArgument, &displayRenderer);
    QCOMPARE(probe.calls,
             QStringList({QStringLiteral("createStrategy"),
                          QStringLiteral("hoverEvent"),
                          QStringLiteral("paintOnHover"),
                          QStringLiteral("tryUseCustomCursor")}));
}

void KoInteractionStrategyFactoryContractTest::sharedAliasAndBaseOwnershipDestroyDerivedFactory()
{
    int destructionCount = 0;
    KoInteractionStrategyFactorySP owner(new FactoryProbe(5, QStringLiteral("共有"), nullptr, &destructionCount));
    KoInteractionStrategyFactorySP retained = owner;

    owner.clear();
    QCOMPARE(destructionCount, 0);
    QCOMPARE(retained->id(), QStringLiteral("共有"));

    retained.clear();
    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KoInteractionStrategyFactoryContractTest)

#include "KoInteractionStrategyFactoryContractTest.moc"
