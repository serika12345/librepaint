/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoSelectedShapesProxy.h"

#include <QStringList>
#include <QTest>

class KoSelection
{
};

class KoShapeLayer
{
};

namespace
{

class SelectedShapesProxyProbe final : public KoSelectedShapesProxy
{
public:
    SelectedShapesProxyProbe(KoSelection *selection, QObject *parent = nullptr, int *destructionCount = nullptr)
        : KoSelectedShapesProxy(parent)
        , m_selection(selection)
        , m_destructionCount(destructionCount)
    {
    }

    ~SelectedShapesProxyProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    KoSelection *selection() override
    {
        return m_selection;
    }

    void emitSelectionChanged()
    {
        Q_EMIT selectionChanged();
    }

    void emitSelectionContentChanged()
    {
        Q_EMIT selectionContentChanged();
    }

    void emitCurrentLayerChanged(const KoShapeLayer *layer)
    {
        Q_EMIT currentLayerChanged(layer);
    }

private:
    KoSelection *m_selection;
    int *m_destructionCount;
};

} // namespace

class KoSelectedShapesProxyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parentOwnsDerivedProxy();
    void requestingEditStateDefaultsFalseAndRoundTrips();
    void selectionDispatchesBorrowedPointerThroughBase();
    void signalsPreserveEmissionOrderAndLayerPointer();
};

void KoSelectedShapesProxyContractTest::parentOwnsDerivedProxy()
{
    int destructionCount = 0;

    {
        QObject parent;
        auto *proxy = new SelectedShapesProxyProbe(nullptr, &parent, &destructionCount);

        QCOMPARE(proxy->parent(), &parent);
        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

void KoSelectedShapesProxyContractTest::requestingEditStateDefaultsFalseAndRoundTrips()
{
    SelectedShapesProxyProbe proxy(nullptr);

    QVERIFY(!proxy.isRequestingToBeEdited());

    proxy.setRequestingToBeEdited(true);
    QVERIFY(proxy.isRequestingToBeEdited());

    proxy.setRequestingToBeEdited(false);
    QVERIFY(!proxy.isRequestingToBeEdited());
}

void KoSelectedShapesProxyContractTest::selectionDispatchesBorrowedPointerThroughBase()
{
    KoSelection selection;
    SelectedShapesProxyProbe proxy(&selection);
    KoSelectedShapesProxy *interface = &proxy;

    QCOMPARE(interface->selection(), &selection);
}

void KoSelectedShapesProxyContractTest::signalsPreserveEmissionOrderAndLayerPointer()
{
    SelectedShapesProxyProbe proxy(nullptr);
    KoShapeLayer layer;
    QStringList events;
    const KoShapeLayer *observedLayer = nullptr;

    connect(&proxy, &KoSelectedShapesProxy::selectionChanged, this, [&events] {
        events.append(QStringLiteral("selectionChanged"));
    });
    connect(&proxy, &KoSelectedShapesProxy::selectionContentChanged, this, [&events] {
        events.append(QStringLiteral("selectionContentChanged"));
    });
    connect(&proxy,
            &KoSelectedShapesProxy::currentLayerChanged,
            this,
            [&events, &observedLayer](const KoShapeLayer *value) {
                events.append(QStringLiteral("currentLayerChanged"));
                observedLayer = value;
            });

    proxy.emitSelectionChanged();
    proxy.emitSelectionContentChanged();
    proxy.emitCurrentLayerChanged(&layer);

    QCOMPARE(events,
             QStringList({QStringLiteral("selectionChanged"),
                          QStringLiteral("selectionContentChanged"),
                          QStringLiteral("currentLayerChanged")}));
    QCOMPARE(observedLayer, &layer);
}

QTEST_GUILESS_MAIN(KoSelectedShapesProxyContractTest)

#include "KoSelectedShapesProxyContractTest.moc"
