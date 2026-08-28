/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoCanvasResourcesInterface.h>

#include <QTest>
#include <QVariant>

namespace
{
class RecordingCanvasResources final : public KoCanvasResourcesInterface
{
public:
    RecordingCanvasResources(int *requestedKey, bool *destroyed)
        : m_requestedKey(requestedKey)
        , m_destroyed(destroyed)
    {
    }

    ~RecordingCanvasResources() override
    {
        *m_destroyed = true;
    }

    QVariant resource(int key) const override
    {
        *m_requestedKey = key;
        return QStringLiteral("canvas-resource");
    }

private:
    int *m_requestedKey;
    bool *m_destroyed;
};
}

class KoCanvasResourcesInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dispatchesResourceLookupAndSupportsPolymorphicLifetime();
};

void KoCanvasResourcesInterfaceContractTest::dispatchesResourceLookupAndSupportsPolymorphicLifetime()
{
    int requestedKey = -1;
    bool destroyed = false;
    KoCanvasResourcesInterface *resources =
        new RecordingCanvasResources(&requestedKey, &destroyed);

    QCOMPARE(resources->resource(173), QVariant(QStringLiteral("canvas-resource")));
    QCOMPARE(requestedKey, 173);
    delete resources;
    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(KoCanvasResourcesInterfaceContractTest)

#include "KoCanvasResourcesInterfaceContractTest.moc"
