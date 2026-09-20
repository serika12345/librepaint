/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoDeferredShapeFactoryBase.h"

#include <QList>
#include <QString>
#include <QTest>

class KoShape
{
};

class KoDocumentResourceManager
{
};

class KoProperties
{
};

namespace
{

class DeferredShapeFactoryProbe final : public KoDeferredShapeFactoryBase
{
public:
    DeferredShapeFactoryProbe(QObject *parent, KoShape *shapeResult, int *destructionCount = nullptr)
        : KoDeferredShapeFactoryBase(parent)
        , m_shapeResult(shapeResult)
        , m_destructionCount(destructionCount)
    {
    }

    ~DeferredShapeFactoryProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    QString deferredPluginName() override
    {
        return QStringLiteral("図形α🎨");
    }

    KoShape *createDefaultShape(KoDocumentResourceManager *documentResources = nullptr) const override
    {
        resourceArguments.append(documentResources);
        return m_shapeResult;
    }

    mutable QList<KoDocumentResourceManager *> resourceArguments;

private:
    KoShape *m_shapeResult = nullptr;
    int *m_destructionCount = nullptr;
};

} // namespace

class KoDeferredShapeFactoryBaseContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesQObjectParentAndVirtualLifetime();
    void dispatchesUnicodeNameAndDefaultShapeArguments();
    void baseCreateShapeIgnoresPropertiesAndForwardsResources();
};

void KoDeferredShapeFactoryBaseContractTest::preservesQObjectParentAndVirtualLifetime()
{
    int destructionCount = 0;
    QObject *parent = new QObject;
    KoDeferredShapeFactoryBase *factory = new DeferredShapeFactoryProbe(parent, nullptr, &destructionCount);

    QCOMPARE(factory->parent(), parent);
    QVERIFY(parent->children().contains(factory));

    delete parent;
    QCOMPARE(destructionCount, 1);
}

void KoDeferredShapeFactoryBaseContractTest::dispatchesUnicodeNameAndDefaultShapeArguments()
{
    KoShape shape;
    KoDocumentResourceManager resources;
    DeferredShapeFactoryProbe probe(nullptr, &shape);
    KoDeferredShapeFactoryBase *base = &probe;

    QCOMPARE(base->deferredPluginName(), QStringLiteral("図形α🎨"));
    QCOMPARE(base->createDefaultShape(&resources), &shape);
    QCOMPARE(base->createDefaultShape(), &shape);
    QCOMPARE(probe.resourceArguments, QList<KoDocumentResourceManager *>({&resources, nullptr}));
}

void KoDeferredShapeFactoryBaseContractTest::baseCreateShapeIgnoresPropertiesAndForwardsResources()
{
    KoShape shape;
    KoProperties firstProperties;
    KoProperties secondProperties;
    KoDocumentResourceManager resources;
    DeferredShapeFactoryProbe probe(nullptr, &shape);
    KoDeferredShapeFactoryBase *base = &probe;

    QCOMPARE(base->createShape(&firstProperties, &resources), &shape);
    QCOMPARE(base->createShape(&secondProperties), &shape);
    QCOMPARE(probe.resourceArguments, QList<KoDocumentResourceManager *>({&resources, nullptr}));
}

QTEST_GUILESS_MAIN(KoDeferredShapeFactoryBaseContractTest)

#include "KoDeferredShapeFactoryBaseContractTest.moc"
